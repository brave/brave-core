/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_LEGACY_PUBLISHER_STATE_H_
#define BRAVELEDGER_LEGACY_PUBLISHER_STATE_H_

#include <string>
#include <map>
#include <memory>

#include "bat/ledger/internal/legacy/publisher_settings_properties.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_publisher {

class Publisher;

class LegacyPublisherState {
 public:
  explicit LegacyPublisherState(
      bat_ledger::LedgerImpl* ledger,
      Publisher* publisher);

  ~LegacyPublisherState();

  std::string GetBalanceReportName(ledger::ActivityMonth month, int year);

  void SetPublisherMinVisitTime(const uint64_t& duration);  // In seconds

  void SetPublisherMinVisits(const unsigned int visits);

  void SetPublisherAllowNonVerified(const bool allow);

  void SetPublisherAllowVideos(const bool allow);

  uint64_t GetPublisherMinVisitTime() const;  // In milliseconds

  unsigned int GetPublisherMinVisits() const;

  bool GetPublisherAllowNonVerified() const;

  bool GetPublisherAllowVideos() const;

  bool GetMigrateScore() const;

  void SetMigrateScore(bool value);

  void ClearAllBalanceReports();

  void SetBalanceReport(
      ledger::ActivityMonth month,
      int year,
      const ledger::BalanceReportInfo& report_info);

  void GetBalanceReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetBalanceReportCallback callback);

  std::map<std::string, ledger::BalanceReportInfoPtr> GetAllBalanceReports();

  void SaveState();

  bool LoadState(const std::string& data);

  void SetBalanceReportItem(
      const ledger::ActivityMonth month,
      const int year,
      const ledger::ReportType type,
      const double amount);

  void SavePublisherProcessed(const std::string& publisher_key);

  bool WasPublisherAlreadyProcessed(const std::string& publisher_key) const;

 private:
  void OnPublisherStateSaved(const ledger::Result result);

  ledger::Result GetBalanceReportInternal(
      const ledger::ActivityMonth month,
      const int year,
      ledger::BalanceReportInfo* report_info);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Publisher* publisher_;  // NOT OWNED
  std::unique_ptr<ledger::PublisherSettingsProperties> state_;
};

}  // namespace braveledger_publisher

#endif  // BRAVELEDGER_LEGACY_PUBLISHER_STATE_H_
