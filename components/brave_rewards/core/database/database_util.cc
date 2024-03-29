/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_util.h"

namespace brave_rewards::internal::database {

void BindNull(mojom::DBCommand* command, const int index) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewNullValue(0);
  command->bindings.push_back(std::move(binding));
}

void BindInt(mojom::DBCommand* command, const int index, const int32_t value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewIntValue(value);
  command->bindings.push_back(std::move(binding));
}

void BindInt64(mojom::DBCommand* command,
               const int index,
               const int64_t value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewInt64Value(value);
  command->bindings.push_back(std::move(binding));
}

void BindDouble(mojom::DBCommand* command,
                const int index,
                const double value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewDoubleValue(value);
  command->bindings.push_back(std::move(binding));
}

void BindBool(mojom::DBCommand* command, const int index, const bool value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewBoolValue(value);
  command->bindings.push_back(std::move(binding));
}

void BindString(mojom::DBCommand* command,
                const int index,
                const std::string& value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewStringValue(value);
  command->bindings.push_back(std::move(binding));
}

void OnResultCallback(ResultCallback callback,
                      mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::move(callback).Run(mojom::Result::OK);
}

int GetIntColumn(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (!record->fields.at(index)->is_int_value()) {
    return 0;
  }

  return record->fields.at(index)->get_int_value();
}

int64_t GetInt64Column(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (!record->fields.at(index)->is_int64_value()) {
    return 0;
  }

  return record->fields.at(index)->get_int64_value();
}

double GetDoubleColumn(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0.0;
  }

  if (!record->fields.at(index)->is_double_value()) {
    return 0.0;
  }

  return record->fields.at(index)->get_double_value();
}

bool GetBoolColumn(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return false;
  }

  if (!record->fields.at(index)->is_bool_value()) {
    return false;
  }

  return record->fields.at(index)->get_bool_value();
}

std::string GetStringColumn(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return "";
  }

  if (!record->fields.at(index)->is_string_value()) {
    return "";
  }

  return record->fields.at(index)->get_string_value();
}

std::string GenerateStringInCase(const std::vector<std::string>& items) {
  if (items.empty()) {
    return "";
  }

  const std::string items_join = base::JoinString(items, "', '");

  return base::StringPrintf("'%s'", items_join.c_str());
}

mojom::PublisherStatus PublisherStatusFromInt(int value) {
  auto unsafe_value = static_cast<mojom::PublisherStatus>(value);
  switch (unsafe_value) {
    case mojom::PublisherStatus::NOT_VERIFIED:
    case mojom::PublisherStatus::UPHOLD_VERIFIED:
    case mojom::PublisherStatus::BITFLYER_VERIFIED:
    case mojom::PublisherStatus::GEMINI_VERIFIED:
    case mojom::PublisherStatus::WEB3_ENABLED:
      return unsafe_value;
  }
  return mojom::PublisherStatus::NOT_VERIFIED;
}

mojom::PublisherExclude PublisherExcludeFromInt(int value) {
  auto unsafe_value = static_cast<mojom::PublisherExclude>(value);
  switch (unsafe_value) {
    case mojom::PublisherExclude::ALL:
    case mojom::PublisherExclude::DEFAULT:
    case mojom::PublisherExclude::EXCLUDED:
    case mojom::PublisherExclude::INCLUDED:
      return unsafe_value;
  }
  return mojom::PublisherExclude::INCLUDED;
}

mojom::RewardsType RewardsTypeFromInt(int value) {
  auto unsafe_value = static_cast<mojom::RewardsType>(value);
  switch (unsafe_value) {
    case mojom::RewardsType::AUTO_CONTRIBUTE:
    case mojom::RewardsType::ONE_TIME_TIP:
    case mojom::RewardsType::RECURRING_TIP:
    case mojom::RewardsType::TRANSFER:
    case mojom::RewardsType::PAYMENT:
      return unsafe_value;
  }
  return mojom::RewardsType::ONE_TIME_TIP;
}

mojom::ContributionStep ContributionStepFromInt(int value) {
  auto unsafe_value = static_cast<mojom::ContributionStep>(value);
  switch (unsafe_value) {
    case mojom::ContributionStep::STEP_RETRY_COUNT:
    case mojom::ContributionStep::STEP_AC_OFF:
    case mojom::ContributionStep::STEP_REWARDS_OFF:
    case mojom::ContributionStep::STEP_AC_TABLE_EMPTY:
    case mojom::ContributionStep::STEP_NOT_ENOUGH_FUNDS:
    case mojom::ContributionStep::STEP_FAILED:
    case mojom::ContributionStep::STEP_COMPLETED:
    case mojom::ContributionStep::STEP_NO:
    case mojom::ContributionStep::STEP_START:
    case mojom::ContributionStep::STEP_PREPARE:
    case mojom::ContributionStep::STEP_RESERVE:
    case mojom::ContributionStep::STEP_EXTERNAL_TRANSACTION:
    case mojom::ContributionStep::STEP_CREDS:
      return unsafe_value;
  }
  return mojom::ContributionStep::STEP_FAILED;
}

mojom::ContributionProcessor ContributionProcessorFromInt(int value) {
  auto unsafe_value = static_cast<mojom::ContributionProcessor>(value);
  switch (unsafe_value) {
    case mojom::ContributionProcessor::NONE:
    case mojom::ContributionProcessor::BRAVE_TOKENS:
    case mojom::ContributionProcessor::UPHOLD:
    case mojom::ContributionProcessor::BITFLYER:
    case mojom::ContributionProcessor::GEMINI:
      return unsafe_value;
  }
  return mojom::ContributionProcessor::NONE;
}

mojom::CredsBatchType CredsBatchTypeFromInt(int value) {
  auto unsafe_value = static_cast<mojom::CredsBatchType>(value);
  switch (unsafe_value) {
    case mojom::CredsBatchType::NONE:
    case mojom::CredsBatchType::SKU:
      return unsafe_value;
  }
  return mojom::CredsBatchType::NONE;
}

mojom::CredsBatchStatus CredsBatchStatusFromInt(int value) {
  auto unsafe_value = static_cast<mojom::CredsBatchStatus>(value);
  switch (unsafe_value) {
    case mojom::CredsBatchStatus::NONE:
    case mojom::CredsBatchStatus::BLINDED:
    case mojom::CredsBatchStatus::CLAIMED:
    case mojom::CredsBatchStatus::SIGNED:
    case mojom::CredsBatchStatus::FINISHED:
    case mojom::CredsBatchStatus::CORRUPTED:
      return unsafe_value;
  }
  return mojom::CredsBatchStatus::NONE;
}

mojom::SKUOrderStatus SKUOrderStatusFromInt(int value) {
  auto unsafe_value = static_cast<mojom::SKUOrderStatus>(value);
  switch (unsafe_value) {
    case mojom::SKUOrderStatus::NONE:
    case mojom::SKUOrderStatus::PENDING:
    case mojom::SKUOrderStatus::PAID:
    case mojom::SKUOrderStatus::FULFILLED:
    case mojom::SKUOrderStatus::CANCELED:
      return unsafe_value;
  }
  return mojom::SKUOrderStatus::NONE;
}

mojom::SKUOrderItemType SKUOrderItemTypeFromInt(int value) {
  auto unsafe_value = static_cast<mojom::SKUOrderItemType>(value);
  switch (unsafe_value) {
    case mojom::SKUOrderItemType::NONE:
    case mojom::SKUOrderItemType::SINGLE_USE:
      return unsafe_value;
  }
  return mojom::SKUOrderItemType::NONE;
}

mojom::SKUTransactionStatus SKUTransactionStatusFromInt(int value) {
  auto unsafe_value = static_cast<mojom::SKUTransactionStatus>(value);
  switch (unsafe_value) {
    case mojom::SKUTransactionStatus::NONE:
    case mojom::SKUTransactionStatus::CREATED:
    case mojom::SKUTransactionStatus::COMPLETED:
      return unsafe_value;
  }
  return mojom::SKUTransactionStatus::NONE;
}

mojom::SKUTransactionType SKUTransactionTypeFromInt(int value) {
  auto unsafe_value = static_cast<mojom::SKUTransactionType>(value);
  switch (unsafe_value) {
    case mojom::SKUTransactionType::NONE:
    case mojom::SKUTransactionType::UPHOLD:
    case mojom::SKUTransactionType::TOKENS:
    case mojom::SKUTransactionType::GEMINI:
      return unsafe_value;
  }
  return mojom::SKUTransactionType::NONE;
}

}  // namespace brave_rewards::internal::database
