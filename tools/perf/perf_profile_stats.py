#!/usr/bin/env vpython3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
r"""A tool to calculate disk usage statistics for perf profiles

Usage:
tools/perf/perf_profile_stats.py brave-typical-mac v1.68.1 > /tmp/old.txt
tools/perf/perf_profile_stats.py brave-typical-mac v1.69.22 > /tmp/new.txt
code --diff /tmp/old.txt /tmp/new.txt
"""
import argparse
import tempfile
import json

import components.profile_tools as profile_tools
from components.version import BraveVersion

parser = argparse.ArgumentParser()
parser.add_argument('profile', type=str)
parser.add_argument('version', type=str)
parser.add_argument('work_directory', type=str, default=None, nargs='?')
parser.add_argument('--json',
                    action='store_true',
                    help='output json instead of text')
parser.add_argument('-s', '--skip-chromium-components', action='store_true')
args = parser.parse_args()

if not args.work_directory:
  args.work_directory = tempfile.mkdtemp(prefix='perf-profile-')

profile_dir = profile_tools.GetProfilePath(args.profile, args.work_directory,
                                           BraveVersion(args.version))
stats = profile_tools.GetProfileStats(profile_dir,
                                      args.skip_chromium_components)

if args.json:
  print(json.dumps(stats.toJSON(), indent=2))
else:
  print(stats.toText())
