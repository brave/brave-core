/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_SERVICE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_SERVICE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"
#include "bat/ledger/internal/publisher/publisher_data.h"

namespace ledger {

class PublisherService : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "publisher-service";

  Future<absl::optional<Publisher>> GetPublisher(
      const std::string& publisher_id);

  Future<std::map<std::string, Publisher>> GetPublishers(
      const std::vector<std::string>& publisher_ids);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_SERVICE_H_
