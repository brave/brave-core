/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_UNSIGNED_TX_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_UNSIGNED_TX_PROPERTIES_H_

#include <string>

namespace ledger {

struct UnsignedTxProperties {
  UnsignedTxProperties();
  UnsignedTxProperties(
      const UnsignedTxProperties& properties);
  ~UnsignedTxProperties();

  bool operator==(
      const UnsignedTxProperties& rhs) const;

  bool operator!=(
      const UnsignedTxProperties& rhs) const;

  std::string amount;
  std::string currency;
  std::string destination;
};

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_UNSIGNED_TX_PROPERTIES_H_
