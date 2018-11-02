/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_H_
#define BAT_LEDGER_LEDGER_H_

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/export.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/media_publisher_info.h"

namespace ledger {

extern bool is_production;
extern int reconcile_time; // minutes

LEDGER_EXPORT struct VisitData {
  VisitData();
  VisitData(const std::string& _tld,
            const std::string& _domain,
            const std::string& _path,
            uint32_t _tab_id,
            PUBLISHER_MONTH _local_month,
            int _local_year,
            const std::string& name,
            const std::string& url,
            const std::string& provider,
            const std::string& favicon_url);
  VisitData(const VisitData& data);
  ~VisitData();

  std::string tld;
  std::string domain;
  std::string path;
  uint32_t tab_id;
  PUBLISHER_MONTH local_month;
  int local_year;
  std::string name;
  std::string url;
  std::string provider;
  std::string favicon_url;
};

LEDGER_EXPORT struct PaymentData {
  PaymentData();
  PaymentData(const std::string& _publisher_id,
           const double& _value,
           const int64_t& _timestamp,
           PUBLISHER_CATEGORY _category,
           PUBLISHER_MONTH _local_month,
           int _local_year);
  PaymentData(const PaymentData& data);
  ~PaymentData();

  std::string publisher_id;
  double value;
  int64_t timestamp;
  PUBLISHER_CATEGORY category;
  PUBLISHER_MONTH local_month;
  int local_year;
};


class LEDGER_EXPORT Ledger {
 public:
  static bool IsMediaLink(const std::string& url, const std::string& first_party_url, const std::string& referrer);

  Ledger() = default;
  virtual ~Ledger() = default;

  // Not copyable, not assignable
  Ledger(const Ledger&) = delete;
  Ledger& operator=(const Ledger&) = delete;

  static Ledger* CreateInstance(LedgerClient* client);

  virtual void Initialize() = 0;
  // returns false if wallet initialization is already in progress
  virtual bool CreateWallet() = 0;
  virtual void Reconcile() = 0;

  virtual void MakePayment(const PaymentData& payment_data) = 0;
  virtual void AddRecurringPayment(const std::string& publisher_id, const double& value) = 0;
  virtual void DoDirectDonation(const PublisherInfo& publisher, const int amount, const std::string& currency) = 0;
  virtual void OnLoad(const VisitData& visit_data, const uint64_t& current_time) = 0;
  virtual void OnUnload(uint32_t tab_id, const uint64_t& current_time) = 0;
  virtual void OnShow(uint32_t tab_id, const uint64_t& current_time) = 0;
  virtual void OnHide(uint32_t tab_id, const uint64_t& current_time) = 0;
  virtual void OnForeground(uint32_t tab_id, const uint64_t& current_time) = 0;
  virtual void OnBackground(uint32_t tab_id, const uint64_t& current_time) = 0;
  virtual void OnMediaStart(uint32_t tab_id, const uint64_t& current_time) = 0;
  virtual void OnMediaStop(uint32_t tab_id, const uint64_t& current_time) = 0;
  virtual void OnXHRLoad(
      uint32_t tab_id,
      const std::string& url,
      const std::map<std::string, std::string>& parts,
      const std::string& first_party_url,
      const std::string& referrer,
      const VisitData& visit_data) = 0;

  virtual void OnPostData(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      const VisitData& visit_data) = 0;

  virtual void OnTimer(uint32_t timer_id) = 0;

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual void SetPublisherInfo(std::unique_ptr<PublisherInfo> publisher_info,
                                PublisherInfoCallback callback) = 0;
  virtual void GetPublisherInfo(const ledger::PublisherInfoFilter& filter,
                                PublisherInfoCallback callback) = 0;
  virtual void SetMediaPublisherInfo(const std::string& media_key,
                                const std::string& publisher_id) = 0;
  virtual void GetMediaPublisherInfo(const std::string& media_key,
                                PublisherInfoCallback callback) = 0;
  virtual std::vector<ContributionInfo> GetRecurringDonationPublisherInfo() = 0;
  virtual void GetPublisherInfoList(uint32_t start, uint32_t limit,
                                    const ledger::PublisherInfoFilter& filter,
                                    GetPublisherInfoListCallback callback) = 0;
  virtual void GetCurrentPublisherInfoList(uint32_t start, uint32_t limit,
                            const ledger::PublisherInfoFilter& filter,
                            ledger::GetPublisherInfoListCallback callback) = 0;

  virtual void SetRewardsMainEnabled(bool enabled) = 0;
  virtual void SetPublisherMinVisitTime(uint64_t duration_in_seconds) = 0;
  virtual void SetPublisherMinVisits(unsigned int visits) = 0;
  virtual void SetPublisherAllowNonVerified(bool allow) = 0;
  virtual void SetPublisherAllowVideos(bool allow) = 0;
  virtual void SetContributionAmount(double amount) = 0;
  virtual void SetUserChangedContribution() = 0;
  virtual void SetAutoContribute(bool enabled) = 0;
  virtual void SetBalanceReport(PUBLISHER_MONTH month,
                              int year,
                              const ledger::BalanceReportInfo& report_info) = 0;

  virtual const std::string& GetBATAddress() const = 0;
  virtual const std::string& GetBTCAddress() const = 0;
  virtual const std::string& GetETHAddress() const = 0;
  virtual const std::string& GetLTCAddress() const = 0;
  virtual uint64_t GetReconcileStamp() const = 0;
  virtual bool GetRewardsMainEnabled() const = 0;
  virtual uint64_t GetPublisherMinVisitTime() const = 0; // In milliseconds
  virtual unsigned int GetPublisherMinVisits() const = 0;
  virtual unsigned int GetNumExcludedSites() const = 0;
  virtual bool GetPublisherAllowNonVerified() const = 0;
  virtual bool GetPublisherAllowVideos() const = 0;
  virtual double GetContributionAmount() const = 0;
  virtual bool GetAutoContribute() const = 0;
  virtual void GetWalletProperties() const = 0;
  virtual void GetGrant(const std::string& lang, const std::string& paymentId) const = 0;
  virtual void SolveGrantCaptcha(const std::string& solution) const = 0;
  virtual void GetGrantCaptcha() const = 0;
  virtual std::string GetWalletPassphrase() const = 0;
  virtual bool GetBalanceReport(PUBLISHER_MONTH month,
                              int year,
                              ledger::BalanceReportInfo* report_info) const = 0;
  virtual std::map<std::string, ledger::BalanceReportInfo> GetAllBalanceReports() const = 0;

  virtual void RecoverWallet(const std::string& passPhrase) const = 0;
  virtual void SaveMediaVisit(const std::string& publisher_id,
                              const ledger::VisitData& visit_data,
                              const uint64_t& duration,
                              const uint64_t window_id) = 0;
  virtual void SetPublisherExclude(const std::string& publisher_id, const ledger::PUBLISHER_EXCLUDE& exclude) = 0;
  virtual void SetPublisherPanelExclude(const std::string& publisher_id,
    const ledger::PUBLISHER_EXCLUDE& exclude, uint64_t windowId) = 0;
  virtual void RestorePublishers() = 0;
  virtual bool IsWalletCreated() const = 0;
  virtual void GetPublisherActivityFromUrl(uint64_t windowId, const ledger::VisitData& visit_data) = 0;
  virtual void SetBalanceReportItem(PUBLISHER_MONTH month,
                                    int year,
                                    ledger::ReportType type,
                                    const std::string& probi) = 0;
  virtual ledger::PublisherBanner GetPublisherBanner(const std::string& publisher_id) = 0;
  virtual double GetBalance() = 0;
  virtual void OnReconcileCompleteSuccess(const std::string& viewing_id,
                                          const ledger::PUBLISHER_CATEGORY category,
                                          const std::string& probi,
                                          const ledger::PUBLISHER_MONTH month,
                                          const int year,
                                          const uint32_t date) = 0;
  virtual void RemoveRecurring(const std::string& publisher_key) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_H_
