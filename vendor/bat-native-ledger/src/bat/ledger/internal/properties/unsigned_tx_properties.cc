/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/unsigned_tx_properties.h"

namespace ledger {

UnsignedTxProperties::UnsignedTxProperties() = default;

UnsignedTxProperties::UnsignedTxProperties(
    const UnsignedTxProperties& properties) {
  amount = properties.amount;
  currency = properties.currency;
  destination = properties.destination;
}

UnsignedTxProperties::~UnsignedTxProperties() = default;

bool UnsignedTxProperties::operator==(
    const UnsignedTxProperties& rhs) const {
  return amount == rhs.amount &&
      currency == rhs.currency &&
      destination == rhs.destination;
}

bool UnsignedTxProperties::operator!=(
    const UnsignedTxProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
