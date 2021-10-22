/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BACKUP_BACKUP_H_
#define BRAVELEDGER_BACKUP_BACKUP_H_

#include "base/timer/timer.h"
#include "bat/ledger/mojom_structs.h"

namespace ledger {
class LedgerImpl;

namespace vg {

class BackupRestore {
 public:
  explicit BackupRestore(LedgerImpl* ledger);

  ~BackupRestore();

  void Start();

  void Restore(const std::string& vg_bodies,
               const std::string& vg_spend_statuses,
               ledger::RestoreVirtualGrantsCallback callback) const;

 private:
  void OnRestore(ledger::RestoreVirtualGrantsCallback callback,
                 type::Result result) const;

  void BackUp();

  void OnBackup(type::Result result, type::VirtualGrants&& vgs) const;

  std::string GetVirtualGrantBodies(const type::VirtualGrants& vgs) const;

  std::string GetVirtualGrantSpendStatuses(
      const type::VirtualGrants& vgs) const;

  bool ParseVirtualGrantBodies(const std::string& json,
                               type::VirtualGrants& vgs) const;

  bool ParseVirtualGrantSpendStatuses(const std::string& json,
                                      type::VirtualGrants& vgs) const;

  LedgerImpl* ledger_;  // NOT OWNED
  // base::RepeatingTimer timer_;
  base::OneShotTimer timer_;
};

}  // namespace vg
}  // namespace ledger

#endif  // BRAVELEDGER_BACKUP_BACKUP_H_
