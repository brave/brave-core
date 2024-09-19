/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_gemini.h"

#include <optional>
#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::endpoints {

PostConnectGemini::PostConnectGemini(RewardsEngine& engine,
                                     std::string&& linking_info,
                                     std::string&& recipient_id)
    : PostConnect(engine),
      linking_info_(std::move(linking_info)),
      recipient_id_(std::move(recipient_id)) {}

PostConnectGemini::~PostConnectGemini() = default;

std::optional<std::string> PostConnectGemini::Content() const {
  if (linking_info_.empty()) {
    engine_->LogError(FROM_HERE) << "linking_info_ is empty";
    return std::nullopt;
  }

  if (recipient_id_.empty()) {
    engine_->LogError(FROM_HERE) << "recipient_id_ is empty";
    return std::nullopt;
  }

  base::Value::Dict content;
  content.Set("linking_info", linking_info_);
  content.Set("recipient_id", recipient_id_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    engine_->LogError(FROM_HERE) << "Failed to write content to JSON";
    return std::nullopt;
  }

  return json;
}

std::string PostConnectGemini::Path(base::cstring_view payment_id) const {
  return base::StringPrintf("/v3/wallet/gemini/%s/claim", payment_id.c_str());
}

}  // namespace brave_rewards::internal::endpoints
