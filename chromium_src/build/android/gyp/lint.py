# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils


@override_utils.override_function(build_utils)
# pylint: disable=unused-argument
def JavaCmd(original_function, xmx='1G'):  #NOSONAR
    # pylint: enable=unused-argument
    # Override to pass xmx='4G', to fix error
    # java.lang.OutOfMemoryError: Java heap space
    # for Android incremental builds

    return original_function('4G')
