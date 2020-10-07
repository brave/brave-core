/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class Profile;

namespace content {
class NavigationHandle;
}

namespace brave_rewards {

bool IsMediaLink(const GURL& url,
                 const GURL& first_party_url,
                 const GURL& referrer);

class RewardsNotificationService;
class RewardsServiceObserver;
class RewardsServicePrivateObserver;

using GetPublisherInfoListCallback =
    base::Callback<void(ledger::type::PublisherInfoList list)>;
using GetAutoContributionAmountCallback = base::Callback<void(double)>;
using GetAutoContributePropertiesCallback = base::Callback<void(
    ledger::type::AutoContributePropertiesPtr)>;
using GetPublisherMinVisitTimeCallback = base::Callback<void(int)>;
using GetPublisherMinVisitsCallback = base::Callback<void(int)>;
using GetPublisherAllowNonVerifiedCallback = base::Callback<void(bool)>;
using GetPublisherAllowVideosCallback = base::Callback<void(bool)>;
using GetAutoContributeEnabledCallback = base::OnceCallback<void(bool)>;
using GetReconcileStampCallback = base::Callback<void(uint64_t)>;
using IsWalletCreatedCallback = base::Callback<void(bool)>;
using GetPendingContributionsTotalCallback = base::Callback<void(double)>;
using GetRewardsMainEnabledCallback = base::Callback<void(bool)>;
using GetRewardsInternalsInfoCallback =
    base::OnceCallback<void(ledger::type::RewardsInternalsInfoPtr info)>;
using SaveRecurringTipCallback = base::OnceCallback<void(bool)>;
using GetRecurringTipsCallback =
    base::OnceCallback<void(ledger::type::PublisherInfoList list)>;
using GetOneTimeTipsCallback =
    base::OnceCallback<void(ledger::type::PublisherInfoList list)>;
using GetPublisherBannerCallback =
    base::OnceCallback<void(ledger::type::PublisherBannerPtr banner)>;
using RefreshPublisherCallback =
    base::OnceCallback<void(
        const ledger::type::PublisherStatus,
        const std::string&)>;
using GetPublisherInfoCallback = base::OnceCallback<void(
    const ledger::type::Result,
    ledger::type::PublisherInfoPtr)>;
using SavePublisherInfoCallback =
    base::OnceCallback<void(const ledger::type::Result)>;
using SaveMediaInfoCallback =
    base::OnceCallback<void(ledger::type::PublisherInfoPtr publisher)>;
using GetInlineTippingPlatformEnabledCallback = base::OnceCallback<void(bool)>;
using GetShareURLCallback = base::OnceCallback<void(const std::string&)>;
using GetPendingContributionsCallback =
    base::OnceCallback<void(ledger::type::PendingContributionInfoList list)>;
using GetCurrentCountryCallback = base::OnceCallback<void(const std::string&)>;
using FetchBalanceCallback = base::OnceCallback<void(
    const ledger::type::Result,
    ledger::type::BalancePtr)>;
using GetUpholdWalletCallback = base::OnceCallback<void(
    const ledger::type::Result result,
    ledger::type::UpholdWalletPtr wallet)>;
using ProcessRewardsPageUrlCallback = base::OnceCallback<void(
    const ledger::type::Result result,
    const std::string&,
    const std::string&,
    const std::map<std::string, std::string>&)>;
using CreateWalletCallback =
    base::OnceCallback<void(const ledger::type::Result)>;
using ClaimPromotionCallback = base::OnceCallback<void(
    const ledger::type::Result,
    const std::string&,
    const std::string&,
    const std::string&)>;
using AttestPromotionCallback = base::OnceCallback<void(
    const ledger::type::Result result,
    ledger::type::PromotionPtr promotion)>;
using GetAnonWalletStatusCallback =
    base::OnceCallback<void(const ledger::type::Result)>;

using GetBalanceReportCallback = base::OnceCallback<void(
    const ledger::type::Result,
    ledger::type::BalanceReportInfoPtr report)>;

using GetMonthlyReportCallback =
    base::OnceCallback<void(ledger::type::MonthlyReportInfoPtr report)>;

using GetAllMonthlyReportIdsCallback =
    base::OnceCallback<void(const std::vector<std::string>&)>;

using GetAllContributionsCallback =
    base::OnceCallback<void(ledger::type::ContributionInfoList contributions)>;

using GetAllPromotionsCallback =
    base::OnceCallback<void(ledger::type::PromotionList list)>;

using GetRewardsParametersCallback =
    base::OnceCallback<void(ledger::type::RewardsParametersPtr)>;

using LoadDiagnosticLogCallback = base::OnceCallback<void(const std::string&)>;

using ClearDiagnosticLogCallback = base::OnceCallback<void(const bool success)>;

using SuccessCallback = base::OnceCallback<void(const bool success)>;

using GetEventLogsCallback =
    base::OnceCallback<void(ledger::type::EventLogs logs)>;

class RewardsService : public KeyedService {
 public:
  RewardsService();
  ~RewardsService() override;

  virtual bool IsInitialized() = 0;

  virtual void CreateWallet(CreateWalletCallback callback) = 0;
  virtual void GetRewardsParameters(GetRewardsParametersCallback callback) = 0;
  virtual void GetActivityInfoList(
      const uint32_t start,
      const uint32_t limit,
      ledger::type::ActivityInfoFilterPtr filter,
      const GetPublisherInfoListCallback& callback) = 0;
  virtual void GetExcludedList(
      const GetPublisherInfoListCallback& callback) = 0;
  virtual void FetchPromotions() = 0;
  // Used by desktop
  virtual void ClaimPromotion(
      const std::string& promotion_id,
      ClaimPromotionCallback callback) = 0;
  // Used by Android
  virtual void ClaimPromotion(
      const std::string& promotion_id,
      AttestPromotionCallback callback) = 0;
  virtual void AttestPromotion(
      const std::string& promotion_id,
      const std::string& solution,
      AttestPromotionCallback callback) = 0;
  virtual void RecoverWallet(const std::string& passPhrase) = 0;
  virtual void RestorePublishers() = 0;
  virtual void OnLoad(SessionID tab_id, const GURL& gurl) = 0;
  virtual void OnUnload(SessionID tab_id) = 0;
  virtual void OnShow(SessionID tab_id) = 0;
  virtual void OnHide(SessionID tab_id) = 0;
  virtual void OnForeground(SessionID tab_id) = 0;
  virtual void OnBackground(SessionID tab_id) = 0;
  virtual void OnXHRLoad(SessionID tab_id,
                         const GURL& url,
                         const GURL& first_party_url,
                         const GURL& referrer) = 0;
  virtual void OnPostData(SessionID tab_id,
                          const GURL& url,
                          const GURL& first_party_url,
                          const GURL& referrer,
                          const std::string& post_data) = 0;

  virtual void GetReconcileStamp(
      const GetReconcileStampCallback& callback) = 0;
  virtual void SetRewardsMainEnabled(bool enabled) = 0;
  virtual void GetPublisherMinVisitTime(
      const GetPublisherMinVisitTimeCallback& callback) = 0;
  virtual void SetPublisherMinVisitTime(int duration_in_seconds) const = 0;
  virtual void GetPublisherMinVisits(
      const GetPublisherMinVisitsCallback& callback) = 0;
  virtual void SetPublisherMinVisits(int visits) const = 0;
  virtual void GetPublisherAllowNonVerified(
      const GetPublisherAllowNonVerifiedCallback& callback) = 0;
  virtual void SetPublisherAllowNonVerified(bool allow) const = 0;
  virtual void GetPublisherAllowVideos(
      const GetPublisherAllowVideosCallback& callback) = 0;
  virtual void SetPublisherAllowVideos(bool allow) const = 0;
  virtual void SetAutoContributionAmount(double amount) const = 0;
  virtual void GetAutoContributeEnabled(
      GetAutoContributeEnabledCallback callback) = 0;
  virtual void SetAutoContributeEnabled(bool enabled) = 0;
  virtual void GetBalanceReport(
      const uint32_t month,
      const uint32_t year,
      GetBalanceReportCallback callback) = 0;
  virtual void IsWalletCreated(const IsWalletCreatedCallback& callback) = 0;
  virtual void GetPublisherActivityFromUrl(
      uint64_t windowId,
      const std::string& url,
      const std::string& favicon_url,
      const std::string& publisher_blob) = 0;
  virtual void GetAutoContributionAmount(
      const GetAutoContributionAmountCallback& callback) = 0;
  virtual void GetPublisherBanner(const std::string& publisher_id,
                                  GetPublisherBannerCallback callback) = 0;
  virtual void OnTip(
      const std::string& publisher_key,
      const double amount,
      const bool recurring) = 0;

  // Used in importer from muon days
  virtual void OnTip(
      const std::string& publisher_key,
      double amount,
      const bool recurring,
      ledger::type::PublisherInfoPtr publisher) = 0;

  virtual void RemoveRecurringTip(const std::string& publisher_key) = 0;
  virtual void GetRecurringTips(GetRecurringTipsCallback callback) = 0;
  virtual void GetOneTimeTips(GetOneTimeTipsCallback callback) = 0;
  virtual void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) = 0;
  virtual RewardsNotificationService* GetNotificationService() const = 0;
  virtual void SetBackupCompleted() = 0;
  virtual void GetAutoContributeProperties(
    const GetAutoContributePropertiesCallback& callback) = 0;
  virtual void GetPendingContributionsTotal(
    const GetPendingContributionsTotalCallback& callback) = 0;
  virtual void GetRewardsMainEnabled(
    const GetRewardsMainEnabledCallback& callback) const = 0;
  virtual void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) = 0;
  virtual void AddPrivateObserver(
      RewardsServicePrivateObserver* observer) = 0;
  virtual void RemovePrivateObserver(
      RewardsServicePrivateObserver* observer) = 0;
  virtual void OnAdsEnabled(bool ads_enabled) = 0;

  virtual void RefreshPublisher(
      const std::string& publisher_key,
      RefreshPublisherCallback callback) = 0;

  virtual void GetPendingContributions(
    GetPendingContributionsCallback callback) = 0;

  virtual void RemovePendingContribution(const uint64_t id) = 0;
  virtual void RemoveAllPendingContributions() = 0;

  void AddObserver(RewardsServiceObserver* observer);
  void RemoveObserver(RewardsServiceObserver* observer);

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  virtual void SaveRecurringTip(
      const std::string& publisher_key,
      const double amount,
      SaveRecurringTipCallback callback) = 0;

  virtual const RewardsNotificationService::RewardsNotificationsMap&
  GetAllNotifications() = 0;

  virtual void SaveInlineMediaInfo(
      const std::string& media_type,
      const std::map<std::string, std::string>& args,
      SaveMediaInfoCallback callback) = 0;

  virtual void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      const uint64_t duration,
      const bool firstVisit) = 0;

  virtual void GetPublisherInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) = 0;

  virtual void GetPublisherPanelInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) = 0;

  virtual void SavePublisherInfo(
      const uint64_t window_id,
      ledger::type::PublisherInfoPtr publisher_info,
      SavePublisherInfoCallback callback) = 0;

  virtual void SetInlineTippingPlatformEnabled(
      const std::string& key,
      bool enabled) = 0;

  virtual void GetInlineTippingPlatformEnabled(
      const std::string& key,
      GetInlineTippingPlatformEnabledCallback callback) = 0;

  virtual void GetShareURL(
      const std::string& type,
      const std::map<std::string, std::string>& args,
      GetShareURLCallback callback) = 0;

  virtual void FetchBalance(FetchBalanceCallback callback) = 0;

  virtual void GetUpholdWallet(GetUpholdWalletCallback callback) = 0;

  virtual void ProcessRewardsPageUrl(
      const std::string& path,
      const std::string& query,
      ProcessRewardsPageUrlCallback callback) = 0;

  virtual void DisconnectWallet(const std::string& wallet_type) = 0;

  virtual bool OnlyAnonWallet() = 0;

  virtual void GetAnonWalletStatus(GetAnonWalletStatusCallback callback) = 0;

  virtual void GetMonthlyReport(
      const uint32_t month,
      const uint32_t year,
      GetMonthlyReportCallback callback) = 0;

  virtual void GetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback) = 0;

  virtual void GetAllContributions(
      GetAllContributionsCallback callback) = 0;

  virtual void GetAllPromotions(
      GetAllPromotionsCallback callback) = 0;

  virtual void DiagnosticLog(
      const std::string& file,
      const int line,
      const int verbose_level,
      const std::string& message) = 0;

  virtual void LoadDiagnosticLog(
      const int num_lines,
      LoadDiagnosticLogCallback callback) = 0;

  virtual void ClearDiagnosticLog(
      ClearDiagnosticLogCallback callback) = 0;

  virtual void CompleteReset(SuccessCallback callback) = 0;

  virtual void GetEventLogs(GetEventLogsCallback callback) = 0;

 protected:
  base::ObserverList<RewardsServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RewardsService);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_
