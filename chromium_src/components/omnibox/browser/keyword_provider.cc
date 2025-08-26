/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/autocomplete_match.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_starter_pack_data.h"

// In `KeywordProvider::FillInUrlAndContents`, when the "@ask" keyword is in
// use, we want the empty query placeholder text to be "question oriented"
// rather than "search oriented". Use the same logic for kAskBraveSearch as
// upstream uses for kAiMode.
#define kAiMode                         \
  kAiMode || turl->starter_pack_id() == \
                 template_url_starter_pack_data::kAskBraveSearch

#include <components/omnibox/browser/keyword_provider.cc>

#undef kAiMode
