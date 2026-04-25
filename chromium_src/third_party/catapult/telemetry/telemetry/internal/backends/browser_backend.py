# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils


@override_utils.override_method(BrowserBackend)
def UploadLogsToCloudStorage(self, *args, **kwargs):
    """Override to copy browser logs to artifacts instead of uploading"""
    with open(self.log_file_path, 'rb') as infile:
        try:
            artifact_logger.CreateArtifact('browser_log.txt', infile.read())
        except Exception as e:
            logging.error(e)
