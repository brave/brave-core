/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_CLIENT_PROPERTIES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_CLIENT_PROPERTIES_H_

#include <stdint.h>
#include <map>
#include <string>

#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/legacy/wallet_info_properties.h"

namespace brave_rewards::internal {

struct ClientProperties {
  ClientProperties();
  ClientProperties(ClientProperties&& other);
  ClientProperties& operator=(ClientProperties&& other);
  ~ClientProperties();

  bool operator==(const ClientProperties& rhs) const;

  bool operator!=(const ClientProperties& rhs) const;

  base::Value::Dict ToValue() const;
  bool FromValue(const base::Value::Dict& value);

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  WalletInfoProperties wallet_info;
  uint64_t boot_timestamp;
  uint64_t reconcile_timestamp;
  double fee_amount;
  bool user_changed_fee;
  bool auto_contribute;
  bool rewards_enabled;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_CLIENT_PROPERTIES_H_
