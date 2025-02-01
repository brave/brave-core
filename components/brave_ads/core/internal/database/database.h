/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_H_

#include <cstdint>
#include <memory>

#include "base/files/file_path.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "sql/database.h"
#include "sql/meta_table.h"

namespace brave_ads {

class Database final {
 public:
  explicit Database(base::FilePath path);

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  ~Database();

  mojom::DBTransactionResultInfoPtr RunDBTransaction(
      mojom::DBTransactionInfoPtr mojom_db_transaction,
      uint64_t trace_id);

 private:
  mojom::DBTransactionResultInfo::StatusCode RunDBActions(
      const mojom::DBTransactionInfoPtr& mojom_db_transaction,
      const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result,
      uint64_t trace_id);

  mojom::DBTransactionResultInfo::StatusCode MaybeRaze(
      const mojom::DBTransactionInfoPtr& mojom_db_transaction,
      uint64_t trace_id);

  bool InitializeMetaTable();

  bool ShouldCreateTables();
  mojom::DBTransactionResultInfo::StatusCode Initialize(
      const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result);

  mojom::DBTransactionResultInfo::StatusCode Execute(
      const mojom::DBActionInfoPtr& mojom_db_action);
  mojom::DBTransactionResultInfo::StatusCode ExecuteWithBindings(
      const mojom::DBActionInfoPtr& mojom_db_action);
  mojom::DBTransactionResultInfo::StatusCode ExecuteQueryWithBindings(
      const mojom::DBActionInfoPtr& mojom_db_action,
      const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result);

  mojom::DBTransactionResultInfo::StatusCode Migrate();

  mojom::DBTransactionResultInfo::StatusCode MaybeVacuum(
      const mojom::DBTransactionInfoPtr& mojom_db_transaction,
      uint64_t trace_id);

  void ErrorCallback(int error, sql::Statement* statement);

  void MemoryPressureListenerCallback(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  base::FilePath db_path_;
  sql::Database db_;
  sql::MetaTable meta_table_;

  bool is_initialized_ = false;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<Database> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_H_
