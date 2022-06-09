/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_CLEAR_SERVER_DATA_H_
#define BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_CLEAR_SERVER_DATA_H_

#include "base/callback_forward.h"

namespace syncer {

class SyncCycle;

class BraveSyncServerCommands {
 public:
  BraveSyncServerCommands(const BraveSyncServerCommands&) = delete;
  BraveSyncServerCommands& operator=(const BraveSyncServerCommands&) = delete;

  static void PermanentlyDeleteAccount(SyncCycle* cycle,
                                       base::OnceClosure callback);

 private:
  BraveSyncServerCommands() = default;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_CLEAR_SERVER_DATA_H_
