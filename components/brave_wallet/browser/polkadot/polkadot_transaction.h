/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_H_

#include "base/values.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

struct PolkadotTransaction {
 public:
  PolkadotTransaction();
  ~PolkadotTransaction();

  PolkadotTransaction(PolkadotTransaction&&);

  base::Value::Dict ToValue() const;
  static std::optional<PolkadotTransaction> FromValue(
      const base::Value::Dict& value);

  const PolkadotAddress& recipient() const { return recipient_; }
  void set_recipient(PolkadotAddress recipient) { recipient_ = recipient; }

  uint128_t amount() const { return amount_; }
  void set_amount(uint128_t amount) { amount_ = amount; }

  uint128_t fee() const { return fee_; }
  void set_fee(uint128_t fee) { fee_ = fee; }

  bool transfer_all() const { return transfer_all_; }
  void set_transfer_all(bool transfer_all) { transfer_all_ = transfer_all; }

 private:
  PolkadotAddress recipient_;
  uint128_t amount_ = uint128_t{0};
  uint128_t fee_ = uint128_t{0};
  bool transfer_all_ = false;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TRANSACTION_H_
