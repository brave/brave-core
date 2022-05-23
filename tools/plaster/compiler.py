#!/usr/bin/env python3
#
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */
#

import argparse
import sys
import clang.cindex
from clang.cindex import CursorKind
import idl_schema


def main():
  parser = argparse.ArgumentParser(description='Plaster patching engine')
  args = parser.parse_args()


if __name__ == '__main__':
  sys.exit(main())
