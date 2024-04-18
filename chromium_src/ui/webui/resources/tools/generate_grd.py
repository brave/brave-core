# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

REPLACE_CR_ELEMENTS = ["cr_button", "cr_toggle"]


def generate_file_overrides():
    overrides = dict()
    gen_dir = '${root_gen_dir}/'
    for el in REPLACE_CR_ELEMENTS:
        path = f"ui/webui/resources/tsc/cr_elements/{el}/{el}"
        exts = ['.html.js', '.js']
        for ext in exts:
            overrides[gen_dir + path + ext] = gen_dir + 'brave/' + path + ext

    return overrides


FILE_OVERRIDES = generate_file_overrides()

import override_utils


@override_utils.override_function(globals())
def _generate_include_row(original_function, grd_prefix, filename, pathname, \
                          resource_path_rewrites, resource_path_prefix):
    if pathname in FILE_OVERRIDES:
        pathname = FILE_OVERRIDES[pathname]

    return original_function(grd_prefix, filename, pathname, \
                            resource_path_rewrites, resource_path_prefix)
