/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_TABLE_H_
#define BRAVELEDGER_DATABASE_DATABASE_TABLE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_database {

using ContributionPublisherInfoPair =
    std::pair<std::string, ledger::PublisherInfoPtr>;

using ServerPublisherLinksCallback =
    std::function<void(const std::map<std::string, std::string>& links)>;

using ServerPublisherAmountsCallback =
    std::function<void(const std::vector<double>& amounts)>;

using ContributionQueuePublishersListCallback =
    std::function<void(ledger::ContributionQueuePublisherList)>;

using ContributionPublisherListCallback =
    std::function<void(ledger::ContributionPublisherList)>;

using ContributionPublisherPairListCallback =
    std::function<void(std::vector<ContributionPublisherInfoPair>)>;

class DatabaseTable {
 public:
  explicit DatabaseTable(bat_ledger::LedgerImpl* ledger);
  virtual ~DatabaseTable();

 protected:
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_TABLE_H_
