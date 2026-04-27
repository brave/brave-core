/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_database_table_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

ConfirmationTokenInfo ConfirmationTokenFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  ConfirmationTokenInfo confirmation_token;
  confirmation_token.unblinded_token =
      cbr::UnblindedToken(ColumnString(mojom_db_row, 0));
  confirmation_token.public_key = cbr::PublicKey(ColumnString(mojom_db_row, 1));
  confirmation_token.signature_base64 = ColumnString(mojom_db_row, 2);
  return confirmation_token;
}

}  // namespace brave_ads::database::table
