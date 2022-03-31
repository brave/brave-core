/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_BLOCK_TRACKER_H_

#include "brave/components/brave_wallet/browser/block_tracker.h"

namespace brave_wallet {

class JsonRpcService;

class FilBlockTracker : public BlockTracker {
 public:
  explicit FilBlockTracker(JsonRpcService* json_rpc_service);
  ~FilBlockTracker() override = default;
  FilBlockTracker(const FilBlockTracker&) = delete;
  FilBlockTracker operator=(const FilBlockTracker&) = delete;

  // If timer is already running, it will be replaced with new interval
  void Start(base::TimeDelta interval) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_BLOCK_TRACKER_H_
