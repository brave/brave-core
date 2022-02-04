/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BASE_H_

#include <string>

#include "bat/ads/internal/account/deposits/deposit_aliases.h"

namespace ads {

class DepositBase {
 public:
  DepositBase();
  virtual ~DepositBase();

  virtual void GetValue(const std::string& creative_instance_id,
                        GetDepositCallback callback) = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_BASE_H_
