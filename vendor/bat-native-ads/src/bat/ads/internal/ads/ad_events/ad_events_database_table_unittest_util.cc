/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/ad_events_database_table_unittest_util.h"

#include <utility>

#include "base/check.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/database/database_table_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {
namespace table {
namespace ad_events {

void Reset(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  DeleteTable(transaction.get(), "ad_events");

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      [callback](mojom::DBCommandResponsePtr command_response) {
        if (!command_response ||
            command_response->status !=
                mojom::DBCommandResponse::Status::RESPONSE_OK) {
          DCHECK(false);
          callback(/* success */ false);
          return;
        }

        RebuildAdEventHistoryFromDatabase();

        callback(/* success */ true);
      });
}

}  // namespace ad_events
}  // namespace table
}  // namespace database
}  // namespace ads
