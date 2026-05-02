# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
from unittest.mock import patch
from test.fake_chromium_repo import FakeChromiumRepo

import plaster as plaster_module
import repository


class FakeChromiumSrc(FakeChromiumRepo):
    """Extends FakeChromiumRepo to manage repository patches."""

    def __init__(self):
        super().__init__()
        self.brave_patch = patch('repository.BRAVE_CORE_PATH', self.brave)
        self.chromium_patch = patch('repository.CHROMIUM_SRC_PATH',
                                    self.chromium)
        self.repository_chromium_patch = patch(
            'repository.chromium', repository.Repository(self.chromium))
        self.repository_brave_patch = patch('repository.brave',
                                            repository.Repository(self.brave))
        self.plaster_files_path_patch = patch.object(plaster_module,
                                                     'PLASTER_FILES_PATH',
                                                     self.brave / 'rewrite')
        self.original_cwd = None

    def setup(self):
        """Set up the fake repo and apply patches."""
        self.brave_patch.start()
        self.chromium_patch.start()
        self.repository_chromium_patch.start()
        self.repository_brave_patch.start()
        self.plaster_files_path_patch.start()

        # Re-initialize the global instances in the repository module
        repository.chromium = repository.Repository(self.chromium)
        repository.brave = repository.Repository(self.brave)

        (self.brave / 'chromium_src').mkdir(exist_ok=True)
        (self.brave / 'rewrite').mkdir(exist_ok=True)

        # Change the current working directory to the fake Chromium repo
        self.original_cwd = os.getcwd()
        os.chdir(self.brave)

    def cleanup(self):
        """Clean up the fake repo, patches, and restore the working directory"""
        self.brave_patch.stop()
        self.chromium_patch.stop()
        self.repository_chromium_patch.stop()
        self.repository_brave_patch.stop()
        self.plaster_files_path_patch.stop()
        if self.original_cwd:
            os.chdir(self.original_cwd)
        super().cleanup()
