/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"

namespace brave_ads::database {

void TableInterface::Migrate(mojom::DBTransactionInfo* mojom_db_transaction,
                             const int to_version) {
  CHECK(mojom_db_transaction);

  if (to_version == kVersionNumber) {
    Create(mojom_db_transaction);
  }
}

}  // namespace brave_ads::database
