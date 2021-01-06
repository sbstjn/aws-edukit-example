# AWS IoT EduKit Pre-Provisioned MCU Device Registration Helper
# v1.0.0
#
# Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import sys
import argparse
from datetime import datetime, timedelta
import os
import json

# Verify ESP-IDF is installed and environmental variables are added to PATH
try:
    import esptool
except ImportError: 
    idf_path = os.getenv("IDF_PATH")
    if not idf_path or not os.path.exists(idf_path):
        print("\n\nESP-IDF not found! Install ESP-IDF and run the export script...\n\n")
        raise
    sys.path.insert(0, os.path.join(idf_path, "components", "esptool_py", "esptool"))
    import esptool

# Import the Espressif CryptoAuthLib Utility libraries
sys.path.append(os.path.abspath(os.path.join(os.path.dirname( __file__ ), "..",  "components", "esp-cryptoauthlib", "esp_cryptoauth_utility")))
import helper_scripts as esp_hs

# Import the Microchip Trust Platform Design Suite libraries
trustplatform_path = os.path.join(os.path.dirname( __file__ ), "..", "vendor", "cryptoauth_trustplatform_designsuite")
sys.path.append(trustplatform_path)

# Import the Microchip Trust Platform Design Suite AWS and manifest helper libraries
trustplatform_aws_path = os.path.join(os.path.dirname( __file__ ), "..", "vendor", "cryptoauth_trustplatform_designsuite", "aws")
sys.path.append(trustplatform_aws_path)
from helper_aws import *
from Microchip_manifest_handler import *



def main():
    """AWS IoT EduKit MCU hardware device registration script
    Checkes environment is set correctly, generates ECDSA certificates,
    ensures all required python libraries are included, retrieves on-board 
    device certificate using the esp-cryptoauth library and utility, creates 
    an AWS IoT thing using the AWS CLI and Microchip Trust Platform Design Suite.
    """
    app_binary = 'sample_bins/secure_cert_mfg_esp32.bin'
    parser = argparse.ArgumentParser(description='''Provision the Core2 for AWS IoT EduKit with 
        device_certificate and signer_certificate required for TLS authentication''')

    parser.add_argument(
        "--port", '-p',
        dest='port',
        metavar='[port]',
        required=True,
        help='Serial comm port of the Core2 for AWS IoT EduKit device')

    args = parser.parse_args()

    basepath = os.path.join(os.path.dirname( __file__ ), "..", "tmp")

    args.basepath = basepath
    args.signer_cert = os.path.join(basepath, "signer_cert.crt")
    args.signer_privkey = os.path.join(basepath, "signer_key.pem")
    args.print_atecc608a_type = False

    esp = esptool.ESP32ROM(args.port,baud=115200)
    esp_hs.serial.load_app_stub(app_binary, esp)
    init_mfg = esp_hs.serial.cmd_interpreter()

    retval = init_mfg.wait_for_init(esp._port)
    if retval is not True:
        print("CMD prompt timed out.")
        exit(0)

    retval = init_mfg.exec_cmd(esp._port, "init")
    esp_hs.serial.esp_cmd_check_ok(retval, "init")
    esp_hs.generate_manifest_file(esp, args, init_mfg)

    # # upload_manifest()


if __name__ == "__main__":
    main() 