# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import brave_chromium_utils
import override_utils

# Get gn arg to enable WebAPI probes.
_IS_PG_WEBAPI_PROBES_ENABLED = brave_chromium_utils.get_gn_arg(
    "enable_brave_page_graph_webapi_probes")


@override_utils.override_method(IdlCompiler,
                                condition=_IS_PG_WEBAPI_PROBES_ENABLED)
def build_database(self, original_method):
    db = original_method(self)

    # Make sure all dictionaries and unions are marked as outputs, otherwise
    # ToV8() will not be generated.
    dicts = self._db.find_by_kind(DatabaseBody.Kind.DICTIONARY)
    unions = self._db.find_by_kind(DatabaseBody.Kind.UNION)

    for dictionary in dicts.values():
        dictionary._usage |= Dictionary.Usage.OUTPUT
    for union in unions.values():
        union._usage |= UnionType.Usage.OUTPUT

    return db
