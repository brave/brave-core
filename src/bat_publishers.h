/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_PUBLISHERS_H_
#define BRAVELEDGER_BAT_PUBLISHERS_H_

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat_helper.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_bat_helper {
struct PUBLISHER_STATE_ST;
}

namespace braveledger_bat_publishers {

class BatPublishers : public ledger::LedgerCallbackHandler {
 public:

  BatPublishers(bat_ledger::LedgerImpl* ledger);

  ~BatPublishers() override;

  bool loadState(const std::string& data);

  void saveVisit(const ledger::VisitData& visit_data, const uint64_t& duration);

  void MakePayment(const ledger::PaymentData& paid_data);

  void AddRecurringPayment(const std::string& domain, const double& value);

  void setPublisherMinVisitTime(const uint64_t& duration); // In milliseconds

  void setPublisherMinVisits(const unsigned int& visits);

  void setPublisherAllowNonVerified(const bool& allow);
  void setBalanceReport(const std::string& year, const std::string& month, 
    const ledger::BalanceReportInfo& report_info);
  bool getBalanceReport(const std::string& year, const std::string& month, 
    ledger::BalanceReportInfo* report_info);

  uint64_t getPublisherMinVisitTime() const; // In milliseconds
  unsigned int getPublisherMinVisits() const;
  bool getPublisherAllowNonVerified() const;

  std::vector<braveledger_bat_helper::WINNERS_ST> winners(const unsigned int& ballots);

  std::unique_ptr<ledger::PublisherInfo> onPublisherInfoUpdated(
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo>);
  std::string GetPublisherKey(ledger::PUBLISHER_CATEGORY category, const std::string& year,
    const std::string& month, const std::string& publisher_id);
  std::string GetBalanceReportName(const std::string& year,
    const std::string& month);

 private:
  // LedgerCallbackHandler impl
  void OnPublisherStateSaved(ledger::Result result) override;

  bool isEligableForContribution(const ledger::PublisherInfo& info);
  bool isVerified(const ledger::PublisherInfo& publisher_id);
  void saveVisitInternal(
      std::string publisher_key,
      ledger::VisitData visit_data,
      uint64_t duration,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void makePaymentInternal(
      std::string publisher_key,
      ledger::PaymentData paid_data,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void addRecurringPaymentInternal(
      std::string publisher_key,
      std::string domain,
      double value,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> publisher_info);

  double concaveScore(const uint64_t& duration);

  void saveState();

  void calcScoreConsts();

  void synopsisNormalizer();

  bool isPublisherVisible(const braveledger_bat_helper::PUBLISHER_ST& publisher_st);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  std::vector<braveledger_bat_helper::PUBLISHER_ST> topN();

  std::map<std::string, braveledger_bat_helper::PUBLISHER_ST> publishers_;

  std::unique_ptr<braveledger_bat_helper::PUBLISHER_STATE_ST> state_;

  unsigned int a_;

  unsigned int a2_;

  unsigned int a4_;

  unsigned int b_;

  unsigned int b2_;
};

}  // namespace braveledger_bat_publishers

#endif  // BRAVELEDGER_BAT_PUBLISHERS_H_
