/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_REWARDS_BACKUP_H_
#define BRAVELEDGER_DATABASE_DATABASE_REWARDS_BACKUP_H_

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using BackUpVirtualGrantsCallback =
    base::OnceCallback<void(type::Result, type::VirtualGrants&&)>;

using RestoreVirtualGrantsCallback = base::OnceCallback<void(type::Result)>;

class DatabaseVGBackupRestore : public DatabaseTable {
 public:
  explicit DatabaseVGBackupRestore(LedgerImpl* ledger);

  ~DatabaseVGBackupRestore() override;

  void BackUpVirtualGrants(BackUpVirtualGrantsCallback callback) const;

  void RestoreVirtualGrants(type::VirtualGrants&& vgs,
                            RestoreVirtualGrantsCallback callback) const;

 private:
  using Tables =
      std::map<std::string, std::string>;  // name -> create statement

  using Indices =
      std::map<std::string, std::string>;  // name -> create statement

  void OnBackUpVirtualGrants(BackUpVirtualGrantsCallback callback,
                             type::DBCommandResponsePtr response) const;

  bool AllNULLRecord(const type::DBRecordPtr& record) const;

  void GetCreateTableStatements(Tables&& tables,
                                type::VirtualGrants&& vgs,
                                RestoreVirtualGrantsCallback callback) const;

  void OnGetCreateTableStatements(Tables&& tables,
                                  type::VirtualGrants&& vgs,
                                  RestoreVirtualGrantsCallback callback,
                                  type::DBCommandResponsePtr response) const;

  void GetCreateIndexStatements(Tables&& tables,
                                type::VirtualGrants&& vgs,
                                RestoreVirtualGrantsCallback callback) const;

  void OnGetCreateIndexStatements(Tables&& tables,
                                  const type::VirtualGrants& vgs,
                                  RestoreVirtualGrantsCallback callback,
                                  type::DBCommandResponsePtr response) const;

  void AlterTables(const Tables& tables,
                   type::DBTransaction& transaction) const;

  void DropIndices(const Indices& indices,
                   type::DBTransaction& transaction) const;

  void CreateTables(const Tables& tables,
                    type::DBTransaction& transaction) const;

  void CreateIndices(const Indices& indices,
                     type::DBTransaction& transaction) const;

  void RestoreVirtualGrants(const Tables& tables,
                            const Indices& indices,
                            const type::VirtualGrants& vgs,
                            RestoreVirtualGrantsCallback callback) const;

  void OnRestoreVirtualGrants(RestoreVirtualGrantsCallback callback,
                              type::DBCommandResponsePtr response) const;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_REWARDS_BACKUP_H_
