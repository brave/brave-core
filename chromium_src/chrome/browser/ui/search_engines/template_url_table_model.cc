/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/autocomplete_match.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_starter_pack_data.h"

// We disable starter pack expansion to hide @gemini search keyword. Piggy back
// on it to also disable @aimode.
#define kGemini                                 \
  kGemini || template_url->starter_pack_id() == \
                 template_url_starter_pack_data::kAiMode

#include "src/chrome/browser/ui/search_engines/template_url_table_model.cc"
#undef kGemini
