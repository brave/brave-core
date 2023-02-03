/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_TYPED_URL_SYNC_BRIDGE_H_
#define BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_TYPED_URL_SYNC_BRIDGE_H_

#include <memory>

#include "components/history/core/browser/sync/typed_url_sync_bridge.h"

namespace history {

class BraveTypedURLSyncBridge : public TypedURLSyncBridge {
 public:
  BraveTypedURLSyncBridge(
      HistoryBackend* history_backend,
      TypedURLSyncMetadataDatabase* sync_metadata_store,
      std::unique_ptr<syncer::ModelTypeChangeProcessor> change_processor);

  BraveTypedURLSyncBridge(const BraveTypedURLSyncBridge&) = delete;
  BraveTypedURLSyncBridge& operator=(const BraveTypedURLSyncBridge&) = delete;

  ~BraveTypedURLSyncBridge() override = default;
  bool ShouldSyncVisit(const URLRow& url_row,
                       ui::PageTransition transition) override;

  static int GetSendAllFlagVisitThrottleThreshold();
  static int GetSendAllFlagVisitThrottleMultiple();

 private:
  friend class BraveTypedURLSyncBridgeTest;
};

}  // namespace history

#endif  // BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_TYPED_URL_SYNC_BRIDGE_H_
