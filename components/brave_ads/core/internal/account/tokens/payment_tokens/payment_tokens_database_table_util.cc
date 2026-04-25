/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

PaymentTokenInfo PaymentTokenFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  PaymentTokenInfo payment_token;
  payment_token.transaction_id = ColumnString(mojom_db_row, 0);
  payment_token.unblinded_token =
      cbr::UnblindedToken(ColumnString(mojom_db_row, 1));
  payment_token.public_key = cbr::PublicKey(ColumnString(mojom_db_row, 2));
  payment_token.confirmation_type =
      ToMojomConfirmationType(ColumnString(mojom_db_row, 3));
  payment_token.ad_type = ToMojomAdType(ColumnString(mojom_db_row, 4));
  return payment_token;
}

}  // namespace brave_ads::database::table
