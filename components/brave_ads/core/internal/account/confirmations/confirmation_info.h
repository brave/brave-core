/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct ConfirmationInfo final {
  ConfirmationInfo();

  ConfirmationInfo(const ConfirmationInfo&);
  ConfirmationInfo& operator=(const ConfirmationInfo&);

  ConfirmationInfo(ConfirmationInfo&&) noexcept;
  ConfirmationInfo& operator=(ConfirmationInfo&&) noexcept;

  ~ConfirmationInfo();

  std::string transaction_id;
  std::string creative_instance_id;
  ConfirmationType type = ConfirmationType::kUndefined;
  AdType ad_type = AdType::kUndefined;
  base::Time created_at;
  bool was_created = false;
  absl::optional<OptedInInfo> opted_in;
};

bool operator==(const ConfirmationInfo&, const ConfirmationInfo&);
bool operator!=(const ConfirmationInfo&, const ConfirmationInfo&);

using ConfirmationList = std::vector<ConfirmationInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_
