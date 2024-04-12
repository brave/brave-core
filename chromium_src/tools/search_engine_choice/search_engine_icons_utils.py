# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils


@override_utils.override_function(globals())
def get_used_engines(orig_func, src_dir):
    engines = orig_func(src_dir)
    for engine in {'duckduckgo', 'qwant'}:
        if engine in engines:
            engines.remove(engine)
    return engines
