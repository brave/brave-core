/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_DATABASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_DATABASE_H_

#include <cstdint>
#include <memory>

#include "base/files/file_path.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "bat/ads/export.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"

namespace ads {

class ADS_EXPORT Database {
 public:
  explicit Database(const base::FilePath& path);

  ~Database();

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  void RunTransaction(mojom::DBTransactionPtr transaction,
                      mojom::DBCommandResponse* command_response);

 private:
  mojom::DBCommandResponse::Status Initialize(
      const int32_t version,
      const int32_t compatible_version,
      mojom::DBCommandResponse* command_response);

  mojom::DBCommandResponse::Status Execute(mojom::DBCommand* command);

  mojom::DBCommandResponse::Status Run(mojom::DBCommand* command);

  mojom::DBCommandResponse::Status Read(
      mojom::DBCommand* command,
      mojom::DBCommandResponse* command_response);

  mojom::DBCommandResponse::Status Migrate(const int32_t version,
                                           const int32_t compatible_version);

  void OnErrorCallback(const int error, sql::Statement* statement);

  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  base::FilePath db_path_;
  sql::Database db_;
  sql::MetaTable meta_table_;
  bool is_initialized_ = false;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_DATABASE_H_
