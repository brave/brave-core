/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_PAYMENT_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_PAYMENT_DATA_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/core/enum_string.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {

struct PaymentOrderItem {
  std::string id;
  std::string sku;
  int quantity;
  double price;
};

enum class PaymentOrderStatus { kPending, kPaid, kFulfilled, kCanceled };

std::string StringifyEnum(PaymentOrderStatus status);

absl::optional<PaymentOrderStatus> ParseEnum(
    const EnumString<PaymentOrderStatus>& s);

enum class PaymentCredentialType { kSingleUse };

std::string StringifyEnum(PaymentCredentialType type);

absl::optional<PaymentCredentialType> ParseEnum(
    const EnumString<PaymentCredentialType>& s);

struct PaymentOrder {
  PaymentOrder();
  ~PaymentOrder();

  PaymentOrder(const PaymentOrder& other);
  PaymentOrder& operator=(const PaymentOrder& other);

  PaymentOrder(PaymentOrder&& other);
  PaymentOrder& operator=(PaymentOrder&& other);

  std::string id;
  double total_price = 0;
  PaymentOrderStatus status = PaymentOrderStatus::kPending;
  std::vector<PaymentOrderItem> items;
};

struct PaymentCredentials {
  PaymentCredentials();
  ~PaymentCredentials();

  PaymentCredentials(const PaymentCredentials& other);
  PaymentCredentials& operator=(const PaymentCredentials& other);

  PaymentCredentials(PaymentCredentials&& other);
  PaymentCredentials& operator=(PaymentCredentials&& other);

  std::string batch_proof;
  std::string public_key;
  std::vector<std::string> signed_tokens;
};

struct PaymentVote {
  std::string unblinded_token;
  std::string public_key;
};

enum class PaymentVoteType {
  kAutoContribute,
  kOneOffTip,
  kRecurringTip,
  kPayment
};

std::string StringifyEnum(PaymentVoteType type);

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_PAYMENT_DATA_H_
