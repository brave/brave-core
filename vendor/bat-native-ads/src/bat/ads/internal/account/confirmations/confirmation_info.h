/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_

#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/confirmations/opted_in_info.h"

namespace ads {

struct ConfirmationInfo final {
  ConfirmationInfo();

  ConfirmationInfo(const ConfirmationInfo& other);
  ConfirmationInfo& operator=(const ConfirmationInfo& other);

  ConfirmationInfo(ConfirmationInfo&& other) noexcept;
  ConfirmationInfo& operator=(ConfirmationInfo&& other) noexcept;

  ~ConfirmationInfo();

  std::string transaction_id;
  std::string creative_instance_id;
  ConfirmationType type = ConfirmationType::kUndefined;
  AdType ad_type = AdType::kUndefined;
  base::Time created_at;
  bool was_created = false;
  absl::optional<OptedInInfo> opted_in;
};

bool operator==(const ConfirmationInfo& lhs, const ConfirmationInfo& rhs);
bool operator!=(const ConfirmationInfo& lhs, const ConfirmationInfo& rhs);

using ConfirmationList = std::vector<ConfirmationInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_INFO_H_
