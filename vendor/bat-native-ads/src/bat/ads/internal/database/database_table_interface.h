/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_INTERFACE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_INTERFACE_H_

#include <string>

#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::database {

class TableInterface {
 public:
  virtual ~TableInterface() = default;

  virtual std::string GetTableName() const = 0;

  virtual void Migrate(mojom::DBTransactionInfo* transaction,
                       int to_version) = 0;
};

}  // namespace ads::database

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_INTERFACE_H_
