/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_SYNCER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_SYNCER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/jslib_messages_fwd.h"

#define BRAVE_SYNCER_H \
 public: \
  void DownloadBraveRecords(SyncCycle* cycle); \
 private: \
  void OnGetRecords(std::unique_ptr<brave_sync::RecordsList> records); \
  std::unique_ptr<brave_sync::RecordsList> brave_records_;

#include "../../../../../components/sync/engine_impl/syncer.h"
#undef BRAVE_SYNCER_H

#endif    // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_SYNCER_H_
