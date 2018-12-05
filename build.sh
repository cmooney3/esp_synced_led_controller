#!/bin/bash

# The board spec for the Arduino compiler to use as a target when building
BOARD="esp8266:esp8266:generic"
# Location of the arduino executable to use
ARDUINO="arduino"
# Location of the main source file to compile
INO="esp_synced_led_controller.ino"
# Temporary directory to store build artifacts
BUILD_DIR="/tmp/esp8266_arduino_builds"
# Usually the "action" is --upload, but since we might update multiple ESPs at
# once we'll handle that later for efficiency's sake
ACTION="--verify"

# The directory where the OTA and wifi passwords are stored
PASSWORDS_DIR="passwords"

# The location of espota.py, this is used to perform ota updates
ESPOTA="/home/${USER}/.arduino15/packages/esp8266/hardware/esp8266/"`
      `"2.3.0/tools/espota.py"
# The OTA port we're using
OTA_PORT="8266"
# The header file that defines the OTA password when compiling.  This OTA
# password is needed in this script to apply an OTA FW update.
OTA_PASSWORD_FILE="${PASSWORDS_DIR}/ota_password.h"


# Derived values from the configuration constants
OUTPUT_BIN="${BUILD_DIR}/${INO}.bin"

# Color codes to allow us to make the output prettier and easier to read
RED='\033[0;31m'
YELLOW='\033[0;33m'
GREEN='\033[0;32m'
WHITE='\033[0;97m'
NC='\033[0m' # No Color

function print_heading {
  # Print out a nice colored heading to help separate out various sections of
  # the output.
  heading_title=$1
  echo -e "${WHITE}------------------------------------------------------------${NC}"
  echo -e "${WHITE}${1}${NC}"
  echo -e "${WHITE}------------------------------------------------------------${NC}"
}

function print_error {
  echo -e "${RED}ERROR${NC}  $@"
}

function print_success {
  echo -e "${GREEN}SUCCESS${NC}  $@"
}

function print_warn {
  echo -e "${YELLOW}WARNING${NC}  $@"
}

function usage {
  echo "Usage: ./build.sh [--ota|--serial]"
  echo "build.sh will compile the project and optionally flash all availible ESPs"
  echo "with the program over either serial connections or OTA updates"
  exit 1
}


function create_password_file_if_needed {
  # This software relies on having both a WIFI and an OTA password, but for
  # obvious security reasons these shouldn't be included in github.  They're
  # instead defined in *_password.h files that are #included into the source.
  # These *_password.h files can't be checked out, they need to be regenerated
  # with each new checkout, so this function checks if they exists and builds
  # them from user input if neccessary.
  password_type="$1"  # Either "ota" or "wifi"
  password_file="${PASSWORDS_DIR}/${password_type}_password.h"

  if [ ! -f "${password_file}" ]; then
    print_warn "Missing ${password_file}!  Generating one now:"

    echo "What is the ${password_type} password? "
    read password

    mkdir -p ${PASSWORDS_DIR}
    echo "const char* ${password_type}_password = \"${password}\";" > ${password_file}
  fi
}

function compile {
  # Actually compile the source code into a binary using the Arduino compiler.
  print_heading "COMPILING..."

  # Make sure the build directory exists
  mkdir -p ${BUILD_DIR}

  # Check to make sure the *_password.h files exist.  We can't build without them
  create_password_file_if_needed "wifi"
  create_password_file_if_needed "ota"

  # Compilation command
  JAVA_TOOL_OPTIONS='-Djava.awt.headless=true' "${ARDUINO}" "${ACTION}" \
      "${INO}" --board "${BOARD}"  --pref build.path="${BUILD_DIR}" # --verbose

  # Check that everything went smoothly
  err=$?
  if [ ${err} -ne 0 ]; then
    print_error "${INO} Failed to compile."
    echo "Unable to continue to uploading step, exiting now."
    exit $err 
  else
    print_success "${INO} compiled without errors."
  fi
  echo
}

function find_all_arduino_ota_ips {
  # This scans the LAN for esp's that are availible for OTA updates.  Right now
  # we just assume *all* the esps that support OTA on the LAN are ours.
  # Note: When calling this if you want the ips in a nice bash array you have
  # to call it with ()'s around it like ips=($(find_all_arduino_ota_ips)).
  # Also Note: I don't know how this function works, I just copy/pasted from 
  # https://github.com/esp8266/Arduino/issues/3553
  # TODO -- Check to see if these devices are the right kind (not other esps)
  ((avahi-browse _arduino._tcp --resolve --parsable --terminate) 2>/dev/null | \
    grep -F "=;") | cut -d\; -f8
}

function do_ota_fw_updates {
  # This function sends out OTA FW updates to every ESP it can find on the LAN
  # by first scanning, then sending out the updates one by one.

  # First detect all the ESP's on the LAN and build an array of their IPs.
  print_heading "DETECTING OTA DEVICES ON THE LAN..."
  ota_ips=($(find_all_arduino_ota_ips))
  if [ ${#ota_ips[@]} -eq 0 ]; then
    print_error "No OTA device(s) found on the network."
  else
    print_success "Found ${#ota_ips[@]} OTA devices on the network:"
    for ip in ${ota_ips[@]}
    do
      echo -e "\t- ${ip}"
    done
  fi
  echo

  # Get the OTA password out of ota_password.h
  ota_password="$(cat ${OTA_PASSWORD_FILE} | cut -d\" -f2)"

  # Go through each of those IPs one-by-one and perform OTA FW updates.
  print_heading "PERFORMING OTA FW UPDATES..."
  for ip in "${ota_ips[@]}"
  do
    echo "FW Update Target: ${ip}"
    python ${ESPOTA} -i $ip -p "${OTA_PORT}" -f "${OUTPUT_BIN}" \
	   --auth="${ota_password}"
    ret=$?
    if [ ${ret} -ne 0 ]; then
      print_error "espota.py failed to apply update (err code: ${ret})"
    else
      print_success
    fi
  done
}

function do_serial_fw_updates() {
  # First look for all the /dev/ttyUSB* devices and put them in an array so we
  # can scan through them and see if they're ESP's
  print_heading "FINDING ALL SERIAL PORTS..."
  shopt -s nullglob # This makes an empty glob still create a legal, empty array
  serial_ports=(/dev/ttyUSB*)
  if [ ${#serial_ports[@]} -eq 0 ]; then
    # If we didn't find anything in /dev/ttyUSB* then there isn't a serial
    # converter plugged into the computer, and there's no hope of performing
    # a serial FW update.  So we just print an error message and quit.
    print_error "No serial ports found."
    echo "There are no /dev/ttyUSB* devices.  Are you sure the serial port "`
        `"is set up?"
    exit -1
  fi
  # If we're here it means there's at least one ttyUSB* device found, so we
  # should attempt to upload a new FW to each serial port.
  echo "USB serial devices found: "
  for port in ${serial_ports[@]}
  do
    echo -e "\t- ${port}"
  done
  echo

  print_heading "PERFORMING SERIAL FW UPDATES..."
  for port in "${serial_ports[@]}"
  do
    echo "FW Update Target: ${port}"
    # Note: this line was generated by copy/pasting what the arduino IDE does
    # when updating.  If something changes, just change --verify to --upload 
    # (and add --verbose) and run it once.  Look for the line like this that
    # uploads the program and replace it here.
    ~/.arduino15/packages/esp8266/tools/esptool/0.4.9/esptool -cd ck \
     -cb 115200 -cp ${port} -ca 0x00000 -cf ${OUTPUT_BIN}
    ret=$?

    # Check that there were no errors when uploading.
    if [ ${ret} -ne 0 ]; then
      print_error "esptool failed to apply update (err code: ${ret})"
    else
      print_success
    fi
  done
}

################################################################################
# Actual script starts running here 
################################################################################
# Parse the command line arguments
UPDATE_SERIAL="serial"
UPDATE_OTA="ota"
UPDATE_TYPE="" # Don't upload at all by default, just build.
if [ $# -gt 1 ]; then
    print_error "Too many arguments ($#)"
    usage
fi
if [ $# -eq 1 ]; then
  if [ $1 == "--serial" -o $1 == "-s" ]; then
    UPDATE_TYPE="${UPDATE_SERIAL}"
  elif [ $1 == "--ota" -o $1 == "-o" ]; then
    UPDATE_TYPE="${UPDATE_OTA}"
  else
    print_error "Unknown flag $1"
    usage
  fi
fi

# Compile the .ino for ESP8266 using the Arduino IDE from the command line
compile

# If selected the FW update style of choice (either OTA or via serial connection)
if [ "${UPDATE_TYPE}" == "${UPDATE_OTA}" ]; then
  do_ota_fw_updates
elif [ "${UPDATE_TYPE}" == "${UPDATE_SERIAL}" ]; then
  do_serial_fw_updates
else
  exit 0
fi
