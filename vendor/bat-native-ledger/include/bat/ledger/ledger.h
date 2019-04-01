/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_H_
#define BAT_LEDGER_LEDGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/export.h"
#include "bat/ledger/auto_contribute_props.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/media_publisher_info.h"
#include "bat/ledger/transactions_info.h"
#include "bat/ledger/rewards_internals_info.h"

namespace ledger {

extern bool is_production;
extern bool is_debug;
extern bool is_testing;
extern int reconcile_time;  // minutes
extern bool short_retries;

LEDGER_EXPORT struct VisitData {
  VisitData();
  VisitData(const std::string& _tld,
            const std::string& _domain,
            const std::string& _path,
            uint32_t _tab_id,
            const std::string& name,
            const std::string& url,
            const std::string& provider,
            const std::string& favicon_url);
  VisitData(const VisitData& data);
  ~VisitData();

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string tld;
  std::string domain;
  std::string path;
  uint32_t tab_id;
  std::string name;
  std::string url;
  std::string provider;
  std::string favicon_url;
};

using PublisherBannerCallback =
    std::function<void(std::unique_ptr<ledger::PublisherBanner> banner)>;
using WalletAddressesCallback =
    std::function<void(std::map<std::string, std::string> addresses)>;
using ConfirmationsHistoryCallback = std::function<void(
    std::unique_ptr<ledger::TransactionsInfo> info)>;
using GetExcludedPublishersNumberDBCallback = std::function<void(uint32_t)>;
using OnWalletPropertiesCallback = std::function<void(const ledger::Result,
                                  std::unique_ptr<ledger::WalletInfo>)>;

class LEDGER_EXPORT Ledger {
 public:
  static bool IsMediaLink(const std::string& url,
                          const std::string& first_party_url,
                          const std::string& referrer);

  Ledger() = default;
  virtual ~Ledger() = default;

  // Not copyable, not assignable
  Ledger(const Ledger&) = delete;
  Ledger& operator=(const Ledger&) = delete;

  static Ledger* CreateInstance(LedgerClient* client);

  virtual void Initialize() = 0;

  // returns false if wallet initialization is already in progress
  virtual bool CreateWallet() = 0;

  virtual void AddRecurringPayment(const std::string& publisher_id,
                                   const double& value) = 0;

  virtual void DoDirectDonation(const PublisherInfo& publisher,
                                int amount,
                                const std::string& currency) = 0;

  virtual void OnLoad(const VisitData& visit_data,
                      const uint64_t& current_time) = 0;

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

  virtual void SetPublisherInfo(
      std::unique_ptr<PublisherInfo> publisher_info) = 0;

  virtual void SetActivityInfo(
      std::unique_ptr<PublisherInfo> publisher_info) = 0;

  virtual void GetPublisherInfo(const std::string& publisher_key,
                                PublisherInfoCallback callback) = 0;

  virtual void GetActivityInfo(const ledger::ActivityInfoFilter& filter,
                                PublisherInfoCallback callback) = 0;

  virtual void SetMediaPublisherInfo(const std::string& media_key,
                                const std::string& publisher_id) = 0;

  virtual void GetMediaPublisherInfo(const std::string& media_key,
                                PublisherInfoCallback callback) = 0;

  virtual std::vector<ContributionInfo> GetRecurringDonationPublisherInfo() = 0;

  virtual void GetActivityInfoList(uint32_t start, uint32_t limit,
                                    const ledger::ActivityInfoFilter& filter,
                                    PublisherInfoListCallback callback) = 0;

  virtual void SetRewardsMainEnabled(bool enabled) = 0;

  virtual void SetPublisherMinVisitTime(uint64_t duration_in_seconds) = 0;

  virtual void SetPublisherMinVisits(unsigned int visits) = 0;

  virtual void SetPublisherAllowNonVerified(bool allow) = 0;

  virtual void SetPublisherAllowVideos(bool allow) = 0;

  virtual void SetContributionAmount(double amount) = 0;

  virtual void SetUserChangedContribution() = 0;

  virtual void SetAutoContribute(bool enabled) = 0;

  virtual void SetBalanceReport(ACTIVITY_MONTH month,
                              int year,
                              const ledger::BalanceReportInfo& report_info) = 0;

  virtual std::map<std::string, std::string> GetAddresses() = 0;

  virtual const std::string& GetBATAddress() const = 0;

  virtual const std::string& GetBTCAddress() const = 0;

  virtual const std::string& GetETHAddress() const = 0;

  virtual const std::string& GetLTCAddress() const = 0;

  virtual uint64_t GetReconcileStamp() const = 0;

  virtual bool GetRewardsMainEnabled() const = 0;

  virtual uint64_t GetPublisherMinVisitTime() const = 0;  // In milliseconds

  virtual unsigned int GetPublisherMinVisits() const = 0;

  virtual void GetExcludedPublishersNumber(
      ledger::GetExcludedPublishersNumberDBCallback callback) const = 0;

  virtual bool GetPublisherAllowNonVerified() const = 0;

  virtual bool GetPublisherAllowVideos() const = 0;

  virtual double GetContributionAmount() const = 0;

  virtual bool GetAutoContribute() const = 0;

  virtual void FetchWalletProperties(
      OnWalletPropertiesCallback callback) const = 0;

  virtual void FetchGrants(const std::string& lang,
                           const std::string& paymentId) const = 0;

  virtual void SolveGrantCaptcha(const std::string& solution,
                                 const std::string& promotionId) const = 0;

  virtual void GetGrantCaptcha(
      const std::string& promotion_id,
      const std::string& promotion_type) const = 0;

  virtual std::string GetWalletPassphrase() const = 0;

  virtual bool GetBalanceReport(ACTIVITY_MONTH month,
                              int year,
                              ledger::BalanceReportInfo* report_info) const = 0;

  virtual std::map<std::string, ledger::BalanceReportInfo>
  GetAllBalanceReports() const = 0;

  virtual void GetAutoContributeProps(ledger::AutoContributeProps* props) = 0;

  virtual void RecoverWallet(const std::string& passPhrase) const = 0;

  virtual void SaveMediaVisit(const std::string& publisher_id,
                              const ledger::VisitData& visit_data,
                              const uint64_t& duration,
                              const uint64_t window_id) = 0;

  virtual void SetPublisherExclude(
      const std::string& publisher_id,
      const ledger::PUBLISHER_EXCLUDE& exclude) = 0;

  virtual void RestorePublishers() = 0;

  virtual bool IsWalletCreated() const = 0;

  virtual void GetPublisherActivityFromUrl(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& publisher_blob) = 0;

  virtual void SetBalanceReportItem(ACTIVITY_MONTH month,
                                    int year,
                                    ledger::ReportType type,
                                    const std::string& probi) = 0;

  virtual void GetPublisherBanner(
      const std::string& publisher_id,
      ledger::PublisherBannerCallback callback) = 0;

  virtual double GetBalance() = 0;

  virtual void OnReconcileCompleteSuccess(
      const std::string& viewing_id,
      const ledger::REWARDS_CATEGORY category,
      const std::string& probi,
      const ledger::ACTIVITY_MONTH month,
      const int year,
      const uint32_t date) = 0;

  virtual void RemoveRecurringTip(const std::string& publisher_key) = 0;

  virtual double GetDefaultContributionAmount() = 0;

  virtual uint64_t GetBootStamp() const = 0;

  virtual bool HasSufficientBalanceToReconcile() = 0;

  virtual void GetAddressesForPaymentId(
      ledger::WalletAddressesCallback callback) = 0;

  virtual void SetCatalogIssuers(const std::string& info) = 0;

  virtual void ConfirmAd(const std::string& info) = 0;
  virtual void GetConfirmationsHistory(
      const uint64_t from_timestamp_seconds,
      const uint64_t to_timestamp_seconds,
      ledger::ConfirmationsHistoryCallback callback) = 0;
  virtual void GetRewardsInternalsInfo(ledger::RewardsInternalsInfo* info) = 0;

  virtual void GetRecurringTips(ledger::PublisherInfoListCallback callback) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_H_
