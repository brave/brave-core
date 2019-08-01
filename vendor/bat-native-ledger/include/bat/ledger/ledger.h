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
#include "bat/ledger/media_event_info.h"
#include "bat/ledger/transactions_info.h"
#include "bat/ledger/rewards_internals_info.h"
#include "bat/ledger/pending_contribution.h"

namespace ledger {

using VisitData = ledger::mojom::VisitData;
using VisitDataPtr = ledger::mojom::VisitDataPtr;

extern bool is_production;
extern bool is_debug;
extern bool is_testing;
extern int reconcile_time;  // minutes
extern bool short_retries;

using PublisherBannerCallback =
    std::function<void(ledger::PublisherBannerPtr banner)>;
using GetTransactionHistoryCallback =
    std::function<void(std::unique_ptr<ledger::TransactionsInfo> info)>;
using OnWalletPropertiesCallback = std::function<void(const ledger::Result,
                                  ledger::WalletPropertiesPtr)>;
using OnRefreshPublisherCallback =
    std::function<void(ledger::PublisherStatus)>;
using HasSufficientBalanceToReconcileCallback = std::function<void(bool)>;
using FetchBalanceCallback = std::function<void(ledger::Result,
                                                ledger::BalancePtr)>;
using ExternalWalletCallback = std::function<void(
    ledger::Result,
    ledger::ExternalWalletPtr)>;
using ExternalWalletAuthorizationCallback =
    std::function<void(ledger::Result, std::map<std::string, std::string>)>;
using DisconnectWalletCallback = std::function<void(ledger::Result)>;
using TransferAnonToExternalWalletCallback =
    std::function<void(ledger::Result)>;
using DoDirectTipCallback = std::function<void(ledger::Result)>;
using FetchGrantsCallback =
    std::function<void(ledger::Result, std::vector<ledger::GrantPtr>)>;
using SetPublisherExcludeCallback = std::function<void(ledger::Result)>;
using GetGrantCaptchaCallback = std::function<void(const std::string&,
                                                   const std::string&)>;
using RewardsInternalsInfoCallback =
    std::function<void(ledger::RewardsInternalsInfoPtr)>;
using CreateWalletCallback = std::function<void(ledger::Result)>;
using InitializeCallback = std::function<void(ledger::Result)>;

using GetBalanceReportCallback =
    std::function<void(bool, ledger::BalanceReportInfoPtr)>;

using RecoverWalletCallback = std::function<void(
    const ledger::Result,
    const double balance,
    std::vector<ledger::GrantPtr>)>;

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

  virtual void Initialize(InitializeCallback) = 0;

  // returns false if wallet initialization is already in progress
  virtual void CreateWallet(const std::string& safetynet_token,
                            CreateWalletCallback callback) = 0;

  virtual void DoDirectTip(const std::string& publisher_key,
                           int amount,
                           const std::string& currency,
                           ledger::DoDirectTipCallback callback) = 0;

  virtual void OnLoad(VisitDataPtr visit_data,
                      const uint64_t& current_time) = 0;

  virtual void OnUnload(uint32_t tab_id, const uint64_t& current_time) = 0;

  virtual void OnShow(uint32_t tab_id, const uint64_t& current_time) = 0;

  virtual void OnHide(uint32_t tab_id, const uint64_t& current_time) = 0;

  virtual void OnForeground(uint32_t tab_id, const uint64_t& current_time) = 0;

  virtual void OnBackground(uint32_t tab_id, const uint64_t& current_time) = 0;

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

  virtual void GetPublisherInfo(const std::string& publisher_key,
                                PublisherInfoCallback callback) = 0;

  virtual void GetActivityInfoList(uint32_t start, uint32_t limit,
                                    ledger::ActivityInfoFilterPtr filter,
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
                           const std::string& paymentId,
                           const std::string& safetynet_token,
                           ledger::FetchGrantsCallback callback) const = 0;

  virtual void SolveGrantCaptcha(const std::string& solution,
                                 const std::string& promotionId) const = 0;

  virtual void GetGrantCaptcha(
      const std::vector<std::string>& headers,
      GetGrantCaptchaCallback callback) const = 0;

  virtual void ApplySafetynetToken(const std::string& promotion_id,
      const std::string& token) const = 0;

  virtual void GetGrantViaSafetynetCheck(const std::string& promotion_id) const = 0;

  virtual std::string GetWalletPassphrase() const = 0;

  virtual void GetBalanceReport(
      ACTIVITY_MONTH month,
      int year,
      ledger::GetBalanceReportCallback callback) const = 0;

  virtual std::map<std::string, ledger::BalanceReportInfoPtr>
  GetAllBalanceReports() const = 0;

  virtual ledger::AutoContributePropsPtr GetAutoContributeProps() = 0;

  virtual void RecoverWallet(
      const std::string& pass_phrase,
      RecoverWalletCallback callback)  = 0;

  virtual void SetPublisherExclude(
      const std::string& publisher_id,
      const ledger::PUBLISHER_EXCLUDE& exclude,
      SetPublisherExcludeCallback callback) = 0;

  virtual void RestorePublishers(
    ledger::RestorePublishersCallback callback) = 0;

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
      const ledger::RewardsCategory category,
      const std::string& probi,
      const ledger::ACTIVITY_MONTH month,
      const int year,
      const uint32_t date) = 0;

  virtual void RemoveRecurringTip(
    const std::string& publisher_key,
    RemoveRecurringTipCallback callback) = 0;

  virtual double GetDefaultContributionAmount() = 0;

  virtual uint64_t GetBootStamp() const = 0;

  virtual void HasSufficientBalanceToReconcile(
      HasSufficientBalanceToReconcileCallback callback) = 0;

  virtual void SetCatalogIssuers(const std::string& info) = 0;

  virtual void ConfirmAd(const std::string& info) = 0;
  virtual void ConfirmAction(const std::string& uuid,
                             const std::string& creative_set_id,
                             const std::string& type) = 0;
  virtual void GetTransactionHistory(
      GetTransactionHistoryCallback callback) = 0;

  // This uses a callback instead of returning directly so that
  // GetCurrentReconciles() can be moved to the database later.
  virtual void GetRewardsInternalsInfo(
      ledger::RewardsInternalsInfoCallback callback) = 0;

  virtual void SaveRecurringTip(
      ledger::ContributionInfoPtr info,
      ledger::SaveRecurringTipCallback callback) = 0;
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
      ledger::RemovePendingContributionCallback callback) = 0;

  virtual void RemoveAllPendingContributions(
      ledger::RemovePendingContributionCallback callback) = 0;

  virtual void GetPendingContributionsTotal(
      ledger::PendingContributionsTotalCallback callback) = 0;

  virtual void FetchBalance(ledger::FetchBalanceCallback callback) = 0;

  virtual void GetExternalWallet(const std::string& wallet_type,
                                 ledger::ExternalWalletCallback callback) = 0;

  virtual void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback) = 0;

  virtual void DisconnectWallet(
      const std::string& wallet_type,
      ledger::DisconnectWalletCallback callback) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_H_
