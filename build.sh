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

# The location of espota.py, this is used to perform ota updates
ESPOTA="/home/${USER}/.arduino15/packages/esp8266/hardware/esp8266/"`
      `"2.3.0/tools/espota.py"
# The OTA port we're using
OTA_PORT="8266"
# The header file that defines the OTA password when compiling.  This OTA
# password is needed in this script to apply an OTA FW update.
OTA_PASSWORD_FILE="ota_password.h"

# Derived values from the configuration constants
OUTPUT_BIN="${BUILD_DIR}/${INO}.bin"

# Color codes to allow us to make the output prettier and easier to read
RED='\033[0;31m'
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

function compile {
  # Actually compile the source code into a binary using the Arduino compiler.
  print_heading "COMPILING..."
  JAVA_TOOL_OPTIONS='-Djava.awt.headless=true' "${ARDUINO}" "${ACTION}" \
      "${INO}" --board "${BOARD}"  --pref build.path="${BUILD_DIR}" # --verbose
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
  print_heading "SENDING OTA FW UPDATES..."
  for ip in "${ota_ips[@]}"
  do
    echo "Target: ${ip}"
    python ${ESPOTA} -i $ip -p "${OTA_PORT}" -f "${OUTPUT_BIN}" \
	   --auth="${ota_password}"
    ret=$?
    if [ ${ret} -ne 0 ]; then
      print_error "esptool.py failed to apply update (err code: ${ret})"
    else
      print_success
    fi
  done
}

################################################################################
# Actual script starts running here 
################################################################################
# Make sure the build directory exists
mkdir -p ${BUILD_DIR}

# Compile the .ino for ESP8266 using the Arduino IDE from the command line
compile

# Do the uploading of choice (either OTA or via serial connection)
do_ota_fw_updates

##  Uploading with serial ######################################################
# Now upload the binary to each ESP8266 we can find
# # First look for all the /dev/ttyUSB* devices and put them in an array so we
# # can scan through them and see if they're ESP's
# shopt -s nullglob # This makes an empty glob still create a legal, empty array
# serial_devices=(/dev/ttyUSB*)
# echo "USB serial devices found: [${serial_devices[@]}]"
# 
# # TODO - use the array built above to do this in a cleaner way!!
# if [ ${#serial_devices[@]} -eq 0 ]; then
#     echo -e "${RED}ERROR${NC}\tCan't attempt to upload -- no devices found."
#     echo "I don't see any /dev/ttyUSB* devices.  Are you sure you've got an esp hooked up?"
#     exit -1
# else
#   pids_to_wait_for=""
#   echo "Spawning uploading process for each ESP8266 we can find:"
#   for PORT in "${serial_devices[@]}"
#   do
#     echo -n "	Starting ${PORT}..."
#     # Note: this line was generated by copy/pasting what the arduino IDE does when updating.  If something changes, just
#     # change --verify to --upload (and add --verbose) and run it once.  Look for the line like this that uploads the program and replace it here.
#     (~/.arduino15/packages/esp8266/tools/esptool/0.4.9/esptool -vv -cd ck -cb 115200 -cp ${PORT} -ca 0x00000 -cf ${OUTPUT_BIN} 1> /dev/null) &
#     pid=$!
#     echo "	STARTED (pid:${pid})"
#     pids_to_wait_for="${pid} ${pids_to_wait_for}"
#   done
# 
#   # wait for all pids
#   echo "All upload subprocesses spawned.  Waiting for them to complete..."
#   upload_error_detected=0
#   for pid in ${pids_to_wait_for[@]}; do
#       echo -n "	Waiting for pid ${pid}..."
#       wait $pid
#       ret_code=$?
#       if [ ${ret_code} -ne 0 ]; then
#         upload_error_detected=1
#         echo -ne "${RED}" 
#       else
#         echo -ne "${GREEN}" 
#       fi
#       echo -e "	DONE (return code: ${ret_code})${NC}"
#   done
#   echo "All upload subprocesses completed."
# 
#   if [ ${upload_error_detected} -eq 1 ]; then
#     echo -e "${RED}ERROR:  At least one upload subprocess failed!${NC}"
#   else
#     echo -e "${GREEN}SUCCESS:  All upload subprocess succeded!${NC}"
#   fi
# fi
# 
