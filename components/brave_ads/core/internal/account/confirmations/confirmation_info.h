/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

struct ConfirmationInfo final {
  ConfirmationInfo();

  ConfirmationInfo(const ConfirmationInfo&);
  ConfirmationInfo& operator=(const ConfirmationInfo&);

  ConfirmationInfo(ConfirmationInfo&&) noexcept;
  ConfirmationInfo& operator=(ConfirmationInfo&&) noexcept;

  ~ConfirmationInfo();

  bool operator==(const ConfirmationInfo&) const = default;

  std::string transaction_id;
  std::string creative_instance_id;
  mojom::ConfirmationType type = mojom::ConfirmationType::kUndefined;
  mojom::AdType ad_type = mojom::AdType::kUndefined;
  std::optional<base::Time> created_at;
  std::optional<RewardInfo> reward;
  UserDataInfo user_data;
};

using ConfirmationList = std::vector<ConfirmationInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_
