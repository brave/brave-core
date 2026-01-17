/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SEARCH_SUGGESTION_PARSER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SEARCH_SUGGESTION_PARSER_H_

#define ParseSuggestResults(...)               \
  ParseSuggestResults_Chromium(__VA_ARGS__);   \
  static bool ParseSuggestResults(__VA_ARGS__, \
                                  bool is_brave_rich_suggestion = false)

#include <components/omnibox/browser/search_suggestion_parser.h>  // IWYU pragma: export

#undef ParseSuggestResults

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SEARCH_SUGGESTION_PARSER_H_
