/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"

#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_id_helper.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads::database {

namespace {

void RunTransactionCallback(
    RunDBTransactionCallback callback,
    uint64_t trace_id,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (mojom_db_transaction_result) {
    if (mojom_db_transaction_result->rows_union) {
      TRACE_EVENT_NESTABLE_ASYNC_END2(
          kTraceEventCategory, "DatabaseTransactionUtil::RunTransaction",
          TRACE_ID_WITH_SCOPE("DatabaseTransactionUtil", trace_id),
          "statusCode", mojom_db_transaction_result->status_code, "rowCount",
          mojom_db_transaction_result->rows_union->get_rows().size());
    } else {
      TRACE_EVENT_NESTABLE_ASYNC_END1(
          kTraceEventCategory, "DatabaseTransactionUtil::RunTransaction",
          TRACE_ID_WITH_SCOPE("DatabaseTransactionUtil", trace_id),
          "statusCode", mojom_db_transaction_result->status_code);
    }
  } else {
    TRACE_EVENT_NESTABLE_ASYNC_END1(
        kTraceEventCategory, "DatabaseTransactionUtil::RunTransaction",
        TRACE_ID_WITH_SCOPE("DatabaseTransactionUtil", trace_id),
        "mojom_db_transaction_result", "nullptr");
  }

  std::move(callback).Run(std::move(mojom_db_transaction_result));
}

void RunTransactionForSuccessOrFailureCallback(
    ResultCallback callback,
    uint64_t trace_id,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (mojom_db_transaction_result) {
    if (mojom_db_transaction_result->rows_union) {
      TRACE_EVENT_NESTABLE_ASYNC_END2(
          kTraceEventCategory, "DatabaseTransactionUtil::RunTransaction",
          TRACE_ID_WITH_SCOPE("DatabaseTransactionUtil", trace_id),
          "statusCode", mojom_db_transaction_result->status_code, "rowCount",
          mojom_db_transaction_result->rows_union->get_rows().size());
    } else {
      TRACE_EVENT_NESTABLE_ASYNC_END1(
          kTraceEventCategory, "DatabaseTransactionUtil::RunTransaction",
          TRACE_ID_WITH_SCOPE("DatabaseTransactionUtil", trace_id),
          "statusCode", mojom_db_transaction_result->status_code);
    }
  } else {
    TRACE_EVENT_NESTABLE_ASYNC_END1(
        kTraceEventCategory, "DatabaseTransactionUtil::RunTransaction",
        TRACE_ID_WITH_SCOPE("DatabaseTransactionUtil", trace_id),
        "mojom_db_transaction_result", "nullptr");
  }

  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    return std::move(callback).Run(/*success=*/false);
  }

  std::move(callback).Run(/*success=*/true);
}

}  // namespace

bool IsTransactionSuccessful(
    const mojom::DBTransactionResultInfoPtr& mojom_db_transaction_result) {
  return mojom_db_transaction_result &&
         mojom_db_transaction_result->status_code ==
             mojom::DBTransactionResultInfo::StatusCode::kSuccess;
}

void RunTransaction(const base::Location& location,
                    mojom::DBTransactionInfoPtr mojom_db_transaction,
                    RunDBTransactionCallback callback) {
  const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(
      kTraceEventCategory, "DatabaseTransactionUtil::RunTransaction",
      TRACE_ID_WITH_SCOPE("DatabaseTransactionUtil", trace_id), "location",
      location.ToString());

  GlobalState::GetInstance()->GetDatabaseManager().RunTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&RunTransactionCallback, std::move(callback), trace_id),
      trace_id);
}

void RunTransaction(const base::Location& location,
                    mojom::DBTransactionInfoPtr mojom_db_transaction,
                    ResultCallback callback) {
  const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1(
      kTraceEventCategory, "DatabaseTransactionUtil::RunTransaction",
      TRACE_ID_WITH_SCOPE("DatabaseTransactionUtil", trace_id), "location",
      location.ToString());

  GlobalState::GetInstance()->GetDatabaseManager().RunTransaction(
      std::move(mojom_db_transaction),
      base::BindOnce(&RunTransactionForSuccessOrFailureCallback,
                     std::move(callback), trace_id),
      trace_id);
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
