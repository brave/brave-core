/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

TransactionInfo TransactionFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  TransactionInfo transaction;

  transaction.id = ColumnString(mojom_db_row, 0);

  const base::Time created_at = ColumnTime(mojom_db_row, 1);
  if (!created_at.is_null()) {
    transaction.created_at = created_at;
  }

  transaction.creative_instance_id = ColumnString(mojom_db_row, 2);
  transaction.value = ColumnDouble(mojom_db_row, 3);
  transaction.segment = ColumnString(mojom_db_row, 4);
  transaction.ad_type = ToMojomAdType(ColumnString(mojom_db_row, 5));
  transaction.confirmation_type =
      ToMojomConfirmationType(ColumnString(mojom_db_row, 6));

  const base::Time reconciled_at = ColumnTime(mojom_db_row, 7);
  if (!reconciled_at.is_null()) {
    transaction.reconciled_at = reconciled_at;
  }

  return transaction;
}

}  // namespace brave_ads::database::table
