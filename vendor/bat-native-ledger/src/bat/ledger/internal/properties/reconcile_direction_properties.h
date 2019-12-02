/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_RECONCILE_DIRECTION_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_RECONCILE_DIRECTION_PROPERTIES_H_

#include <string>
#include <vector>

namespace ledger {

struct ReconcileDirectionProperties {
  ReconcileDirectionProperties();
  ReconcileDirectionProperties(
      const ReconcileDirectionProperties& properties);
  ~ReconcileDirectionProperties();

  bool operator==(
      const ReconcileDirectionProperties& rhs) const;

  bool operator!=(
      const ReconcileDirectionProperties& rhs) const;

  std::string publisher_key;
  double amount_percent;
};

typedef std::vector<ReconcileDirectionProperties> ReconcileDirections;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_RECONCILE_DIRECTION_PROPERTIES_H_
