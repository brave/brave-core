/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_TABLE_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_TABLE_INTERFACE_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads::database {

class TableInterface {
 public:
  virtual ~TableInterface() = default;

  virtual std::string GetTableName() const = 0;

  virtual void Create(mojom::DBTransactionInfo* mojom_db_transaction) = 0;
  virtual void Migrate(mojom::DBTransactionInfo* mojom_db_transaction,
                       int to_version);
};

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_TABLE_INTERFACE_H_
