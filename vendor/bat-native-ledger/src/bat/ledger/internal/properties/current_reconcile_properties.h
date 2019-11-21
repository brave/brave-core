/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_CURRENT_RECONCILE_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_CURRENT_RECONCILE_PROPERTIES_H_

#include <stdint.h>
#include <string>
#include <map>

#include "bat/ledger/internal/properties/reconcile_direction_properties.h"
#include "bat/ledger/mojom_structs.h"

namespace ledger {

struct CurrentReconcileProperties {
  CurrentReconcileProperties();
  CurrentReconcileProperties(
      const CurrentReconcileProperties& properties);
  ~CurrentReconcileProperties();

  bool operator==(
      const CurrentReconcileProperties& rhs) const;

  bool operator!=(
      const CurrentReconcileProperties& rhs) const;

  std::string viewing_id;
  std::string anonize_viewing_id;
  std::string registrar_vk;
  std::string pre_flight;
  std::string master_user_token;
  std::string surveyor_id;
  uint64_t timestamp;
  std::map<std::string, double> rates;
  std::string amount;
  std::string currency;
  double fee;
  ReconcileDirections directions;
  RewardsType type;
  ContributionRetry retry_step;
  int32_t retry_level;
  std::string destination;
  std::string proof;
};

typedef std::map<std::string, CurrentReconcileProperties> CurrentReconciles;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_CURRENT_RECONCILE_PROPERTIES_H_
