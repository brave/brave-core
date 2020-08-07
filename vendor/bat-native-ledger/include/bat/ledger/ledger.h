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
#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

extern Environment _environment;
extern bool is_debug;
extern bool is_testing;
extern int reconcile_interval;  // minutes
extern bool short_retries;

using SearchPublisherPrefixListCallback = std::function<void(bool)>;
using PublisherPrefixListUpdatedCallback = std::function<void()>;
using PublisherBannerCallback = std::function<void(PublisherBannerPtr banner)>;
using GetRewardsParametersCallback = std::function<void(RewardsParametersPtr)>;
using OnRefreshPublisherCallback = std::function<void(PublisherStatus)>;
using HasSufficientBalanceToReconcileCallback = std::function<void(bool)>;
using FetchBalanceCallback = std::function<void(Result, BalancePtr)>;
using ExternalWalletCallback = std::function<void(Result, ExternalWalletPtr)>;
using ExternalWalletAuthorizationCallback =
    std::function<void(Result, std::map<std::string, std::string>)>;
using FetchPromotionCallback = std::function<void(Result, PromotionList)>;
using ClaimPromotionCallback =
    std::function<void(const Result, const std::string&)>;
using RewardsInternalsInfoCallback =
    std::function<void(RewardsInternalsInfoPtr)>;
using AttestPromotionCallback =
    std::function<void(const Result, PromotionPtr promotion)>;
using GetBalanceReportCallback =
    std::function<void(const Result, BalanceReportInfoPtr)>;
using GetBalanceReportListCallback = std::function<void(BalanceReportInfoList)>;
using ContributionInfoListCallback = std::function<void(ContributionInfoList)>;
using GetMonthlyReportCallback =
    std::function<void(const Result, MonthlyReportInfoPtr)>;
using GetAllMonthlyReportIdsCallback =
    std::function<void(const std::vector<std::string>&)>;
using GetEventLogsCallback = std::function<void(EventLogs)>;

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

  virtual void Initialize(
      const bool execute_create_script,
      ResultCallback) = 0;

  // returns false if wallet initialization is already in progress
  virtual void CreateWallet(ResultCallback callback) = 0;

  virtual void OneTimeTip(
      const std::string& publisher_key,
      const double amount,
      ResultCallback callback) = 0;

  virtual void OnLoad(
      VisitDataPtr visit_data,
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

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual void GetActivityInfoList(
      uint32_t start, uint32_t limit,
      ActivityInfoFilterPtr filter,
      PublisherInfoListCallback callback) = 0;

  virtual void GetExcludedList(PublisherInfoListCallback callback) = 0;

  virtual void SetRewardsMainEnabled(bool enabled) = 0;

  virtual void SetPublisherMinVisitTime(int duration_in_seconds) = 0;

  virtual void SetPublisherMinVisits(int visits) = 0;

  virtual void SetPublisherAllowNonVerified(bool allow) = 0;

  virtual void SetPublisherAllowVideos(bool allow) = 0;

  virtual void SetAutoContributionAmount(double amount) = 0;

  virtual void SetAutoContributeEnabled(bool enabled) = 0;

  virtual uint64_t GetReconcileStamp() = 0;

  virtual bool GetRewardsMainEnabled() = 0;

  virtual int GetPublisherMinVisitTime() = 0;  // In milliseconds

  virtual int GetPublisherMinVisits() = 0;

  virtual bool GetPublisherAllowNonVerified() = 0;

  virtual bool GetPublisherAllowVideos() = 0;

  virtual double GetAutoContributionAmount() = 0;

  virtual bool GetAutoContributeEnabled() = 0;

  virtual void GetRewardsParameters(GetRewardsParametersCallback callback) = 0;

  virtual void FetchPromotions(FetchPromotionCallback callback) const = 0;

  // |payload|:
  // desktop and Android: empty
  // iOS: { "publicKey": "{{publicKey}}" }
  // =====================================
  // |callback| returns result as json
  // desktop: { "captchaImage": "{{captchaImage}}", "hint": "{{hint}}" }
  // iOS and Android: { "nonce": "{{nonce}}" }
  virtual void ClaimPromotion(
      const std::string& promotion_id,
      const std::string& payload,
      ClaimPromotionCallback callback) const = 0;

  // |solution|:
  // desktop:
  //  {
  //    "captchaId": "{{captchaId}}",
  //    "x": "{{x}}",
  //    "y": "{{y}}"
  //  }
  // iOS:
  //  {
  //    "nonce": "{{nonce}}",
  //    "blob": "{{blob}}",
  //    "signature": "{{signature}}"
  //  }
  // android:
  //  {
  //    "nonce": "{{nonce}}",
  //    "token": "{{token}}"
  //  }
  virtual void AttestPromotion(
      const std::string& promotion_id,
      const std::string& solution,
      AttestPromotionCallback callback) const = 0;

  virtual std::string GetWalletPassphrase() const = 0;

  virtual void GetBalanceReport(
      ActivityMonth month,
      int year,
      GetBalanceReportCallback callback) const = 0;

  virtual void GetAllBalanceReports(
      GetBalanceReportListCallback callback) const = 0;

  virtual AutoContributePropertiesPtr GetAutoContributeProperties() = 0;

  virtual void RecoverWallet(
      const std::string& pass_phrase,
      ResultCallback callback)  = 0;

  virtual void SetPublisherExclude(
      const std::string& publisher_id,
      const PublisherExclude& exclude,
      ResultCallback callback) = 0;

  virtual void RestorePublishers(ResultCallback callback) = 0;

  virtual bool IsWalletCreated() = 0;

  virtual void GetPublisherActivityFromUrl(
      uint64_t windowId,
      VisitDataPtr visit_data,
      const std::string& publisher_blob) = 0;

  virtual void GetPublisherBanner(
      const std::string& publisher_id,
      PublisherBannerCallback callback) = 0;

  virtual void RemoveRecurringTip(
      const std::string& publisher_key,
      ResultCallback callback) = 0;

  virtual uint64_t GetCreationStamp() = 0;

  virtual void HasSufficientBalanceToReconcile(
      HasSufficientBalanceToReconcileCallback callback) = 0;

  virtual void GetRewardsInternalsInfo(
      RewardsInternalsInfoCallback callback) = 0;

  virtual void SaveRecurringTip(
      RecurringTipPtr info,
      ResultCallback callback) = 0;

  virtual void GetRecurringTips(PublisherInfoListCallback callback) = 0;

  virtual void GetOneTimeTips(PublisherInfoListCallback callback) = 0;
  virtual void RefreshPublisher(
      const std::string& publisher_key,
      OnRefreshPublisherCallback callback) = 0;

  virtual void StartMonthlyContribution() = 0;

  virtual void SaveMediaInfo(
      const std::string& type,
      const std::map<std::string, std::string>& data,
      PublisherInfoCallback callback) = 0;

  virtual void SetInlineTippingPlatformEnabled(
      const InlineTipsPlatforms platform,
      bool enabled) = 0;

  virtual bool GetInlineTippingPlatformEnabled(
      const InlineTipsPlatforms platform) = 0;

  virtual std::string GetShareURL(
      const std::string& type,
      const std::map<std::string, std::string>& args) = 0;

  virtual void GetPendingContributions(
      PendingContributionInfoListCallback callback) = 0;

  virtual void RemovePendingContribution(
      const uint64_t id,
      ResultCallback callback) = 0;

  virtual void RemoveAllPendingContributions(ResultCallback callback) = 0;

  virtual void GetPendingContributionsTotal(
      PendingContributionsTotalCallback callback) = 0;

  virtual void FetchBalance(FetchBalanceCallback callback) = 0;

  virtual void GetExternalWallet(
      const std::string& wallet_type,
      ExternalWalletCallback callback) = 0;

  virtual void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ExternalWalletAuthorizationCallback callback) = 0;

  virtual void DisconnectWallet(
      const std::string& wallet_type,
      ResultCallback callback) = 0;

  virtual void GetAllPromotions(GetAllPromotionsCallback callback) = 0;

  virtual void GetAnonWalletStatus(ResultCallback callback) = 0;

  virtual void GetTransactionReport(
      const ActivityMonth month,
      const int year,
      GetTransactionReportCallback callback) = 0;

  virtual void GetContributionReport(
      const ActivityMonth month,
      const int year,
      GetContributionReportCallback callback) = 0;

  virtual void GetAllContributions(ContributionInfoListCallback callback) = 0;

  virtual void SavePublisherInfo(
      PublisherInfoPtr info,
      ResultCallback callback) = 0;

  virtual void GetMonthlyReport(
      const ActivityMonth month,
      const int year,
      GetMonthlyReportCallback callback) = 0;

  virtual void GetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback) = 0;

  virtual void ProcessSKU(
      const std::vector<SKUOrderItem>& items,
      ExternalWalletPtr wallet,
      SKUOrderCallback callback) = 0;

  virtual void Shutdown(ResultCallback callback) = 0;

  virtual void GetEventLogs(GetEventLogsCallback callback) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_H_
