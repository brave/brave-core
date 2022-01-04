/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync_pairs/pair_sync_service.h"

#include <utility>

#include "base/rand_util.h"
#include "base/time/default_clock.h"
#include "brave/components/sync/protocol/pair_specifics.pb.h"

PairSyncService::PairSyncService(
    std::unique_ptr<PairSyncBridge> pair_sync_bridge)
    : pair_sync_bridge_(
          (DCHECK(pair_sync_bridge), std::move(pair_sync_bridge))),
      clock_(base::DefaultClock::GetInstance()) {
  timer_.Start(FROM_HERE, base::Seconds(10), this, &PairSyncService::AddPair);
}

PairSyncService::~PairSyncService() = default;

base::WeakPtr<syncer::ModelTypeControllerDelegate>
PairSyncService::GetControllerDelegate() {
  return pair_sync_bridge_ ? pair_sync_bridge_->GetControllerDelegate()
                           : nullptr;
}

void PairSyncService::Shutdown() {}

void PairSyncService::AddPair() {
  sync_pb::PairSpecifics pair;
  pair.set_key(clock_->Now().since_origin().InMicroseconds());

  std::string value(8, 0);
  for (auto& c : value) {
    c = base::RandInt('a', 'z');
  }
  pair.set_value(std::move(value));

  pair_sync_bridge_->AddPair(std::move(pair));
}
