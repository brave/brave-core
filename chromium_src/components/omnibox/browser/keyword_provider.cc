/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_KEYWORD_PROVIDER_FILL_IN_URL_AND_CONTENTS                   \
  if (remaining_input.empty() &&                                          \
      turl->starter_pack_id() ==                                          \
          template_url_starter_pack_data::kAskBraveSearch) {              \
    match->contents.assign(                                               \
        l10n_util::GetStringUTF16(IDS_EMPTY_STARTER_PACK_AI_MODE_VALUE)); \
    match->contents_class.emplace_back(0, ACMatchClassification::DIM);    \
    return;                                                               \
  }

#include <components/omnibox/browser/keyword_provider.cc>

#undef BRAVE_KEYWORD_PROVIDER_FILL_IN_URL_AND_CONTENTS
