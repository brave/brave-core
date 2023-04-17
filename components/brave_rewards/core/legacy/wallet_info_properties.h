/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_WALLET_INFO_PROPERTIES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_WALLET_INFO_PROPERTIES_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/values.h"

namespace brave_rewards::internal {

struct WalletInfoProperties {
  WalletInfoProperties();
  WalletInfoProperties(WalletInfoProperties&& other);
  WalletInfoProperties& operator=(WalletInfoProperties&& other);
  ~WalletInfoProperties();

  bool operator==(const WalletInfoProperties& rhs) const;

  bool operator!=(const WalletInfoProperties& rhs) const;

  base::Value::Dict ToValue() const;
  bool FromValue(const base::Value::Dict& value);

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  std::string payment_id;
  std::string address_card_id;
  std::vector<uint8_t> key_info_seed;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_WALLET_INFO_PROPERTIES_H_
