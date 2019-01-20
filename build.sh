#!/bin/bash

# Note: You need to have the esp8266 board installed in Arduino for this to work.
# Running these two commands *should* install the correct board definitions.
# arduino --pref "boardsmanager.additional.urls=http://arduino.esp8266.com/stable/package_esp8266com_index.json" --save-prefs
# arduino --install-boards esp8266:esp8266 --save-prefs
#
# Then you need to actually open up the arduino ide and set the settings in there
# once.  I'm not sure why you can't do it from the command line, but F_CPU doesn't
# seem to get set otherwise.  Once you do it with the ide once this script should
# work.

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

# The directory where the networking password(s) are stored
PASSWORDS_DIR="passwords"

# Derived values from the configuration constants
OUTPUT_BIN="${BUILD_DIR}/${INO}.bin"

# Where ESPTOOL is, use find to not need to set it all up
ESPTOOL="$(find ${HOME}/.arduino15/ -name esptool -type f)"

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
  echo "Usage: ./build.sh [--flash]"
  echo "build.sh will compile the project and optionally flash all availible ESPs"
  echo "with the program over a serial connection"
  exit 1
}


function create_password_file_if_needed {
  # This software relies on having secret passwords for networking, but for
  # obvious security reasons these shouldn't be included in github.  They're
  # instead defined in *_password.h files that are #included into the source.
  # These *_password.h files can't be checked out, they need to be regenerated
  # with each new checkout, so this function checks if they exists and builds
  # them from user input if neccessary.
  password_type="$1"
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

  # Check to make sure the Mesh password exists.  We can't build without them
  # because it's required information for connecting to the mesh network.
  # TODO -- uncomment this and actually use this password.
  # create_password_file_if_needed "mesh"

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
    ${ESPTOOL} -cd ck -cb 115200 -cp ${port} -ca 0x00000 -cf ${OUTPUT_BIN}
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
if [ $# -gt 1 ]; then
    print_error "Too many arguments ($#)"
    usage
fi

FLASH_REQUESTED=0
if [ $# -eq 1 ]; then
  if [ $1 == "--flash" -o $1 == "-f" ]; then
    FLASH_REQUESTED=1
  else
    print_error "Unknown flag $1"
    usage
  fi
fi

# Compile the .ino for ESP8266 using the Arduino IDE from the command line
compile

# If selected, flash the FW update
if [ ${FLASH_REQUESTED} -ne 0 ]; then
  do_serial_fw_updates
fi
