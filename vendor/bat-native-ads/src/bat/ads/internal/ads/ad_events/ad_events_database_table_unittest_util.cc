/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/ad_events_database_table_unittest_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/database/database_table_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database::table::ad_events {

void Reset(ResultCallback callback) {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(transaction.get(), "ad_events");

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(
          [](ResultCallback callback,
             mojom::DBCommandResponseInfoPtr command_response) {
            if (!command_response ||
                command_response->status !=
                    mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
              std::move(callback).Run(/*success*/ false);
              return;
            }

            RebuildAdEventHistoryFromDatabase();

            std::move(callback).Run(/*success*/ true);
          },
          std::move(callback)));
}

}  // namespace ads::database::table::ad_events
