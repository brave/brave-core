/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_
#define BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/sync_pairs/pair_sync_bridge.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/model_type_controller_delegate.h"

class PairSyncService : public KeyedService {
 public:
  using GetPairsCallback = base::OnceCallback<void(
      std::vector<std::pair<std::int64_t, std::string>>)>;

  explicit PairSyncService(std::unique_ptr<PairSyncBridge> pair_sync_bridge);

  PairSyncService(const PairSyncService&) = delete;
  PairSyncService& operator=(const PairSyncService&) = delete;

  ~PairSyncService() override;

  base::WeakPtr<syncer::ModelTypeControllerDelegate> GetControllerDelegate();

  void Shutdown() override;

  void AddPair(std::int64_t key, const std::string& value);

  void GetPairs(GetPairsCallback callback);

 private:
  void OnGetPairs(GetPairsCallback callback,
                  std::unique_ptr<syncer::DataBatch> data_batch);

  std::unique_ptr<PairSyncBridge> pair_sync_bridge_;
};

#endif  // BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_
