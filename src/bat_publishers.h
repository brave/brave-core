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
#include "bat/ledger/publisher_info.h"
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

  void saveVisit(const std::string& publisher_id,
                 const ledger::VisitData& visit_data,
                 const uint64_t& duration);
  bool saveVisitAllowed() const;

  void MakePayment(const ledger::PaymentData& payment_data);

  void AddRecurringPayment(const std::string& publisher_id, const double& value);

  void setPublisherMinVisitTime(const uint64_t& duration); // In seconds

  void setPublisherMinVisits(const unsigned int& visits);

  void setPublishersLastRefreshTimestamp(uint64_t ts);

  void setExclude(const std::string& publisher_id, const ledger::PUBLISHER_EXCLUDE& exclude);
  void restorePublishers();

  void setPublisherAllowNonVerified(const bool& allow);
  void setPublisherAllowVideos(const bool& allow);
  void setBalanceReport(ledger::PUBLISHER_MONTH month,
                        int year,
                        const ledger::BalanceReportInfo& report_info);
  bool getBalanceReport(ledger::PUBLISHER_MONTH month,
                        int year,
                        ledger::BalanceReportInfo* report_info);
  std::map<std::string, ledger::BalanceReportInfo> getAllBalanceReports();

  uint64_t getPublisherMinVisitTime() const; // In milliseconds
  unsigned int getPublisherMinVisits() const;
  bool getPublisherAllowNonVerified() const;
  uint64_t getLastPublishersListLoadTimestamp() const;
  bool getPublisherAllowVideos() const;

  std::vector<braveledger_bat_helper::WINNERS_ST> winners(const unsigned int& ballots);

  std::unique_ptr<ledger::PublisherInfo> onPublisherInfoUpdated(
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo>);
  std::string GetBalanceReportName(ledger::PUBLISHER_MONTH month, int year);
  std::vector<ledger::ContributionInfo> GetRecurringDonationList();

  void RefreshPublishersList(const std::string & pubs_list);

  void OnPublishersListSaved(ledger::Result result) override;

  bool loadPublisherList(const std::string& data);

 private:
  ledger::PublisherInfoFilter CreatePublisherFilter(
      const std::string& publisher_id,
      ledger::PUBLISHER_CATEGORY category,
      ledger::PUBLISHER_MONTH month,
      int year,
      ledger::PUBLISHER_EXCLUDE excluded);

  // LedgerCallbackHandler impl
  void OnPublisherStateSaved(ledger::Result result) override;

  bool isEligableForContribution(const ledger::PublisherInfo& info);
  bool isVerified(const std::string& publisher_id);
  bool isExcluded(const std::string& publisher_id, const ledger::PUBLISHER_EXCLUDE& excluded);
  void saveVisitInternal(
      std::string publisher_id,
      ledger::VisitData visit_data,
      uint64_t duration,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void makePaymentInternal(
      ledger::PaymentData payment_data,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void onSetExcludeInternal(
    ledger::PUBLISHER_EXCLUDE exclude,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info);

  void onRestorePublishersInternal(const ledger::PublisherInfoList& publisherInfoList, uint32_t /* next_record */);

  double concaveScore(const uint64_t& duration);

  void saveState();

  void calcScoreConsts();

  void synopsisNormalizer(const ledger::PublisherInfo& info);
  void synopsisNormalizerInternal(const ledger::PublisherInfoList& list, uint32_t /* next_record */);

  bool isPublisherVisible(const braveledger_bat_helper::PUBLISHER_ST& publisher_st);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED

  std::vector<braveledger_bat_helper::PUBLISHER_ST> topN();

  std::unique_ptr<braveledger_bat_helper::PUBLISHER_STATE_ST> state_;

  std::map<std::string, braveledger_bat_helper::SERVER_LIST> server_list_;

  unsigned int a_;

  unsigned int a2_;

  unsigned int a4_;

  unsigned int b_;

  unsigned int b2_;
};

}  // namespace braveledger_bat_publishers

#endif  // BRAVELEDGER_BAT_PUBLISHERS_H_
