#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import os

# Version number and URL for pre-configured rust dependency package
# e.g. rust_deps_mac_0.1.0.gz
DEPS_PACKAGES_URL = "https://brave-build-deps-public.s3.brave.com"
RUST_DEPS_PACKAGE_VERSION = "1.1.0"
MAC_TOOLCHAIN_ROOT = os.path.join(os.path.dirname(os.path.dirname(
                                  os.path.dirname(__file__))),
                                  'build', 'mac_files')
