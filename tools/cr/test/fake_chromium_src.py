# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from pathlib import PurePath
import os
from unittest.mock import patch
from test.fake_chromium_repo import FakeChromiumRepo

import repository


class FakeChromiumSrc(FakeChromiumRepo):
    """Extends FakeChromiumRepo to manage repository patches."""

    def __init__(self):
        super().__init__()
        self.brave_patch = patch('repository.BRAVE_CORE_PATH',
                                 PurePath(self.brave))
        self.chromium_patch = patch('repository.CHROMIUM_SRC_PATH',
                                    PurePath(self.chromium))
        self.original_cwd = None

    def setup(self):
        """Set up the fake repo and apply patches."""
        self.brave_patch.start()
        self.chromium_patch.start()

        # Re-initialize the global instances in the repository module
        repository.chromium = repository.Repository(PurePath(self.chromium))
        repository.brave = repository.Repository(PurePath(self.brave))

        # Change the current working directory to the fake Chromium repo
        self.original_cwd = os.getcwd()
        os.chdir(self.brave)

    def cleanup(self):
        """Clean up the fake repo, patches, and restore the working directory"""
        self.brave_patch.stop()
        self.chromium_patch.stop()
        if self.original_cwd:
            os.chdir(self.original_cwd)
        super().cleanup()
