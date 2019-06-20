/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/payment_info.h"

namespace confirmations {

PaymentInfo::PaymentInfo() :
    balance(0.0),
    month(""),
    transaction_count(0) {}

PaymentInfo::PaymentInfo(const PaymentInfo& info) :
    balance(info.balance),
    month(info.month),
    transaction_count(info.transaction_count) {}

PaymentInfo::~PaymentInfo() {}

}  // namespace confirmations
