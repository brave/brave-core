#!/usr/bin/env python3

# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from argparse import ArgumentParser

from lib.widevine import generate_sig_file


def main():
    args = parse_args()
    generate_sig_file(args.input_file, args.output_file, args.flags)


def parse_args():
    parser = ArgumentParser()
    parser.add_argument('input_file')
    parser.add_argument('output_file')
    parser.add_argument('flags')
    return parser.parse_args()


if __name__ == '__main__':
    main()
