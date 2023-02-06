/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_TYPED_URL_SYNC_BRIDGE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_TYPED_URL_SYNC_BRIDGE_H_

#define ShouldSyncVisit                                            \
  ShouldSyncVisitUnused();                                         \
  static bool HasTypedUrl(const std::vector<VisitRow>& visits);    \
  virtual bool ShouldSyncVisit(const URLRow& url_row,              \
                               ui::PageTransition transition) = 0; \
  friend class BraveTypedURLSyncBridge;                            \
  friend class BraveTypedURLSyncBridgeTest;                        \
  bool ShouldSyncVisit

#include "src/components/history/core/browser/sync/typed_url_sync_bridge.h"

#undef ShouldSyncVisit

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_TYPED_URL_SYNC_BRIDGE_H_
