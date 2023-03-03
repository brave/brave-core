/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/history/core/browser/sync/brave_typed_url_sync_bridge.h"

#include <memory>
#include <utility>

#include "brave/components/brave_sync/features.h"

namespace history {

// For the case when kBraveSyncSendAllHistory feature is enabled,
// we want to send to the sync server first 20 visits and then each 10th
namespace {
const int kSendAllFlagVisitThrottleThreshold = 20;
const int kSendAllFlagVisitThrottleMultiple = 10;
}  // namespace

BraveTypedURLSyncBridge::BraveTypedURLSyncBridge(
    HistoryBackend* history_backend,
    TypedURLSyncMetadataDatabase* sync_metadata_store,
    std::unique_ptr<syncer::ModelTypeChangeProcessor> change_processor)
    : TypedURLSyncBridge(history_backend,
                         sync_metadata_store,
                         std::move(change_processor)) {}

bool BraveTypedURLSyncBridge::ShouldSyncVisit(const URLRow& url_row,
                                              ui::PageTransition transition) {
  if (base::FeatureList::IsEnabled(
          brave_sync::features::kBraveSyncSendAllHistory)) {
    return url_row.visit_count() < kSendAllFlagVisitThrottleThreshold ||
           (url_row.visit_count() % kSendAllFlagVisitThrottleMultiple) == 0;
  }
  return TypedURLSyncBridge::ShouldSyncVisit(url_row.typed_count(), transition);
}

int BraveTypedURLSyncBridge::GetSendAllFlagVisitThrottleThreshold() {
  return kSendAllFlagVisitThrottleThreshold;
}

int BraveTypedURLSyncBridge::GetSendAllFlagVisitThrottleMultiple() {
  return kSendAllFlagVisitThrottleMultiple;
}

}  // namespace history
