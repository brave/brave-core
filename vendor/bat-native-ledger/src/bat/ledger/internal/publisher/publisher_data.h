/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_DATA_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/external_wallet/external_wallet_data.h"

namespace ledger {

struct Publisher {
  Publisher();
  ~Publisher();

  Publisher(const Publisher& other);
  Publisher& operator=(const Publisher& other);

  Publisher(Publisher&& other);
  Publisher& operator=(Publisher&& other);

  std::string id;
  bool registered = false;
  std::vector<ExternalWallet> wallets;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_DATA_H_
