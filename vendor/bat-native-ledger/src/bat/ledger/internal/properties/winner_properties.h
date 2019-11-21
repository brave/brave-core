/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_WINNER_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_WINNER_PROPERTIES_H_

#include <stdint.h>
#include <vector>

#include "bat/ledger/internal/properties/reconcile_direction_properties.h"

namespace ledger {

struct WinnerProperties {
  WinnerProperties();
  WinnerProperties(
      const WinnerProperties& properties);
  ~WinnerProperties();

  bool operator==(
      const WinnerProperties& rhs) const;

  bool operator!=(
      const WinnerProperties& rhs) const;

  ReconcileDirectionProperties direction;
  uint32_t vote_count;
};

typedef std::vector<WinnerProperties> Winners;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_WINNER_PROPERTIES_H_
