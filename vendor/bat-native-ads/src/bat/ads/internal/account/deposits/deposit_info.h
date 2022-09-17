/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INFO_H_

#include <string>

#include "base/time/time.h"

namespace ads {

struct DepositInfo final {
  DepositInfo();

  DepositInfo(const DepositInfo& other);
  DepositInfo& operator=(const DepositInfo& other);

  DepositInfo(DepositInfo&& other) noexcept;
  DepositInfo& operator=(DepositInfo&& other) noexcept;

  ~DepositInfo();

  bool IsValid() const;

  std::string creative_instance_id;
  double value = 0.0;
  base::Time expire_at;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INFO_H_
