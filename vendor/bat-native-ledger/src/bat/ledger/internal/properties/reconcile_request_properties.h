/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_RECONCILE_REQUEST_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_RECONCILE_REQUEST_PROPERTIES_H_

#include <string>

#include "bat/ledger/internal/properties/unsigned_tx_properties.h"

namespace ledger {

struct ReconcileRequestProperties {
  ReconcileRequestProperties();
  ReconcileRequestProperties(
      const ReconcileRequestProperties& properties);
  ~ReconcileRequestProperties();

  bool operator==(
      const ReconcileRequestProperties& rhs) const;

  bool operator!=(
      const ReconcileRequestProperties& rhs) const;

  std::string type;
  std::string signed_tx_headers_digest;
  std::string signed_tx_headers_signature;
  UnsignedTxProperties signed_tx_body;
  std::string signed_tx_octets;
  std::string viewing_id;
  std::string surveyor_id;
};

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_RECONCILE_REQUEST_PROPERTIES_H_
