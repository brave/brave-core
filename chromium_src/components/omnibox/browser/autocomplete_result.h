/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_RESULT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_RESULT_H_

#include "base/ranges/algorithm.h"

#define SortAndCull                                           \
  Unused();                                                   \
  void RemoveAllMatchesNotOfType(AutocompleteProvider::Type); \
  template <typename UnaryPredicate>                          \
  void MoveMatchToBeLast(UnaryPredicate predicate) {          \
    if (auto it = base::ranges::find_if(matches_, predicate); \
        it != matches_.end()) {                               \
      std::rotate(it, std::next(it), matches_.end());         \
    }                                                         \
  }                                                           \
  void SortAndCull

#define ConvertOpenTabMatches                                        \
  ConvertOpenTabMatches_Chromium(AutocompleteProviderClient* client, \
                                 const AutocompleteInput* input);    \
  void ConvertOpenTabMatches

#include "src/components/omnibox/browser/autocomplete_result.h"  // IWYU pragma: export

#undef ConvertOpenTabMatches
#undef SortAndCull

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_RESULT_H_
