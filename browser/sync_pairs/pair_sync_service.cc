/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync_pairs/pair_sync_service.h"

#include <utility>

#include "brave/components/sync/protocol/pair_specifics.pb.h"

using bat_ledger::mojom::Pair;
using bat_ledger::mojom::PairPtr;

PairSyncService::PairSyncService(
    std::unique_ptr<PairSyncBridge> pair_sync_bridge)
    : pair_sync_bridge_(
          (DCHECK(pair_sync_bridge), std::move(pair_sync_bridge))) {}

PairSyncService::~PairSyncService() = default;

base::WeakPtr<syncer::ModelTypeControllerDelegate>
PairSyncService::GetControllerDelegate() {
  return pair_sync_bridge_ ? pair_sync_bridge_->GetControllerDelegate()
                           : nullptr;
}

void PairSyncService::Shutdown() {}

void PairSyncService::AddPair(std::int64_t key, const std::string& value) {
  sync_pb::PairSpecifics pair;
  pair.set_key(key);
  pair.set_value(value);

  pair_sync_bridge_->AddPair(std::move(pair));
}

void PairSyncService::GetPairs(GetPairsCallback callback) {
  pair_sync_bridge_->GetPairs(base::BindOnce(&PairSyncService::OnGetPairs,
                                             base::Unretained(this),
                                             std::move(callback)));
}

void PairSyncService::OnGetPairs(
    GetPairsCallback callback, std::unique_ptr<syncer::DataBatch> data_batch) {
  std::vector<PairPtr> pairs;

  if (data_batch) {
    while (data_batch->HasNext()) {
      auto key_and_data = data_batch->Next();
      const auto& pair = key_and_data.second->specifics.pair();
      pairs.emplace_back(Pair::New(pair.key(), pair.value()));
    }
  }

  std::move(callback).Run(std::move(pairs));
}
