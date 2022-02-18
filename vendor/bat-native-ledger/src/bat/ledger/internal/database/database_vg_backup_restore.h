/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_REWARDS_BACKUP_H_
#define BRAVELEDGER_DATABASE_DATABASE_REWARDS_BACKUP_H_

#include "bat/ledger/internal/database/database_table.h"
#include "brave/components/sync/protocol/vg_specifics.pb.h"

namespace ledger {
namespace database {

using BackUpVgBodiesCallback =
    base::OnceCallback<void(type::Result,
                            std::vector<sync_pb::VgBodySpecifics>)>;

using BackUpVgSpendStatusesCallback =
    base::OnceCallback<void(type::Result,
                            std::vector<sync_pb::VgSpendStatusSpecifics>)>;

using RestoreVgsCallback = base::OnceCallback<void(type::Result)>;

class DatabaseVGBackupRestore : public DatabaseTable {
 public:
  explicit DatabaseVGBackupRestore(LedgerImpl* ledger);

  ~DatabaseVGBackupRestore() override;

  void BackUpVgBodies(BackUpVgBodiesCallback callback) const;

  void BackUpVgSpendStatuses(BackUpVgSpendStatusesCallback callback) const;

  void RestoreVgs(
      std::vector<sync_pb::VgBodySpecifics> vg_bodies,
      std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses,
      RestoreVgsCallback callback) const;

 private:
  using Tables =
      std::map<std::string, std::string>;  // name -> create statement

  using Indices =
      std::map<std::string, std::string>;  // name -> create statement

  void OnBackUpVgBodies(BackUpVgBodiesCallback callback,
                        type::DBCommandResponsePtr response) const;

  void OnBackUpVgSpendStatuses(BackUpVgSpendStatusesCallback callback,
                               type::DBCommandResponsePtr response) const;

  bool AllNULLRecord(const type::DBRecordPtr& record) const;

  void GetCreateTableStatements(
      Tables&& tables,
      std::vector<sync_pb::VgBodySpecifics> vg_bodies,
      std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses,
      RestoreVgsCallback callback) const;

  void OnGetCreateTableStatements(
      Tables&& tables,
      std::vector<sync_pb::VgBodySpecifics> vg_bodies,
      std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses,
      RestoreVgsCallback callback,
      type::DBCommandResponsePtr response) const;

  void GetCreateIndexStatements(
      Tables&& tables,
      std::vector<sync_pb::VgBodySpecifics> vg_bodies,
      std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses,
      RestoreVgsCallback callback) const;

  void OnGetCreateIndexStatements(
      Tables&& tables,
      std::vector<sync_pb::VgBodySpecifics> vg_bodies,
      std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses,
      RestoreVgsCallback callback,
      type::DBCommandResponsePtr response) const;

  void AlterTables(const Tables& tables,
                   type::DBTransaction& transaction) const;

  void DropIndices(const Indices& indices,
                   type::DBTransaction& transaction) const;

  void CreateTables(const Tables& tables,
                    type::DBTransaction& transaction) const;

  void CreateIndices(const Indices& indices,
                     type::DBTransaction& transaction) const;

  void RestoreVgs(
      const Tables& tables,
      const Indices& indices,
      std::vector<sync_pb::VgBodySpecifics> vg_bodies,
      std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses,
      RestoreVgsCallback callback) const;

  void OnRestoreVgs(RestoreVgsCallback callback,
                    type::DBCommandResponsePtr response) const;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_REWARDS_BACKUP_H_
