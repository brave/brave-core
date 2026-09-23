#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os


def main():
    parser = argparse.ArgumentParser("Hardlink a file from src to dst.")
    parser.add_argument('src', type=str)
    parser.add_argument('dst', type=str)
    args = parser.parse_args()

    ensure_hardlink(args.src, args.dst)


def ensure_hardlink(src, dst):
    src = os.path.abspath(src) if not os.path.isabs(src) else src
    dst = os.path.abspath(dst) if not os.path.isabs(dst) else dst

    try:
        os.link(src, dst)
    except FileExistsError:
        if not os.path.samefile(src, dst):
            # recreating link if dst is not pointing to the src
            os.unlink(dst)
            os.link(src, dst)


if __name__ == '__main__':
    main()
