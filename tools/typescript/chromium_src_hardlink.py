# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os


def ensure_hardlink(src, dst):
    src = os.path.abspath(src) if not os.path.isabs(src) else src
    dst = os.path.abspath(dst) if not os.path.isabs(dst) else dst

    try:
        os.link(src, dst)
    except FileExistsError:
        if not os.path.samefile(src, dst):
            # recreating link if dst is not pointing to the src
            try:
                os.unlink(dst)
                os.link(src, dst)
            except (FileExistsError, FileNotFoundError):
                # Ignore this error, it happens because of a race condition
                # on android when the relevant target is running for more than
                # one architecture. This is a temporary workaround
                # TODO(https://github.com/brave/brave-browser/issues/49768)
                pass
