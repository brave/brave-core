/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/payment_data.h"

namespace ledger {

std::string StringifyEnum(PaymentOrderStatus status) {
  switch (status) {
    case PaymentOrderStatus::kPending:
      return "pending";
    case PaymentOrderStatus::kPaid:
      return "paid";
    case PaymentOrderStatus::kFulfilled:
      return "fulfilled";
    case PaymentOrderStatus::kCanceled:
      return "canceled";
  }
}

absl::optional<PaymentOrderStatus> ParseEnum(
    const EnumString<PaymentOrderStatus>& s) {
  return s.Match({PaymentOrderStatus::kPending, PaymentOrderStatus::kPaid,
                  PaymentOrderStatus::kFulfilled,
                  PaymentOrderStatus::kCanceled});
}

std::string StringifyEnum(PaymentCredentialType type) {
  switch (type) {
    case PaymentCredentialType::kSingleUse:
      return "single-use";
  }
}

absl::optional<PaymentCredentialType> ParseEnum(
    const EnumString<PaymentCredentialType>& s) {
  return s.Match({PaymentCredentialType::kSingleUse});
}

PaymentOrder::PaymentOrder() = default;
PaymentOrder::~PaymentOrder() = default;

PaymentOrder::PaymentOrder(const PaymentOrder&) = default;
PaymentOrder& PaymentOrder::operator=(const PaymentOrder&) = default;

PaymentOrder::PaymentOrder(PaymentOrder&&) = default;
PaymentOrder& PaymentOrder::operator=(PaymentOrder&&) = default;

PaymentCredentials::PaymentCredentials() = default;
PaymentCredentials::~PaymentCredentials() = default;

PaymentCredentials::PaymentCredentials(const PaymentCredentials&) = default;
PaymentCredentials& PaymentCredentials::operator=(const PaymentCredentials&) =
    default;

PaymentCredentials::PaymentCredentials(PaymentCredentials&&) = default;
PaymentCredentials& PaymentCredentials::operator=(PaymentCredentials&&) =
    default;

std::string StringifyEnum(PaymentVoteType type) {
  switch (type) {
    case PaymentVoteType::kAutoContribute:
      return "auto-contribute";
    case PaymentVoteType::kOneOffTip:
      return "oneoff-tip";
    case PaymentVoteType::kRecurringTip:
      return "recurring-tip";
    case PaymentVoteType::kPayment:
      return "payment";
  }
}

}  // namespace ledger
