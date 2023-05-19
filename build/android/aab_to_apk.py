# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import shlex
import shutil
import subprocess
import sys
import zipfile
import sign_apk


def main():
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument('bundletool')
    argument_parser.add_argument('target_aab_path')
    argument_parser.add_argument('output_apk_path')
    argument_parser.add_argument('output_path')
    argument_parser.add_argument('key_path')
    argument_parser.add_argument('key_passwd')
    argument_parser.add_argument('prvt_key_passwd')
    argument_parser.add_argument('key_name')
    argument_parser.add_argument('zipalign_path')
    argument_parser.add_argument('apksigner_path')
    argument_parser.add_argument('jarsigner_path')
    argument_parser.add_argument('key_info_solana', type=str, nargs='+')
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
    utils_path = [args.zipalign_path, args.apksigner_path, args.jarsigner_path]
    transform(args.output_apk_path, universal_apk, \
        utils_path, args.key_path, args.key_passwd, \
        args.prvt_key_passwd, args.key_name)
    if len(args.key_info_solana) >= 5:
        transform(args.key_info_solana[0], universal_apk, \
            utils_path, args.key_info_solana[1], \
            args.key_info_solana[2], args.key_info_solana[3], \
            args.key_info_solana[4])
    if os.path.isfile(apks_name):
        os.remove(apks_name)
    if os.path.isfile(universal_apk):
        os.remove(universal_apk)


def transform(output_apk_path, universal_apk, utils_path, key_path, \
    key_passwd, prvt_key_passwd, key_name):
    if os.path.isfile(output_apk_path):
        os.remove(output_apk_path)
    try:
        shutil.copy(universal_apk, output_apk_path)
    except IOError as e:
        print("Unable to copy file. %s" % e)
        return

    sign_apk.sign(utils_path[0], utils_path[1], \
        utils_path[2], [ output_apk_path ], key_path, \
        key_passwd, prvt_key_passwd, key_name)


if __name__ == '__main__':
    sys.exit(main())
