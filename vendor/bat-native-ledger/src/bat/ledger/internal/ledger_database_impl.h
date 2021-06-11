/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_DATABASE_IMPL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_DATABASE_IMPL_H_

#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "bat/ledger/ledger_database.h"
#include "sql/database.h"
#include "sql/meta_table.h"

namespace ledger {

class LedgerDatabaseImpl : public mojom::LedgerDatabase {
 public:
  // Creates an instance of |LedgerDatabaseImpl| on the specified task runner
  // that will process messages from the provided mojo receiver.
  static void CreateOnTaskRunner(
      const base::FilePath& file_path,
      mojo::PendingReceiver<mojom::LedgerDatabase> receiver,
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  explicit LedgerDatabaseImpl(const base::FilePath& path);

  LedgerDatabaseImpl(const LedgerDatabaseImpl&) = delete;
  LedgerDatabaseImpl& operator=(const LedgerDatabaseImpl&) = delete;

  ~LedgerDatabaseImpl() override;

  void RunDBTransaction(mojom::DBTransactionPtr transaction,
                        RunDBTransactionCallback callback) override;

  void DeleteFile(DeleteFileCallback callback) override;

  sql::Database* GetInternalDatabaseForTesting() { return &db_; }

 private:
  void CloseDatabase();

  void RunTransactionInternal(mojom::DBTransactionPtr transaction,
                              mojom::DBCommandResponse* command_response);

  mojom::DBCommandResponse::Status Initialize(
      int32_t version,
      int32_t compatible_version,
      mojom::DBCommandResponse* command_response);

  mojom::DBCommandResponse::Status Execute(mojom::DBCommand* command);

  mojom::DBCommandResponse::Status Run(mojom::DBCommand* command);

  mojom::DBCommandResponse::Status Read(
      mojom::DBCommand* command,
      mojom::DBCommandResponse* command_response);

  mojom::DBCommandResponse::Status Migrate(int32_t version,
                                           int32_t compatible_version);

  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  const base::FilePath db_path_;
  sql::Database db_;
  sql::MetaTable meta_table_;
  bool initialized_ = false;
  base::MemoryPressureListener memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_DATABASE_IMPL_H_
