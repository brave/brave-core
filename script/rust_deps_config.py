#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

# Version number and URL for pre-configured rust dependency package, e.g. rust_deps_mac_0.1.0.gz
# TODO: set to value below after migrating the script to Python 3
# it now breaks because of urllib2 and SSL/TLS incompatibility when using Fastly
# RUST_DEPS_PACKAGES_URL = "https://rust-pkg-brave-core.s3.brave.com"
RUST_DEPS_PACKAGES_URL = "https://s3-us-west-2.amazonaws.com/rust-pkg-brave-core"
RUST_DEPS_PACKAGE_VERSION = "0.1.1"
