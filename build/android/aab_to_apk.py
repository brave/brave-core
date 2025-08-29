# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import shlex
import subprocess
import sys
import zipfile
import sign_apk


def main():
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument('--bundletool',
                                 required=True,
                                 help='Path to bundletool JAR file')
    argument_parser.add_argument('--target-aab-path',
                                 required=True,
                                 help='Path to the target AAB file')
    argument_parser.add_argument('--output-apk-path',
                                 required=True,
                                 help='Path for the output APK file')
    argument_parser.add_argument('--output-path',
                                 required=True,
                                 help='Output directory path')
    argument_parser.add_argument('--key-path',
                                 required=True,
                                 help='Path to keystore file')
    argument_parser.add_argument('--key-passwd',
                                 required=True,
                                 help='Keystore password')
    argument_parser.add_argument('--prvt-key-passwd',
                                 required=True,
                                 help='Private key password')
    argument_parser.add_argument('--key-name',
                                 required=True,
                                 help='Key alias name')
    argument_parser.add_argument('--zipalign-path',
                                 required=True,
                                 help='Path to zipalign tool')
    argument_parser.add_argument('--apksigner-path',
                                 required=True,
                                 help='Path to apksigner tool')
    argument_parser.add_argument('--jarsigner-path',
                                 required=True,
                                 help='Path to jarsigner tool')
    args = argument_parser.parse_args()

    apks_name = os.path.splitext(args.output_apk_path)[0] + ".apks"

    if os.path.isfile(apks_name):
        os.remove(apks_name)
    command_line = "java -jar " + args.bundletool + " build-apks " + \
                   "--bundle=" + args.target_aab_path + \
                   " --output=" + apks_name + \
                   " --mode=universal " + \
                   "--ks=" + args.key_path + \
                   " --ks-key-alias=" + args.key_name + \
                   " --ks-pass=pass:" + args.key_passwd + \
                   " --key-pass=pass:" + args.prvt_key_passwd
    cmd_args = shlex.split(command_line)
    subprocess.check_output(cmd_args)

    universal_apk = args.output_path + 'universal.apk'
    if os.path.isfile(universal_apk):
        os.remove(universal_apk)
    with zipfile.ZipFile(apks_name, 'r') as z:
        z.extract('universal.apk', args.output_path)
        z.close()

    if os.path.isfile(args.output_apk_path):
        os.remove(args.output_apk_path)
    os.rename(universal_apk, args.output_apk_path)

    if os.path.isfile(apks_name):
        os.remove(apks_name)

    sign_apk.sign(zipalign_path=args.zipalign_path,
                  apksigner_path=args.apksigner_path,
                  jarsigner_path=args.jarsigner_path,
                  unsigned_apk_paths=[args.output_apk_path],
                  key_path=args.key_path,
                  key_passwd=args.key_passwd,
                  prvt_key_passwd=args.prvt_key_passwd,
                  key_name=args.key_name)


if __name__ == '__main__':
    sys.exit(main())
