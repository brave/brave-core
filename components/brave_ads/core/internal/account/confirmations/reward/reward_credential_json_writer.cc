/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_credential_json_writer.h"

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/credential_builder.h"

namespace brave_ads::json::writer {

std::optional<std::string> WriteRewardCredential(
    const std::optional<RewardInfo>& reward,
    const std::string& payload) {
  CHECK(!payload.empty());

  if (!reward) {
    return std::nullopt;
  }

  const std::optional<base::Value::Dict> credential =
      cbr::MaybeBuildCredential(reward->unblinded_token, payload);
  if (!credential) {
    return std::nullopt;
  }

  std::string json;
  CHECK(base::JSONWriter::Write(*credential, &json));
  return json;
}

}  // namespace brave_ads::json::writer
