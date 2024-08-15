/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SEARCH_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SEARCH_PROVIDER_H_

#define DoHistoryQuery                         \
  DoHistoryQueryUnused();                      \
  friend class BraveSearchProvider;            \
  friend class BraveSearchProviderTest;        \
  bool IsBraveRichSuggestion(bool is_keyword); \
  virtual void DoHistoryQuery

#define IsQueryPotentiallyPrivate virtual IsQueryPotentiallyPrivate

#include "src/components/omnibox/browser/search_provider.h"  // IWYU pragma: export

#undef IsQueryPotentiallyPrivate
#undef DoHistoryQuery

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SEARCH_PROVIDER_H_
