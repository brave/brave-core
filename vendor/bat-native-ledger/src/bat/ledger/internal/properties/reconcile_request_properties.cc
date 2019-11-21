/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/reconcile_request_properties.h"

namespace ledger {

ReconcileRequestProperties::ReconcileRequestProperties() = default;

ReconcileRequestProperties::ReconcileRequestProperties(
    const ReconcileRequestProperties& properties) {
  type = properties.type;
  signed_tx_headers_digest = properties.signed_tx_headers_digest;
  signed_tx_headers_signature = properties.signed_tx_headers_signature;
  signed_tx_body = properties.signed_tx_body;
  signed_tx_octets = properties.signed_tx_octets;
  viewing_id = properties.viewing_id;
  surveyor_id = properties.surveyor_id;
}

ReconcileRequestProperties::~ReconcileRequestProperties() = default;

bool ReconcileRequestProperties::operator==(
    const ReconcileRequestProperties& rhs) const {
  return type == rhs.type &&
    signed_tx_headers_digest == rhs.signed_tx_headers_digest &&
    signed_tx_headers_signature == rhs.signed_tx_headers_signature &&
    signed_tx_body == rhs.signed_tx_body &&
    signed_tx_octets == rhs.signed_tx_octets &&
    viewing_id == rhs.viewing_id &&
    surveyor_id == rhs.surveyor_id;
}

bool ReconcileRequestProperties::operator!=(
    const ReconcileRequestProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
