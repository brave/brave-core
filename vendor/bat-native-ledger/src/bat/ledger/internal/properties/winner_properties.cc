/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/winner_properties.h"

namespace ledger {

WinnerProperties::WinnerProperties()
    : vote_count(0) {}

WinnerProperties::WinnerProperties(
    const WinnerProperties& properties) {
  direction = properties.direction;
  vote_count = properties.vote_count;
}

WinnerProperties::~WinnerProperties() = default;

bool WinnerProperties::operator==(
    const WinnerProperties& rhs) const {
  return direction == rhs.direction &&
      vote_count == rhs.vote_count;
}

bool WinnerProperties::operator!=(
    const WinnerProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
