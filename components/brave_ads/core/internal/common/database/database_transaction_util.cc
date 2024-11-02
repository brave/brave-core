/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads::database {

namespace {

void RunDBTransactionCallback(
    ResultCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (IsError(mojom_db_transaction_result)) {
    return std::move(callback).Run(/*success=*/false);
  }

  std::move(callback).Run(/*success=*/true);
}

}  // namespace

bool IsSuccess(
    const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result) {
  return mojom_db_transaction_result &&
         mojom_db_transaction_result->status_code ==
             mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

bool IsError(
    const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result) {
  return !mojom_db_transaction_result ||
         mojom_db_transaction_result->status_code !=
             mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

void RunDBTransaction(mojom::DBTransactionInfoPtr mojom_db_transaction,
                      ResultCallback callback) {
  GetAdsClient().RunDBTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&RunDBTransactionCallback, std::move(callback)));
}

void Raze(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  mojom_db_transaction->should_raze = true;
}

void Execute(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
             const std::string& sql) {
  CHECK(mojom_db_transaction);

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecute;
  mojom_db_action->sql = sql;
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void Execute(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
             const std::string& sql,
             const std::vector<std::string>& subst) {
  CHECK(mojom_db_transaction);

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecute;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(sql, subst, nullptr);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void Vacuum(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  mojom_db_transaction->should_vacuum = true;
}

}  // namespace brave_ads::database
