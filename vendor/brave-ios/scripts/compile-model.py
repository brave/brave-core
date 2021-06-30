#!/usr/bin/env python3

# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import subprocess
import sys

def main():
    args = parse_args()
    compile_model(args.model[0], args.output[0])

def compile_model(model, output):
    xcode = subprocess.check_output(['xcode-select', '-print-path']).decode('utf-8')
    subprocess.call(xcode.strip() + "/usr/bin/momc " + model + " " + output, shell=True)

def parse_args():
    parser = argparse.ArgumentParser(description='Compile a CoreData model')
    parser.add_argument('--model', nargs=1)
    parser.add_argument('--output', nargs=1)
    return parser.parse_args()

if __name__ == '__main__':
    sys.exit(main())
