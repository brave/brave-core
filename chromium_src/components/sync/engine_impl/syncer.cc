/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/components/sync/engine_impl/syncer.h"

#include <memory>
#include <utility>

#include "base/bind.h"

#include "../../../../../components/sync/engine_impl/syncer.cc"  // NOLINT

namespace syncer {

using brave_sync::GetRecordsCallback;
using brave_sync::RecordsList;

void Syncer::OnGetRecords(std::unique_ptr<RecordsList> records) {
  brave_records_ = std::move(records);
}

void Syncer::DownloadBraveRecords(SyncCycle* cycle) {
  // syncer will be alive as long as sync is enabled
  brave_records_.reset();
  brave_sync::GetRecordsCallback on_get_records =
      base::BindOnce(&Syncer::OnGetRecords, base::Unretained(this));
  base::WaitableEvent wevent;
  cycle->delegate()->OnPollSyncCycle(std::move(on_get_records), &wevent);
  // Make sure OnGetRecords will be the next task on sync thread
  wevent.Wait();
}

}  // namespace syncer
