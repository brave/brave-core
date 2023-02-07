/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "base/types/expected.h"
#include "bat/ledger/export.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/mojom_structs.h"
#include "bat/ledger/public/interfaces/ledger_types.mojom.h"

namespace ledger {

extern mojom::Environment _environment;
extern bool is_debug;
extern bool is_testing;
extern int state_migration_target_version_for_testing;
extern int reconcile_interval;  // minutes
extern int retry_interval;      // seconds

using PublisherBannerCallback = std::function<void(mojom::PublisherBannerPtr)>;

using GetRewardsParametersCallback =
    base::OnceCallback<void(mojom::RewardsParametersPtr)>;

using CreateRewardsWalletCallback =
    base::OnceCallback<void(mojom::CreateRewardsWalletResult)>;

using OnRefreshPublisherCallback = std::function<void(mojom::PublisherStatus)>;

using FetchBalanceCallback =
    base::OnceCallback<void(mojom::Result, mojom::BalancePtr)>;

using GetExternalWalletResult =
    base::expected<mojom::ExternalWalletPtr, mojom::GetExternalWalletError>;

using GetExternalWalletCallback =
    base::OnceCallback<void(GetExternalWalletResult)>;

using ConnectExternalWalletResult =
    base::expected<void, mojom::ConnectExternalWalletError>;

using ConnectExternalWalletCallback =
    base::OnceCallback<void(ConnectExternalWalletResult)>;

using FetchPromotionCallback =
    base::OnceCallback<void(mojom::Result, std::vector<mojom::PromotionPtr>)>;

using ClaimPromotionCallback =
    base::OnceCallback<void(mojom::Result, const std::string&)>;

using RewardsInternalsInfoCallback =
    std::function<void(mojom::RewardsInternalsInfoPtr)>;

using AttestPromotionCallback =
    base::OnceCallback<void(mojom::Result, mojom::PromotionPtr)>;

using GetBalanceReportCallback =
    std::function<void(mojom::Result, mojom::BalanceReportInfoPtr)>;

using GetBalanceReportListCallback =
    std::function<void(std::vector<mojom::BalanceReportInfoPtr>)>;

using ContributionInfoListCallback =
    std::function<void(std::vector<mojom::ContributionInfoPtr>)>;

using GetMonthlyReportCallback =
    std::function<void(mojom::Result, mojom::MonthlyReportInfoPtr)>;

using GetAllMonthlyReportIdsCallback =
    std::function<void(const std::vector<std::string>&)>;

using GetEventLogsCallback =
    std::function<void(std::vector<mojom::EventLogPtr>)>;

using SKUOrderCallback = std::function<void(mojom::Result, const std::string&)>;

using GetContributionReportCallback =
    std::function<void(std::vector<mojom::ContributionReportInfoPtr>)>;

using GetTransactionReportCallback =
    std::function<void(std::vector<mojom::TransactionReportInfoPtr>)>;

using GetAllPromotionsCallback =
    std::function<void(base::flat_map<std::string, mojom::PromotionPtr>)>;

using LegacyResultCallback = std::function<void(mojom::Result)>;

using ResultCallback = base::OnceCallback<void(mojom::Result)>;

using PendingContributionsTotalCallback = std::function<void(double)>;

using PendingContributionInfoListCallback =
    std::function<void(std::vector<mojom::PendingContributionInfoPtr>)>;

using UnverifiedPublishersCallback =
    std::function<void(std::vector<std::string>&&)>;

using PublisherInfoListCallback =
    std::function<void(std::vector<mojom::PublisherInfoPtr>)>;

using PublisherInfoCallback =
    std::function<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetPublisherInfoCallback =
    std::function<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetRewardsWalletCallback = std::function<void(mojom::RewardsWalletPtr)>;

using PostSuggestionsClaimCallback =
    base::OnceCallback<void(mojom::Result result, std::string drain_id)>;

using GetDrainCallback =
    std::function<void(mojom::Result result, mojom::DrainStatus status)>;

class LEDGER_EXPORT Ledger {
 public:
  Ledger() = default;
  virtual ~Ledger() = default;

  Ledger(const Ledger&) = delete;
  Ledger& operator=(const Ledger&) = delete;

  static Ledger* CreateInstance(LedgerClient* client);

  virtual void Initialize(bool execute_create_script, LegacyResultCallback) = 0;

  virtual void CreateRewardsWallet(const std::string& country,
                                   CreateRewardsWalletCallback callback) = 0;

  virtual void OneTimeTip(const std::string& publisher_key,
                          double amount,
                          LegacyResultCallback callback) = 0;

  virtual void OnLoad(mojom::VisitDataPtr visit_data,
                      uint64_t current_time) = 0;

  virtual void OnUnload(uint32_t tab_id, uint64_t current_time) = 0;

  virtual void OnShow(uint32_t tab_id, uint64_t current_time) = 0;

  virtual void OnHide(uint32_t tab_id, uint64_t current_time) = 0;

  virtual void OnForeground(uint32_t tab_id, uint64_t current_time) = 0;

  virtual void OnBackground(uint32_t tab_id, uint64_t current_time) = 0;

  virtual void OnXHRLoad(uint32_t tab_id,
                         const std::string& url,
                         const base::flat_map<std::string, std::string>& parts,
                         const std::string& first_party_url,
                         const std::string& referrer,
                         mojom::VisitDataPtr visit_data) = 0;

  virtual void GetActivityInfoList(uint32_t start,
                                   uint32_t limit,
                                   mojom::ActivityInfoFilterPtr filter,
                                   PublisherInfoListCallback callback) = 0;

  virtual void GetPublishersVisitedCount(
      base::OnceCallback<void(int)> callback) = 0;

  virtual void GetExcludedList(PublisherInfoListCallback callback) = 0;

  virtual void SetPublisherMinVisitTime(int duration_in_seconds) = 0;

  virtual void SetPublisherMinVisits(int visits) = 0;

  virtual void SetPublisherAllowNonVerified(bool allow) = 0;

  virtual void SetPublisherAllowVideos(bool allow) = 0;

  virtual void SetAutoContributionAmount(double amount) = 0;

  virtual void SetAutoContributeEnabled(bool enabled) = 0;

  virtual uint64_t GetReconcileStamp() = 0;

  virtual int GetPublisherMinVisitTime() = 0;  // In milliseconds

  virtual int GetPublisherMinVisits() = 0;

  virtual bool GetPublisherAllowNonVerified() = 0;

  virtual bool GetPublisherAllowVideos() = 0;

  virtual double GetAutoContributionAmount() = 0;

  virtual bool GetAutoContributeEnabled() = 0;

  virtual void GetRewardsParameters(GetRewardsParametersCallback callback) = 0;

  virtual void FetchPromotions(FetchPromotionCallback callback) = 0;

  // |payload|:
  // desktop and Android: empty
  // iOS: { "publicKey": "{{publicKey}}" }
  // =====================================
  // |callback| returns result as json
  // desktop: { "captchaImage": "{{captchaImage}}", "hint": "{{hint}}" }
  // iOS and Android: { "nonce": "{{nonce}}" }
  virtual void ClaimPromotion(const std::string& promotion_id,
                              const std::string& payload,
                              ClaimPromotionCallback callback) = 0;

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
  virtual void AttestPromotion(const std::string& promotion_id,
                               const std::string& solution,
                               AttestPromotionCallback callback) = 0;

  virtual void GetBalanceReport(mojom::ActivityMonth month,
                                int year,
                                GetBalanceReportCallback callback) = 0;

  virtual void GetAllBalanceReports(GetBalanceReportListCallback callback) = 0;

  virtual mojom::AutoContributePropertiesPtr GetAutoContributeProperties() = 0;

  virtual void SetPublisherExclude(const std::string& publisher_id,
                                   mojom::PublisherExclude exclude,
                                   ResultCallback callback) = 0;

  virtual void RestorePublishers(ResultCallback callback) = 0;

  virtual void GetPublisherActivityFromUrl(
      uint64_t window_id,
      mojom::VisitDataPtr visit_data,
      const std::string& publisher_blob) = 0;

  virtual void GetPublisherBanner(const std::string& publisher_id,
                                  PublisherBannerCallback callback) = 0;

  virtual void RemoveRecurringTip(const std::string& publisher_key,
                                  LegacyResultCallback callback) = 0;

  virtual uint64_t GetCreationStamp() = 0;

  virtual void GetRewardsInternalsInfo(
      RewardsInternalsInfoCallback callback) = 0;

  virtual void SaveRecurringTip(mojom::RecurringTipPtr info,
                                LegacyResultCallback callback) = 0;

  virtual void GetRecurringTips(PublisherInfoListCallback callback) = 0;

  virtual void GetOneTimeTips(PublisherInfoListCallback callback) = 0;

  virtual void RefreshPublisher(const std::string& publisher_key,
                                OnRefreshPublisherCallback callback) = 0;

  virtual void StartMonthlyContribution() = 0;

  virtual void UpdateMediaDuration(uint64_t window_id,
                                   const std::string& publisher_key,
                                   uint64_t duration,
                                   bool first_visit) = 0;

  virtual void IsPublisherRegistered(const std::string& publisher_id,
                                     std::function<void(bool)> callback) = 0;

  virtual void GetPublisherInfo(const std::string& publisher_key,
                                GetPublisherInfoCallback callback) = 0;

  virtual void GetPublisherPanelInfo(const std::string& publisher_key,
                                     GetPublisherInfoCallback callback) = 0;

  virtual void SavePublisherInfo(uint64_t window_id,
                                 mojom::PublisherInfoPtr publisher_info,
                                 LegacyResultCallback callback) = 0;

  virtual void SetInlineTippingPlatformEnabled(
      mojom::InlineTipsPlatforms platform,
      bool enabled) = 0;

  virtual bool GetInlineTippingPlatformEnabled(
      mojom::InlineTipsPlatforms platform) = 0;

  virtual std::string GetShareURL(
      const base::flat_map<std::string, std::string>& args) = 0;

  virtual void GetPendingContributions(
      PendingContributionInfoListCallback callback) = 0;

  virtual void RemovePendingContribution(uint64_t id,
                                         LegacyResultCallback callback) = 0;

  virtual void RemoveAllPendingContributions(LegacyResultCallback callback) = 0;

  virtual void GetPendingContributionsTotal(
      PendingContributionsTotalCallback callback) = 0;

  virtual void FetchBalance(FetchBalanceCallback callback) = 0;

  virtual void GetExternalWallet(const std::string& wallet_type,
                                 GetExternalWalletCallback) = 0;

  virtual void ConnectExternalWallet(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ConnectExternalWalletCallback) = 0;

  virtual void GetAllPromotions(GetAllPromotionsCallback callback) = 0;

  virtual void GetTransactionReport(mojom::ActivityMonth month,
                                    int year,
                                    GetTransactionReportCallback callback) = 0;

  virtual void GetContributionReport(
      mojom::ActivityMonth month,
      int year,
      GetContributionReportCallback callback) = 0;

  virtual void GetAllContributions(ContributionInfoListCallback callback) = 0;

  virtual void SavePublisherInfoForTip(mojom::PublisherInfoPtr info,
                                       LegacyResultCallback callback) = 0;

  virtual void GetMonthlyReport(mojom::ActivityMonth month,
                                int year,
                                GetMonthlyReportCallback callback) = 0;

  virtual void GetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback) = 0;

  virtual void ProcessSKU(const std::vector<mojom::SKUOrderItem>& items,
                          const std::string& wallet_type,
                          SKUOrderCallback callback) = 0;

  virtual void Shutdown(LegacyResultCallback callback) = 0;

  virtual void GetEventLogs(GetEventLogsCallback callback) = 0;

  virtual void GetRewardsWallet(GetRewardsWalletCallback callback) = 0;

  virtual void GetDrainStatus(const std::string& drain_id,
                              GetDrainCallback callback) = 0;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_H_
