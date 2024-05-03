// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/searchbox/realbox_handler.h"

#include "components/omnibox/browser/autocomplete_input.h"

// We tweak a few AutocompleteInput settings because unlike Chromium we only
// want keyword search results.
#define set_prefer_keyword(prefer)                    \
  set_keyword_mode_entry_method(                      \
      metrics::OmniboxEventProto::KEYBOARD_SHORTCUT); \
  autocomplete_input.set_prefer_keyword(true)
#define set_allow_exact_keyword_match(allow) set_allow_exact_keyword_match(true)

#include "src/chrome/browser/ui/webui/searchbox/realbox_handler.cc"

#undef set_prefer_keyword
#undef set_allow_exact_keyword_match
