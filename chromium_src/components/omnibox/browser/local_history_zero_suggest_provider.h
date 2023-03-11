// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_LOCAL_HISTORY_ZERO_SUGGEST_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_LOCAL_HISTORY_ZERO_SUGGEST_PROVIDER_H_

#define QueryURLDatabase                             \
  QueryURLDatabase_Unused();                         \
  friend class BraveLocalHistoryZeroSuggestProvider; \
  void QueryURLDatabase

#include "src/components/omnibox/browser/local_history_zero_suggest_provider.h"  // IWYU pragma: export

#undef QueryURLDatabase

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_LOCAL_HISTORY_ZERO_SUGGEST_PROVIDER_H_
