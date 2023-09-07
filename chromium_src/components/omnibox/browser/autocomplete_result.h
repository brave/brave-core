/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_RESULT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_RESULT_H_

#define SortAndCull                                       \
  ReorderMatch(const ACMatches::iterator& it, int index); \
  void RemoveMatch(const ACMatches::iterator& it);        \
  void SortAndCull

#define ConvertOpenTabMatches                                        \
  ConvertOpenTabMatches_Chromium(AutocompleteProviderClient* client, \
                                 const AutocompleteInput* input);    \
  void ConvertOpenTabMatches

#include "src/components/omnibox/browser/autocomplete_result.h"  // IWYU pragma: export

#undef ConvertOpenTabMatches
#undef SortAndCull

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_RESULT_H_
