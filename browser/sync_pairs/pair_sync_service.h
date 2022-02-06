/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_
#define BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/clock.h"
#include "base/timer/timer.h"
#include "brave/browser/sync_pairs/pair_sync_bridge.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/model/model_type_controller_delegate.h"

class PairSyncService : public KeyedService {
 public:
  explicit PairSyncService(std::unique_ptr<PairSyncBridge> pair_sync_bridge);

  PairSyncService(const PairSyncService&) = delete;
  PairSyncService& operator=(const PairSyncService&) = delete;

  ~PairSyncService() override;

  base::WeakPtr<syncer::ModelTypeControllerDelegate> GetControllerDelegate();

  void Shutdown() override;

 private:
  void AddPair();

  std::unique_ptr<PairSyncBridge> pair_sync_bridge_;
  raw_ptr<base::Clock> clock_;
  base::RepeatingTimer timer_;
};

#endif  // BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_
