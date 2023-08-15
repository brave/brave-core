/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_bitflyer.h"

#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/logging/logging.h"

namespace brave_rewards::internal::endpoints {

PostConnectBitflyer::PostConnectBitflyer(RewardsEngineImpl& engine,
                                         std::string&& linking_info)
    : PostConnect(engine), linking_info_(std::move(linking_info)) {}

PostConnectBitflyer::~PostConnectBitflyer() = default;

absl::optional<std::string> PostConnectBitflyer::Content() const {
  if (linking_info_.empty()) {
    BLOG(0, "linking_info_ is empty!");
    return absl::nullopt;
  }

  base::Value::Dict content;
  content.Set("linkingInfo", linking_info_);

  std::string json;
  if (!base::JSONWriter::Write(content, &json)) {
    BLOG(0, "Failed to write content to JSON!");
    return absl::nullopt;
  }

  return json;
}

const char* PostConnectBitflyer::Path() const {
  return "/v3/wallet/bitflyer/%s/claim";
}

}  // namespace brave_rewards::internal::endpoints
