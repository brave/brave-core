/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/search_engine_type.h"

// Add case statements to return the engine type for any Brave-defined starter
// packs.
#define KEYWORD_MODE_STARTER_PACK_AI_MODE               \
  KEYWORD_MODE_STARTER_PACK_AI_MODE;                    \
  case template_url_starter_pack_data::kAskBraveSearch: \
    return KEYWORD_MODE_STARTER_PACK_ASK_BRAVE_SEARCH

#include <components/search_engines/template_url.cc>

#undef KEYWORD_MODE_STARTER_PACK_AI_MODE
