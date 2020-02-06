/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PAYMENT_INFO_H_
#define BAT_CONFIRMATIONS_INTERNAL_PAYMENT_INFO_H_

#include <stdint.h>
#include <string>

namespace confirmations {

struct PaymentInfo {
  double balance = 0.0;
  std::string month;
  uint64_t transaction_count = 0;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PAYMENT_INFO_H_
