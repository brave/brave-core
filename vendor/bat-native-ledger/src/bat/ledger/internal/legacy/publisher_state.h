/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_LEGACY_PUBLISHER_STATE_H_
#define BRAVELEDGER_LEGACY_PUBLISHER_STATE_H_

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "bat/ledger/internal/legacy/publisher_settings_properties.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;
}

namespace ledger {
namespace publisher {

class LegacyPublisherState {
 public:
  explicit LegacyPublisherState(ledger::LedgerImpl* ledger);

  ~LegacyPublisherState();

  uint64_t GetPublisherMinVisitTime() const;  // In milliseconds

  unsigned int GetPublisherMinVisits() const;

  bool GetPublisherAllowNonVerified() const;

  bool GetPublisherAllowVideos() const;

  void Load(ledger::ResultCallback callback);

  std::vector<std::string> GetAlreadyProcessedPublishers() const;

  void GetAllBalanceReports(ledger::type::BalanceReportInfoList* reports);

 private:
  void OnLoad(
      const ledger::type::Result result,
      const std::string& data,
      ledger::ResultCallback callback);

  ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::PublisherSettingsProperties> state_;
};

}  // namespace publisher
}  // namespace ledger

#endif  // BRAVELEDGER_LEGACY_PUBLISHER_STATE_H_
