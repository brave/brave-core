/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_START \
  localized_strings.try_emplace( \
      IDS_IMPORT_FROM_BRAVE, \
      l10n_util::GetStringUTF8(IDS_IMPORT_FROM_BRAVE)); \
  localized_strings.try_emplace( \
      IDS_BOOKMARK_GROUP_FROM_BRAVE, \
      l10n_util::GetStringUTF8(IDS_BOOKMARK_GROUP_FROM_BRAVE));
#include "../../../../../chrome/browser/importer/external_process_importer_client.cc"
#undef BRAVE_START

