#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Opens a test profile in the dev build with `pnpm start`, to try it by hand.

  ./open_profile.py search            # the semantic search profile
  ./open_profile.py learned-memory    # with the learned memory switches
  ./open_profile.py demo              # the demo profile, for the demo

Other arguments go to the browser.
"""

import argparse
import subprocess

import cdp
import demo_profile
import learned_memory_eval
import search_profile

PROFILES = {
    'search': (search_profile.PROFILE_NAME, []),
    'learned-memory': (
        learned_memory_eval.PROFILE_NAME,
        learned_memory_eval.SWITCHES,
    ),
    'demo': (demo_profile.PROFILE_NAME, demo_profile.SWITCHES),
}


def main():
    parser = argparse.ArgumentParser(
        description=__doc__.split('\n', maxsplit=1)[0]
    )
    parser.add_argument('profile', choices=sorted(PROFILES))
    args, browser_args = parser.parse_known_args()
    cdp.check_build()
    name, switches = PROFILES[args.profile]
    if args.profile == 'demo':
        # The daily Dreaming run must not learn before the demo does.
        demo_profile.set_prefs(demo_profile.Demo(name))
    subprocess.run(
        ['pnpm', 'start', f'--user_data_dir_name={name}']
        + switches
        + browser_args,
        cwd=cdp.BRAVE_DIR,
        check=False,
    )


if __name__ == '__main__':
    main()
