# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import brave_chromium_utils
import override_utils

@override_utils.override_function(globals())
def _GenerateConfigXmlTree(original_function, orig_config_path,
                           backported_methods):
    # Expand upstream's lint-suppressions.xml with python chromium_src override
    original_root_node = original_function(orig_config_path,
                                           backported_methods)

    if orig_config_path:
        brave_config_path = brave_chromium_utils.get_chromium_src_override(
            orig_config_path)
        brave_root_node = ElementTree.parse(brave_config_path).getroot()
        for item in brave_root_node.findall('issue'):
            original_root_node.append(item)

    return original_root_node
