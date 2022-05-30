/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_VG_BACKUP_RESTORE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_VG_BACKUP_RESTORE_H_

#include <vector>

#include "bat/ledger/mojom_structs.h"

namespace ledger {
class LedgerImpl;

namespace vg {

class BackupRestore {
 public:
  explicit BackupRestore(LedgerImpl* ledger);

  ~BackupRestore();

  void BackUpVgBodies(ledger::BackUpVgBodiesCallback callback) const;

  void BackUpVgSpendStatuses(
      ledger::BackUpVgSpendStatusesCallback callback) const;

  void RestoreVgs(
      std::vector<sync_pb::VgBodySpecifics> vg_bodies,
      std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses,
      ledger::RestoreVgsCallback callback) const;

 private:
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace vg
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_VG_BACKUP_RESTORE_H_
