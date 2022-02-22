/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/vg_sync_service.h"

#include <utility>

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
  DCHECK(vg_body_sync_bridge_);
  return vg_body_sync_bridge_ ? vg_body_sync_bridge_->GetControllerDelegate()
                              : nullptr;
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
VgSyncService::GetControllerDelegateForVgSpendStatuses() {
  DCHECK(vg_spend_status_sync_bridge_);
  return vg_spend_status_sync_bridge_
             ? vg_spend_status_sync_bridge_->GetControllerDelegate()
             : nullptr;
}

void VgSyncService::Shutdown() {}

void VgSyncService::BackUpVgBodies(
    std::vector<sync_pb::VgBodySpecifics> vg_bodies) {
  vg_body_sync_bridge_->BackUpVgBodies(std::move(vg_bodies));
}

void VgSyncService::RestoreVgBodies(
    std::vector<sync_pb::VgBodySpecifics> vg_bodies) {
  if (observer_) {
    if (vg_spend_statuses_) {
      observer_->RestoreVgs(std::move(vg_bodies),
                            std::move(*vg_spend_statuses_));
      vg_spend_statuses_ = absl::nullopt;
    } else {
      DCHECK(!vg_bodies_);
      vg_bodies_ = std::move(vg_bodies);
    }
  }
}

void VgSyncService::BackUpVgSpendStatuses(
    std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses) {
  vg_spend_status_sync_bridge_->BackUpVgSpendStatuses(
      std::move(vg_spend_statuses));
}

void VgSyncService::RestoreVgSpendStatuses(
    std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses) {
  if (observer_) {
    if (vg_bodies_) {
      observer_->RestoreVgs(std::move(*vg_bodies_),
                            std::move(vg_spend_statuses));
      vg_bodies_ = absl::nullopt;
    } else {
      DCHECK(!vg_spend_statuses_);
      vg_spend_statuses_ = std::move(vg_spend_statuses);
    }
  }
}

void VgSyncService::SetObserver(Observer* observer) {
  observer_ = observer;
  vg_body_sync_bridge_->SetObserver(observer_ ? this : nullptr);
  vg_spend_status_sync_bridge_->SetObserver(observer_ ? this : nullptr);
}

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
