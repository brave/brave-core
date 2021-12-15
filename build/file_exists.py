#!/usr/bin/env python3

# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
"""Writes True if the argument is a file."""

import os.path
import sys


def main():
    sys.stdout.write(str(os.path.isfile(sys.argv[1])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
