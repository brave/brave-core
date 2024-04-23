/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INFO_H_

#include <optional>
#include <string>

#include "base/time/time.h"

namespace brave_ads {

struct DepositInfo final {
  [[nodiscard]] bool IsValid() const;

  std::string creative_instance_id;
  double value = 0.0;
  std::optional<base::Time> expire_at;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INFO_H_
