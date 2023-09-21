/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"

#include <utility>

namespace brave_ads {

UserModelInfo::UserModelInfo() = default;

UserModelInfo::UserModelInfo(IntentUserModelInfo intent,
                             LatentInterestUserModelInfo latent_interest,
                             InterestUserModelInfo interest)
    : intent(std::move(intent)),
      latent_interest(std::move(latent_interest)),
      interest(std::move(interest)) {}

UserModelInfo::UserModelInfo(const UserModelInfo& other) = default;

UserModelInfo& UserModelInfo::operator=(const UserModelInfo& other) = default;

UserModelInfo::UserModelInfo(UserModelInfo&& other) noexcept = default;

UserModelInfo& UserModelInfo::operator=(UserModelInfo&& other) noexcept =
    default;

UserModelInfo::~UserModelInfo() = default;

bool operator==(const UserModelInfo& lhs, const UserModelInfo& rhs) {
  const auto tie = [](const UserModelInfo& user_model) {
    return std::tie(user_model.intent, user_model.latent_interest,
                    user_model.interest);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const UserModelInfo& lhs, const UserModelInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
