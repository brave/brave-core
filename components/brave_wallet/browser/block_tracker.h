/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCK_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCK_TRACKER_H_

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

namespace brave_wallet {

class JsonRpcService;

class BlockTracker {
 public:
  explicit BlockTracker(JsonRpcService* json_rpc_service);
  virtual ~BlockTracker() = default;

  virtual void Start(base::TimeDelta interval) = 0;
  virtual void Stop();
  bool IsRunning() const;

 protected:
  base::RepeatingTimer timer_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCK_TRACKER_H_
