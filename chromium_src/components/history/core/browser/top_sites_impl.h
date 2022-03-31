/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_IMPL_H_

// Needs 12 items for our NTP top site tiles.
#define kTopSitesNumber \
  kTopSitesNumber = 12; \
  static constexpr size_t kTopSitesNumber_Unused

#include "src/components/history/core/browser/top_sites_impl.h"

#undef kTopSitesNumber

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_IMPL_H_
