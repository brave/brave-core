/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database {

namespace {

void RunTransactionCallback(ResultCallback callback,
                            mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response) {
    return std::move(callback).Run(/*success=*/false);
  }

  std::move(callback).Run(
      /*success=*/command_response->status ==
      mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK);
}

}  // namespace

void RunTransaction(mojom::DBTransactionInfoPtr transaction,
                    ResultCallback callback) {
  RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&RunTransactionCallback, std::move(callback)));
}

}  // namespace brave_ads::database
