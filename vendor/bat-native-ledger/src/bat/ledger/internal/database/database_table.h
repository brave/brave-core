/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_TABLE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace database {

using ContributionPublisherInfoPair =
    std::pair<std::string, mojom::PublisherInfoPtr>;

using ServerPublisherLinksCallback =
    std::function<void(const std::map<std::string, std::string>& links)>;

using ServerPublisherAmountsCallback =
    std::function<void(const std::vector<double>& amounts)>;

using ContributionQueuePublishersListCallback =
    std::function<void(std::vector<mojom::ContributionQueuePublisherPtr>)>;

using ContributionPublisherListCallback =
    std::function<void(std::vector<mojom::ContributionPublisherPtr>)>;

using ContributionPublisherPairListCallback =
    std::function<void(std::vector<ContributionPublisherInfoPair>)>;

class DatabaseTable {
 public:
  explicit DatabaseTable(LedgerImpl* ledger);
  virtual ~DatabaseTable();

 protected:
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_TABLE_H_
