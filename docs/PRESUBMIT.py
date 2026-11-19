# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Presubmit script for changes affecting docs/."""

import os

PRESUBMIT_VERSION = '2.0.0'


def CheckKnowledgeBase(input_api, output_api):
    """Lints the knowledge base when it changes."""
    if not any(
        f.LocalPath().replace(os.sep, '/').startswith('docs/knowledge_base/')
        for f in input_api.AffectedFiles()
    ):
        return []
    brave = os.path.dirname(input_api.PresubmitLocalPath())
    return input_api.RunTests(
        [
            input_api.Command(
                name='knowledge_base.py lint',
                cmd=[
                    os.path.join(brave, 'tools', 'cr', 'knowledge_base.py'),
                    'lint',
                ],
                kwargs={'cwd': brave},
                message=output_api.PresubmitError,
            )
        ]
    )
