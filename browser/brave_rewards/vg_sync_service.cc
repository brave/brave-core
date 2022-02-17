/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/vg_sync_service.h"

#include <utility>

#include "brave/components/sync/protocol/vg_specifics.pb.h"

// using bat_ledger::mojom::Pair;
// using bat_ledger::mojom::PairPtr;

VgSyncService::VgSyncService(
    std::unique_ptr<VgBodySyncBridge> vg_body_sync_bridge,
    std::unique_ptr<VgSpendStatusSyncBridge> vg_spend_status_sync_bridge)
    : vg_body_sync_bridge_(
          (DCHECK(vg_body_sync_bridge), std::move(vg_body_sync_bridge))),
      vg_spend_status_sync_bridge_((DCHECK(vg_spend_status_sync_bridge),
                                    std::move(vg_spend_status_sync_bridge))) {}

VgSyncService::~VgSyncService() = default;

base::WeakPtr<syncer::ModelTypeControllerDelegate>
VgSyncService::GetControllerDelegateForVgBodies() {
  return vg_body_sync_bridge_ ? vg_body_sync_bridge_->GetControllerDelegate()
                              : nullptr;
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
VgSyncService::GetControllerDelegateForVgSpendStatuses() {
  return vg_spend_status_sync_bridge_
             ? vg_spend_status_sync_bridge_->GetControllerDelegate()
             : nullptr;
}

void VgSyncService::Shutdown() {}

// void VgSyncService::AddPair(std::int64_t key, const std::string& value) {
//   sync_pb::PairSpecifics pair;
//   pair.set_key(key);
//   pair.set_value(value);
//
//   pair_sync_bridge_->AddPair(std::move(pair));
// }

// void VgSyncService::GetPairs(GetPairsCallback callback) {
//   pair_sync_bridge_->GetPairs(base::BindOnce(&VgSyncService::OnGetPairs,
//                                              base::Unretained(this),
//                                              std::move(callback)));
// }

// void VgSyncService::OnGetPairs(
//     GetPairsCallback callback, std::unique_ptr<syncer::DataBatch> data_batch)
//     {
//   std::vector<PairPtr> pairs;
//
//   if (data_batch) {
//     while (data_batch->HasNext()) {
//       auto key_and_data = data_batch->Next();
//       const auto& pair = key_and_data.second->specifics.pair();
//       pairs.emplace_back(Pair::New(pair.key(), pair.value()));
//     }
//   }
//
//   std::move(callback).Run(std::move(pairs));
// }
