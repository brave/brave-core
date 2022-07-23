/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_CLIENT_PROPERTIES_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_CLIENT_PROPERTIES_H_

#include <stdint.h>
#include <map>
#include <string>

#include "base/values.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"
#include "bat/ledger/mojom_structs.h"

namespace ledger {

struct ClientProperties {
  ClientProperties();
  ClientProperties(
      const ClientProperties& properties);
  ~ClientProperties();

  bool operator==(
      const ClientProperties& rhs) const;

  bool operator!=(
      const ClientProperties& rhs) const;

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
  std::map<std::string, bool> inline_tips;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEGACY_CLIENT_PROPERTIES_H_
