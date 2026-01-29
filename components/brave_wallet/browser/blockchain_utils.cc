/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_utils.h"

#include "brave/components/brave_wallet/browser/blockchain_registry.h"

namespace brave_wallet {

bool IsOfacAddress(const std::string& address) {
  return BlockchainRegistry::GetInstance()->IsOfacAddress(address);
}

}  // namespace brave_wallet
