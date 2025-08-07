/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/omnibox_field_trial.h"

// We disable starter pack expansion to hide @gemini search keyword. Piggy back
// on it to also disable @aimode.
#define BRAVE_FEATURED_SEARCH_PROVIDER_ADD_FEATURED_KEYWORD_MATCHES         \
  if (turl->starter_pack_id() == template_url_starter_pack_data::kAiMode && \
      !OmniboxFieldTrial::IsStarterPackExpansionEnabled())                  \
    continue;
#include <components/omnibox/browser/featured_search_provider.cc>
#undef BRAVE_FEATURED_SEARCH_PROVIDER_ADD_FEATURED_KEYWORD_MATCHES
