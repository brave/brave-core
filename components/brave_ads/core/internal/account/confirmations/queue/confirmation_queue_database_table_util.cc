/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table_util.h"

#include <string>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

ConfirmationQueueItemInfo ConfirmationQueueItemFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  ConfirmationQueueItemInfo confirmation_queue_item;

  confirmation_queue_item.confirmation.transaction_id =
      ColumnString(mojom_db_row, 0);

  confirmation_queue_item.confirmation.creative_instance_id =
      ColumnString(mojom_db_row, 1);

  confirmation_queue_item.confirmation.type =
      ToMojomConfirmationType(ColumnString(mojom_db_row, 2));

  confirmation_queue_item.confirmation.ad_type =
      ToMojomAdType(ColumnString(mojom_db_row, 3));

  const base::Time created_at = ColumnTime(mojom_db_row, 4);
  if (!created_at.is_null()) {
    confirmation_queue_item.confirmation.created_at = created_at;
  }

  const std::string token = ColumnString(mojom_db_row, 5);
  const std::string blinded_token = ColumnString(mojom_db_row, 6);
  const std::string unblinded_token = ColumnString(mojom_db_row, 7);
  const std::string public_key = ColumnString(mojom_db_row, 8);
  const std::string signature = ColumnString(mojom_db_row, 9);
  const std::string credential_base64url = ColumnString(mojom_db_row, 10);
  if (!token.empty() && !blinded_token.empty() && !unblinded_token.empty() &&
      !public_key.empty() && !signature.empty() &&
      !credential_base64url.empty()) {
    confirmation_queue_item.confirmation.reward = RewardInfo();

    confirmation_queue_item.confirmation.reward->token = cbr::Token(token);

    confirmation_queue_item.confirmation.reward->blinded_token =
        cbr::BlindedToken(blinded_token);

    confirmation_queue_item.confirmation.reward->unblinded_token =
        cbr::UnblindedToken(unblinded_token);

    confirmation_queue_item.confirmation.reward->public_key =
        cbr::PublicKey(public_key);

    confirmation_queue_item.confirmation.reward->signature = signature;

    confirmation_queue_item.confirmation.reward->credential_base64url =
        credential_base64url;
  }

  confirmation_queue_item.confirmation.user_data.fixed =
      base::JSONReader::ReadDict(ColumnString(mojom_db_row, 11),
                                 base::JSON_PARSE_RFC)
          .value_or(base::DictValue());

  const base::Time process_at = ColumnTime(mojom_db_row, 12);
  if (!process_at.is_null()) {
    confirmation_queue_item.process_at = process_at;
  }

  confirmation_queue_item.retry_count = ColumnInt(mojom_db_row, 13);

  return confirmation_queue_item;
}

}  // namespace brave_ads::database::table
