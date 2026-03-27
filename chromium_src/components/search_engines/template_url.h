/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_H_

// Return true for any Brave-specific starter packs that are "ask" oriented,
// rather than search oriented.
#define is_ask_type()                                                     \
  is_ask_type() const {                                                   \
    if (starter_pack_id() ==                                              \
        template_url_starter_pack_data::StarterPackId::kAskBraveSearch) { \
      return true;                                                        \
    }                                                                     \
    return is_ask_type_chromium();                                        \
  }                                                                       \
  bool is_ask_type_chromium()

// Brave specific logic on what URL parameters gets added on any requests sent
// to Google via the browser omnibox.
#define HandleReplacement(...)                       \
  HandleReplacement_ChromiumImpl(__VA_ARGS__) const; \
  void HandleReplacement(__VA_ARGS__)

#include <components/search_engines/template_url.h>  // IWYU pragma: export

#undef HandleReplacement
#undef is_ask_type

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SEARCH_ENGINES_TEMPLATE_URL_H_
