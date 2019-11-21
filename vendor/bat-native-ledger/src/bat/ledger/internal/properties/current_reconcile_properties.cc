/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/current_reconcile_properties.h"

namespace ledger {

CurrentReconcileProperties::CurrentReconcileProperties()
    : timestamp(0),
      fee(.0),
      retry_step(ContributionRetry::STEP_NO),
      retry_level(0) {}

CurrentReconcileProperties::CurrentReconcileProperties(
    const CurrentReconcileProperties& properties) {
  viewing_id = properties.viewing_id;
  anonize_viewing_id = properties.anonize_viewing_id;
  registrar_vk = properties.registrar_vk;
  pre_flight = properties.pre_flight;
  master_user_token = properties.master_user_token;
  surveyor_id = properties.surveyor_id;
  timestamp = properties.timestamp;
  rates = properties.rates;
  amount = properties.amount;
  currency = properties.currency;
  fee = properties.fee;
  directions = properties.directions;
  type = properties.type;
  retry_step = properties.retry_step;
  retry_level = properties.retry_level;
  destination = properties.destination;
  proof = properties.proof;
}

CurrentReconcileProperties::~CurrentReconcileProperties() = default;

bool CurrentReconcileProperties::operator==(
    const CurrentReconcileProperties& rhs) const {
  return viewing_id == rhs.viewing_id &&
      anonize_viewing_id == rhs.anonize_viewing_id &&
      registrar_vk == rhs.registrar_vk &&
      pre_flight == rhs.pre_flight &&
      master_user_token == rhs.master_user_token &&
      surveyor_id == rhs.surveyor_id &&
      timestamp == rhs.timestamp &&
      rates == rhs.rates &&
      amount == rhs.amount &&
      currency == rhs.currency &&
      fee == rhs.fee &&
      directions == rhs.directions &&
      type == rhs.type &&
      retry_step == rhs.retry_step &&
      retry_level == rhs.retry_level &&
      destination == rhs.destination &&
      proof == rhs.proof;
}

bool CurrentReconcileProperties::operator!=(
    const CurrentReconcileProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
