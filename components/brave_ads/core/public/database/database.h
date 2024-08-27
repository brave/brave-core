/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_DATABASE_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_DATABASE_DATABASE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/export.h"
#include "sql/database.h"
#include "sql/meta_table.h"

namespace brave_ads {

class ADS_EXPORT Database final {
 public:
  explicit Database(base::FilePath path);

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  Database(Database&&) noexcept = delete;
  Database& operator=(Database&&) noexcept = delete;

  ~Database();

  mojom::DBTransactionResultInfoPtr RunDBTransaction(
      mojom::DBTransactionInfoPtr mojom_db_transaction);

 private:
  mojom::DBTransactionResultInfo::ResultCode RunDBActions(
      const mojom::DBTransactionInfoPtr& mojom_db_transaction,
      mojom::DBTransactionResultInfo* mojom_db_transaction_result);

  bool ShouldCreateTables();
  mojom::DBTransactionResultInfo::ResultCode Initialize(
      mojom::DBTransactionResultInfo* mojom_db_transaction_result);

  mojom::DBTransactionResultInfo::ResultCode Execute(
      const mojom::DBActionInfo* mojom_db_action);

  mojom::DBTransactionResultInfo::ResultCode RunStatement(
      const mojom::DBActionInfo* mojom_db_action);

  mojom::DBTransactionResultInfo::ResultCode StepStatement(
      const mojom::DBActionInfo* mojom_db_action,
      mojom::DBTransactionResultInfo* mojom_db_transaction_result);

  mojom::DBTransactionResultInfo::ResultCode Migrate();

  mojom::DBTransactionResultInfo::ResultCode MaybeVacuum(
      const mojom::DBTransactionInfo* mojom_db_transaction);

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

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_DATABASE_DATABASE_H_
