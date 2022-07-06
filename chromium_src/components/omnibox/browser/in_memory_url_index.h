/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_IN_MEMORY_URL_INDEX_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_IN_MEMORY_URL_INDEX_H_

class BraveHistoryQuickProviderTest;

#define ClearPrivateData                        \
  ClearPrivateDataUnused();                     \
  friend class ::BraveHistoryQuickProviderTest; \
  void ClearPrivateData

#include "src/components/omnibox/browser/in_memory_url_index.h"

#undef ClearPrivateData

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_IN_MEMORY_URL_INDEX_H_
