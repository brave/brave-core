/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_H_

// Return true for any Brave-specific starter packs that are "ask" oriented,
// rather than search oriented.
#define BRAVE_TEMPLATE_URL_IS_ASK_STARTER_PACK                                \
  if (starter_pack_id() == template_url_starter_pack_data::kAskBraveSearch) { \
    return true;                                                              \
  }

#include <components/search_engines/template_url.h>  // IWYU pragma: export

#undef BRAVE_TEMPLATE_URL_IS_ASK_STARTER_PACK

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_H_
