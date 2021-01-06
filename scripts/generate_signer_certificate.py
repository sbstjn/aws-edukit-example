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
import subprocess
from pyasn1_modules import pem
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend
from datetime import datetime, timedelta
import os
import json


def check_environment():
    """Checks to ensure environment is set per AWS IoT EduKit instructions

    Verifies Miniconda is installed and the 'edukit' virtual environment
    is activated.
    Verifies Python 3.7.x is installed and is being used to execute this script.
    Verifies that the AWS CLI is installed and configured correctly. Prints
    AWS IoT endpoint address.
    """
    conda_env = os.environ.get('CONDA_DEFAULT_ENV')
    if conda_env == None or conda_env == "base":
        print("The 'edukit' Conda environment is not created or activated:\n  To install miniconda, visit https://docs.conda.io/en/latest/miniconda.html.\n  To create the environment, use the command 'conda create -n edukit python=3.7'\n  To activate the environment, use the command 'conda activate edukit'\n")
        exit(0)
    
    if sys.version_info[0] != 3 or sys.version_info[1] != 7:
        print(f"Python version {sys.version}")
        print("Incorrect version of Python detected. Must use Python version 3.7.x. You might want to try the command 'conda install python=3.7'.")
        exit(0)

    aws_iot_endpoint = subprocess.run(["aws", "iot", "describe-endpoint", "--endpoint-type", "iot:Data-ATS", "--region", "eu-central-1"], universal_newlines=True, capture_output=True)
    if aws_iot_endpoint.returncode != 0:
        print("Error with AWS CLI! Follow the configurtion docs at 'https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2.html'")
        exit(0)



def generate_signer_certificate():
    """Generates a x.509 certificate signed by ECDSA key
    This signer certificate is used to generate the device manifest file and helps 
    ensure the validity/ownership of the manifest contents. This signer certificate's
    Distinguished Name (DN) includes the AWS IoT registration code (FDQN)as the 
    common name and can be helpful for fleet provisioning.

    Signer certificate and key is saved in ./output_files/

    Certificate is set to expire in 1 year.
    """

    signer_key = ec.generate_private_key(
        curve = ec.SECP256R1(), 
        backend = default_backend()
    )

    aws_iot_reg_code = subprocess.run(["aws", "iot", "get-registration-code", "--region", "eu-central-1"], universal_newlines=True, capture_output=True)
    if aws_iot_reg_code.returncode != 0:
        print("Error with the AWS CLI when running the command 'aws iot get-registration-code'.")
        exit(0)
    aws_cli_resp = json.loads(aws_iot_reg_code.stdout)

    basepath = os.path.join(os.path.dirname( __file__ ), "..", "tmp")

    if not os.path.exists(basepath):
        os.mkdir(basepath)

    signer_key_pem = signer_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.TraditionalOpenSSL,
        encryption_algorithm=serialization.NoEncryption(),
    )
    with open(os.path.join(basepath, "signer_key.pem"), "wb") as signer_key_file:
        signer_key_file.write(signer_key_pem)

    signer_public_key = signer_key.public_key()
    time_now = datetime.utcnow()
    days_to_expire = 365
    x509_cert = (
        x509.CertificateBuilder()
        .issuer_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, aws_cli_resp['registrationCode']),]))
        .subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, aws_cli_resp['registrationCode']),]))
        .serial_number(x509.random_serial_number())
        .public_key(signer_public_key)
        .not_valid_before(time_now)
        .not_valid_after(time_now + timedelta(days=days_to_expire))
        .add_extension(x509.SubjectKeyIdentifier.from_public_key(signer_public_key), False)
        .add_extension(x509.AuthorityKeyIdentifier.from_issuer_public_key(signer_public_key), False)
        .add_extension(x509.BasicConstraints(ca=True, path_length=None), True)
        .sign(signer_key, hashes.SHA256(), default_backend())
    )

    signer_cert_pem = x509_cert.public_bytes(encoding=serialization.Encoding.PEM)

    with open(os.path.join(basepath, "signer_cert.crt"), "wb") as signer_cert_file:
        signer_cert_file.write(signer_cert_pem)

def main():
    """AWS IoT EduKit MCU hardware device registration script
    Checkes environment is set correctly, generates ECDSA certificates,
    ensures all required python libraries are included, retrieves on-board 
    device certificate using the esp-cryptoauth library and utility, creates 
    an AWS IoT thing using the AWS CLI and Microchip Trust Platform Design Suite.
    """
    check_environment()
    
    generate_signer_certificate()

if __name__ == "__main__":
    main() 