/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_MATCHES_STORAGE_KEY                                   \
  if (match_mode == OriginMatchingMode::kThirdPartiesOnly &&        \
      !storage_key.IsFirstPartyContext() &&                         \
      storage_key.top_level_site() == net::SchemefulSite(origin)) { \
    return is_delete_list;                                          \
  }

#include "src/content/browser/browsing_data/browsing_data_filter_builder_impl.cc"

#undef BRAVE_MATCHES_STORAGE_KEY
