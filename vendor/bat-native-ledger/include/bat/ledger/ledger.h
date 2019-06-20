/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_H_
#define BAT_LEDGER_LEDGER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/export.h"
#include "bat/ledger/auto_contribute_props.h"
#include "bat/ledger/balance.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/media_publisher_info.h"
#include "bat/ledger/transactions_info.h"
#include "bat/ledger/rewards_internals_info.h"
#include "bat/ledger/pending_contribution.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using VisitData = ledger::mojom::VisitData;
using VisitDataPtr = ledger::mojom::VisitDataPtr;

extern bool is_production;
extern bool is_debug;
extern bool is_testing;
extern int reconcile_time;  // minutes
extern bool short_retries;

using PublisherBannerCallback =
    std::function<void(std::unique_ptr<ledger::PublisherBanner> banner)>;
using WalletAddressesCallback =
    std::function<void(std::map<std::string, std::string> addresses)>;
using GetTransactionHistoryCallback =
    std::function<void(std::unique_ptr<ledger::TransactionsInfo> info)>;
using OnWalletPropertiesCallback = std::function<void(const ledger::Result,
                                  ledger::WalletPropertiesPtr)>;
using OnRefreshPublisherCallback =
    std::function<void(bool)>;
using GetAddressesCallback =
    std::function<void(std::map<std::string, std::string>)>;
using HasSufficientBalanceToReconcileCallback = std::function<void(bool)>;
using FetchBalanceCallback = std::function<void(ledger::Result,
                                                ledger::BalancePtr)>;

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

  virtual void DoDirectTip(const std::string& publisher_id,
                           int amount,
                           const std::string& currency) = 0;

  virtual void OnLoad(VisitDataPtr visit_data,
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
      VisitDataPtr visit_data) = 0;


  virtual void OnPostData(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      VisitDataPtr visit_data) = 0;

  virtual void OnTimer(uint32_t timer_id) = 0;

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual void SetPublisherInfo(PublisherInfoPtr publisher_info) = 0;

  virtual void SetActivityInfo(PublisherInfoPtr publisher_info) = 0;

  virtual void GetPublisherInfo(const std::string& publisher_key,
                                PublisherInfoCallback callback) = 0;

  virtual void GetActivityInfo(const ledger::ActivityInfoFilter& filter,
                                PublisherInfoCallback callback) = 0;

  virtual void SetMediaPublisherInfo(const std::string& media_key,
                                const std::string& publisher_id) = 0;

  virtual void GetMediaPublisherInfo(const std::string& media_key,
                                PublisherInfoCallback callback) = 0;

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

  virtual void UpdateAdsRewards() = 0;

  virtual void SetBalanceReport(ACTIVITY_MONTH month,
                              int year,
                              const ledger::BalanceReportInfo& report_info) = 0;

  virtual void GetAddresses(
      int32_t current_country_code,
      ledger::GetAddressesCallback callback) = 0;

  virtual const std::string& GetBATAddress() const = 0;

  virtual const std::string& GetBTCAddress() const = 0;

  virtual const std::string& GetETHAddress() const = 0;

  virtual const std::string& GetLTCAddress() const = 0;

  virtual uint64_t GetReconcileStamp() const = 0;

  virtual bool GetRewardsMainEnabled() const = 0;

  virtual uint64_t GetPublisherMinVisitTime() const = 0;  // In milliseconds

  virtual unsigned int GetPublisherMinVisits() const = 0;

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
      const std::vector<std::string>& headers) const = 0;

  virtual std::string GetWalletPassphrase() const = 0;

  virtual bool GetBalanceReport(ACTIVITY_MONTH month,
                              int year,
                              ledger::BalanceReportInfo* report_info) const = 0;

  virtual std::map<std::string, ledger::BalanceReportInfo>
  GetAllBalanceReports() const = 0;

  virtual void GetAutoContributeProps(ledger::AutoContributeProps* props) = 0;

  virtual void RecoverWallet(const std::string& passPhrase) const = 0;

  virtual void SetPublisherExclude(
      const std::string& publisher_id,
      const ledger::PUBLISHER_EXCLUDE& exclude) = 0;

  virtual void RestorePublishers() = 0;

  virtual bool IsWalletCreated() const = 0;

  virtual void GetPublisherActivityFromUrl(
      uint64_t windowId,
      ledger::VisitDataPtr visit_data,
      const std::string& publisher_blob) = 0;

  virtual void SetBalanceReportItem(ACTIVITY_MONTH month,
                                    int year,
                                    ledger::ReportType type,
                                    const std::string& probi) = 0;

  virtual void GetPublisherBanner(
      const std::string& publisher_id,
      ledger::PublisherBannerCallback callback) = 0;

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

  virtual void HasSufficientBalanceToReconcile(
      HasSufficientBalanceToReconcileCallback callback) = 0;

  virtual void GetAddressesForPaymentId(
      ledger::WalletAddressesCallback callback) = 0;

  virtual void SetCatalogIssuers(const std::string& info) = 0;

  virtual void ConfirmAd(const std::string& info) = 0;
  virtual void GetTransactionHistory(
      GetTransactionHistoryCallback callback) = 0;
  virtual void GetRewardsInternalsInfo(ledger::RewardsInternalsInfo* info) = 0;

  virtual void GetRecurringTips(ledger::PublisherInfoListCallback callback) = 0;

  virtual void GetOneTimeTips(ledger::PublisherInfoListCallback callback) = 0;
  virtual void RefreshPublisher(
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback) = 0;

  virtual void StartMonthlyContribution() = 0;

  virtual void SaveMediaInfo(const std::string& type,
                             const std::map<std::string, std::string>& data,
                             ledger::PublisherInfoCallback callback) = 0;

  virtual void SetInlineTipSetting(const std::string& key, bool enabled) = 0;

  virtual bool GetInlineTipSetting(const std::string& key) = 0;

  virtual std::string GetShareURL(
      const std::string& type,
      const std::map<std::string, std::string>& args) = 0;

  virtual void GetPendingContributions(
      ledger::PendingContributionInfoListCallback callback) = 0;

  virtual void RemovePendingContribution(
      const std::string& publisher_key,
      const std::string& viewing_id,
      uint64_t added_date,
      const ledger::RemovePendingContributionCallback& callback) = 0;

  virtual void RemoveAllPendingContributions(
    const ledger::RemovePendingContributionCallback& callback) = 0;

  virtual void GetPendingContributionsTotal(
    const ledger::PendingContributionsTotalCallback& callback) = 0;

  virtual void FetchBalance(ledger::FetchBalanceCallback callback) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_H_
