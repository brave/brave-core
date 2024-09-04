/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/payload/confirmation_payload_json_writer.h"

#include <utility>

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/payload/reward_confirmation_payload_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads::json::writer {

namespace {

constexpr char kTransactionIdKey[] = "transactionId";
constexpr char kCreativeInstanceIdKey[] = "creativeInstanceId";
constexpr char kTypeKey[] = "type";

}  // namespace

std::string WriteConfirmationPayload(const ConfirmationInfo& confirmation) {
  auto dict =
      base::Value::Dict()
          .Set(kTransactionIdKey, confirmation.transaction_id)
          .Set(kCreativeInstanceIdKey, confirmation.creative_instance_id)
          .Set(kTypeKey, ToString(confirmation.type));

  if (confirmation.reward) {
    base::Value::Dict reward_confirmation_payload_dict =
        BuildRewardConfirmationPayload(*confirmation.reward);
    dict.Merge(std::move(reward_confirmation_payload_dict));
  }

  dict.Merge(confirmation.user_data.dynamic.Clone());
  dict.Merge(confirmation.user_data.fixed.Clone());

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

}  // namespace brave_ads::json::writer
