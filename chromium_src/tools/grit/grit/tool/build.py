# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from override_utils import override_function, override_method
from brave_chromium_utils import sys_path


@override_function(globals())
def GetFormatter(original_function, type):
    """Add support for webui types"""
    webui_formats = ['webui_ts', 'webui_header']

    if type in webui_formats:
        with sys_path('//brave/tools/grit', 0):
            import brave_grit.format.webui_strings as webui_strings
            return webui_strings.GetFormatter(type)

    return original_function(type)


@override_method(RcBuilder)
def _EncodingForOutputType(_, original_method, output_type):
    """We need to override the encoding for webui_locale to be utf_8, as grit
       outputs utf_16 by default.
    """
    if output_type in ('webui_header', 'webui_ts'):
        return 'utf_8'
    return original_method(output_type)
