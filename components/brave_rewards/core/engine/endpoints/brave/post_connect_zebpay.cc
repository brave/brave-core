/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/endpoints/brave/post_connect_zebpay.h"

#include <optional>
#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/engine/rewards_engine.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_rewards::internal::endpoints {

PostConnectZebPay::PostConnectZebPay(RewardsEngine& engine,
                                     std::string&& linking_info)
    : PostConnect(engine), linking_info_(std::move(linking_info)) {}

PostConnectZebPay::~PostConnectZebPay() = default;

std::optional<std::string> PostConnectZebPay::Content() const {
  if (linking_info_.empty()) {
    engine_->LogError(FROM_HERE) << "linking_info_ is empty";
    return std::nullopt;
  }

  base::DictValue content;
  content.Set("linking_info", linking_info_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    engine_->LogError(FROM_HERE) << "Failed to write content to JSON";
    return std::nullopt;
  }

  return json;
}

std::string PostConnectZebPay::Path(base::cstring_view payment_id) const {
  return absl::StrFormat("/v3/wallet/zebpay/%s/claim", payment_id);
}

}  // namespace brave_rewards::internal::endpoints
