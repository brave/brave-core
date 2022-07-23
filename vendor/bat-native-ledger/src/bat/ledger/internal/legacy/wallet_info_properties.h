/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_WALLET_INFO_PROPERTIES_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_WALLET_INFO_PROPERTIES_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/values.h"

namespace ledger {

struct WalletInfoProperties {
  WalletInfoProperties();
  WalletInfoProperties(
      const WalletInfoProperties& properties);
  ~WalletInfoProperties();

  bool operator==(
      const WalletInfoProperties& rhs) const;

  bool operator!=(
      const WalletInfoProperties& rhs) const;

  base::Value::Dict ToValue() const;
  bool FromValue(const base::Value::Dict& value);

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  std::string payment_id;
  std::string address_card_id;
  std::vector<uint8_t> key_info_seed;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_WALLET_INFO_PROPERTIES_H_
