# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import sys
import subprocess
import tempfile

def main():
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument('zipalign_path')
    argument_parser.add_argument('apksigner_path')
    argument_parser.add_argument('jarsigner_path')
    argument_parser.add_argument('unsigned_apk_paths', nargs='+')
    argument_parser.add_argument('key_path')
    argument_parser.add_argument('key_passwd')
    argument_parser.add_argument('prvt_key_passwd')
    argument_parser.add_argument('key_name')
    argument_parser.add_argument('--pkcs11-provider', help='PKCS11 provider configuration file path')
    argument_parser.add_argument('--pkcs11-alias', help='PKCS11 key alias')
    args = argument_parser.parse_args()

    sign(args.zipalign_path, args.apksigner_path, args.jarsigner_path, \
        args.unsigned_apk_paths, args.key_path, args.key_passwd, \
        args.prvt_key_passwd, args.key_name, args.pkcs11_provider, args.pkcs11_alias)

def sign(zipalign_path, apksigner_path, jarsigner_path, \
    unsigned_apk_paths, key_path, key_passwd, prvt_key_passwd, \
    key_name, pkcs11_provider=None, pkcs11_alias=None):
    print(f"Starting APK signing process for {len(unsigned_apk_paths)} file(s)")

    with tempfile.NamedTemporaryFile() as staging_file:
        for unsigned_apk_path in unsigned_apk_paths:
            subprocess.check_output([
                zipalign_path, '-p', '-f', '4', unsigned_apk_path,
                staging_file.name
            ])
            if os.path.splitext(unsigned_apk_path)[1] == '.apk':
                cmd_args = [
                    apksigner_path,
                    'sign',
                    '--in',
                    staging_file.name,
                    '--out',
                    unsigned_apk_path,
                    '--ks',
                    key_path,
                    '--ks-key-alias',
                    key_name,
                    '--ks-pass',
                    'pass:' + key_passwd,
                    '--key-pass',
                    'pass:' + prvt_key_passwd,
                ]
            else:
                if pkcs11_provider and pkcs11_alias:
                    print("Using PKCS11 signing")
                    if not os.path.exists(pkcs11_provider):
                        raise FileNotFoundError(f"PKCS11 provider config file not found: {pkcs11_provider}")
                    cmd_args = [
                        jarsigner_path, '-verbose',
                        '-keystore', 'NONE',
                        '-storetype', 'PKCS11',
                        '-providerClass', 'sun.security.pkcs11.SunPKCS11',
                        '-providerArg', pkcs11_provider,
                        '-storepass', 'password',
                        staging_file.name, '-signedjar', unsigned_apk_path, pkcs11_alias
                    ]
                else:
                    print("Using standard keystore signing")
                    cmd_args = [
                        jarsigner_path, '-verbose',
                        '-sigalg', 'SHA256withRSA',
                        '-digestalg', 'SHA-256',
                        '-keystore', key_path,
                        '-storepass', key_passwd,
                        '-keypass', prvt_key_passwd,
                        staging_file.name, '-signedjar', unsigned_apk_path, key_name
                    ]
            try:
                result = subprocess.run(cmd_args, capture_output=True, text=True, check=True)
                print(f"Successfully signed: {unsigned_apk_path}")
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Signing failed for {unsigned_apk_path}")
                print(f"  Command: {' '.join(cmd_args)}")
                print(f"  Exit code: {e.returncode}")
                print(f"  Stdout: {e.stdout}")
                print(f"  Stderr: {e.stderr}")
                raise

    print("APK signing process completed")

if __name__ == '__main__':
    sys.exit(main())
