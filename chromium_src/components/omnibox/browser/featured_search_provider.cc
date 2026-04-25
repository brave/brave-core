/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The disclaimer/link-text IDS swap for this file is handled in
// brave/rewrite/components/omnibox/browser/featured_search_provider.cc.toml;
// brave_components_strings.h below supplies the IDS_BRAVE_* definitions
// referenced after the swap.
#include "components/grit/brave_components_strings.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/search_engines/template_url_starter_pack_data.h"

// Return a relevance value for any Brave-defined starter pack engines.
#define kMaxStarterPackId                                      \
  kAskBraveSearch:                                             \
  return StarterPackRelevance(                                 \
      template_url_starter_pack_data::StarterPackId::kAiMode); \
  case template_url_starter_pack_data::StarterPackId::kMaxStarterPackId

#include <components/omnibox/browser/featured_search_provider.cc>

#undef kMaxStarterPackId
