/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATE_RECONCILE_REQUEST_STATE_H_
#define BRAVELEDGER_STATE_RECONCILE_REQUEST_STATE_H_

#include <string>

#include "bat/ledger/internal/state/state_writer.h"
#include "bat/ledger/internal/properties/reconcile_request_properties.h"
#include "rapidjson/writer.h"

namespace ledger {

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

class ReconcileRequestState
    : public state::Writer<JsonWriter*, ReconcileRequestProperties> {
 public:
  ReconcileRequestState();
  ~ReconcileRequestState();

  bool ToJson(
      JsonWriter* writer,
      const ReconcileRequestProperties& properties) const override;

  std::string ToJson(
      const ReconcileRequestProperties& properties) const override;
};

}  // namespace ledger

#endif  // BRAVELEDGER_STATE_RECONCILE_REQUEST_STATE_H_
