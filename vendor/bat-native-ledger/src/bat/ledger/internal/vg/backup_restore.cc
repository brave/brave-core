/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <numeric>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/vg/backup_restore.h"

namespace ledger {
namespace vg {

BackupRestore::BackupRestore(LedgerImpl* ledger) : ledger_(ledger) {}

BackupRestore::~BackupRestore() = default;

void BackupRestore::BackUpVgBodies(
    ledger::BackUpVgBodiesCallback callback) const {
  ledger_->database()->BackUpVgBodies(std::move(callback));
}

void BackupRestore::BackUpVgSpendStatuses(
    ledger::BackUpVgSpendStatusesCallback callback) const {
  ledger_->database()->BackUpVgSpendStatuses(std::move(callback));
}

void BackupRestore::RestoreVgs(
    std::vector<sync_pb::VgBodySpecifics> vg_bodies,
    std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses,
    ledger::RestoreVgsCallback callback) const {
  ledger_->database()->RestoreVgs(
      std::move(vg_bodies), std::move(vg_spend_statuses), std::move(callback));
}

}  // namespace vg
}  // namespace ledger
