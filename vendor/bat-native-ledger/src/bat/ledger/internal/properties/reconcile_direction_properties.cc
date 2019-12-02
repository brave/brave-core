/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/reconcile_direction_properties.h"

namespace ledger {

ReconcileDirectionProperties::ReconcileDirectionProperties()
    : amount_percent(0.0) {}

ReconcileDirectionProperties::ReconcileDirectionProperties(
    const ReconcileDirectionProperties& properties) {
  publisher_key = properties.publisher_key;
  amount_percent = properties.amount_percent;
}

ReconcileDirectionProperties::~ReconcileDirectionProperties() = default;

bool ReconcileDirectionProperties::operator==(
    const ReconcileDirectionProperties& rhs) const {
  return publisher_key == rhs.publisher_key &&
      amount_percent == rhs.amount_percent;
}

bool ReconcileDirectionProperties::operator!=(
    const ReconcileDirectionProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
