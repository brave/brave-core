/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_UTIL_H_

#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

void DepositWithUserData(mojom::AdType ad_type,
                         mojom::ConfirmationType confirmation_type,
                         const std::string& campaign_id,
                         const std::string& creative_instance_id,
                         const std::string& segment,
                         base::DictValue user_data);

void Deposit(mojom::AdType ad_type,
             mojom::ConfirmationType confirmation_type,
             const std::string& campaign_id,
             const std::string& creative_instance_id,
             const std::string& segment);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_UTIL_H_
