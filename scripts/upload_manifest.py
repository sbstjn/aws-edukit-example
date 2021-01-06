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
from datetime import datetime, timedelta
import os
import re
import json

# Import the Microchip Trust Platform Design Suite libraries
trustplatform_path = os.path.join(os.path.dirname( __file__ ), "..", "vendor", "cryptoauth_trustplatform_designsuite")
sys.path.append(trustplatform_path)

# Import the Microchip Trust Platform Design Suite AWS and manifest helper libraries
trustplatform_aws_path = os.path.join(os.path.dirname( __file__ ), "..", "vendor", "cryptoauth_trustplatform_designsuite", "aws")
sys.path.append(trustplatform_aws_path)
from helper_aws import *
from Microchip_manifest_handler import *

def upload_manifest():
    """Uses Microchip TrustPlatform to register an AWS IoT thing
    Parses through the generated manifest file, creates an AWS IoT thing
    with a thing name that is the ATECC608 secure element serial number,
    applies the device certificate (public key) that is stored in the manifest
    file, and attaches a default policy.
    """
    check_and_install_policy('Default')

    basepath = os.path.join(os.path.dirname( __file__ ), "..", "tmp")

    for file in os.listdir(basepath):
        if re.match("\w+(\_manifest.json)", file):
            manifest_file = open(os.path.join(basepath, file), "r")
            manifest_data = json.loads(manifest_file.read())

            signer_cert = open(os.path.join(basepath,"signer_cert.crt"), "r").read()
            signer_cert_bytes = str.encode(signer_cert)
            invoke_import_manifest('Default', manifest_data, signer_cert_bytes)
            invoke_validate_manifest_import(manifest_data, signer_cert_bytes)

def main():
    """AWS IoT EduKit MCU hardware device registration script
    Checkes environment is set correctly, generates ECDSA certificates,
    ensures all required python libraries are included, retrieves on-board 
    device certificate using the esp-cryptoauth library and utility, creates 
    an AWS IoT thing using the AWS CLI and Microchip Trust Platform Design Suite.
    """
    
    upload_manifest()

if __name__ == "__main__":
    main() 