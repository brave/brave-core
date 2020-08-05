/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_GET_AD_REWARDS_GET_PAYMENTS_PAYMENT_INFO_H_
#define BAT_ADS_INTERNAL_SERVER_GET_AD_REWARDS_GET_PAYMENTS_PAYMENT_INFO_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace ads {

struct PaymentInfo {
  double balance = 0.0;
  std::string month;
  uint64_t transaction_count = 0;
};

using PaymentList = std::vector<PaymentInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_GET_AD_REWARDS_GET_PAYMENTS_PAYMENT_INFO_H_
