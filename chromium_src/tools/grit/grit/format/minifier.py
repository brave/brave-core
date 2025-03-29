# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import override_utils


@override_utils.override_function(globals())
def Minify(original_function, source, filename):
    with override_utils.override_scope_variable(
            globals(), 'js_minifier_ignore_list',
            js_minifier_ignore_list + ['gen/brave/web-ui-opaque_ke/']):
        return original_function(source, filename)
