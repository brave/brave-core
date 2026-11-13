# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import brave_chromium_utils


def _brave_add_chromium_src_overrides(zf, sources_by_rel):
    """Bundles the `//brave/chromium_src` override of each bundled module, as
    source, where `brave_chromium_utils` inlines it from, and returns their
    paths so that a change to one rebuilds the archive."""
    overrides = []
    for rel_py in sorted(sources_by_rel):
        override = brave_chromium_utils.wspath(f'//brave/chromium_src/{rel_py}')
        if not os.path.exists(override):
            continue
        with open(override, 'rb') as f:
            zip_helpers.add_to_zip_hermetic(
                zf,
                f'{brave_chromium_utils.PYZ_CHROMIUM_SRC_DIR}/{rel_py}',
                data=f.read(),
            )
        overrides.append(override)
    return overrides
