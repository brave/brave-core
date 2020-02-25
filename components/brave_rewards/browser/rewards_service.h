/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/components/brave_rewards/browser/auto_contribution_props.h"
#include "brave/components/brave_rewards/browser/balance.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/external_wallet.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
#include "brave/components/brave_rewards/browser/pending_contribution.h"
#include "brave/components/brave_rewards/browser/promotion.h"
#include "brave/components/brave_rewards/browser/rewards_internals_info.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/monthly_report.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class Profile;

namespace ads {
struct IssuersInfo;
struct AdNotificationInfo;
}

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

using GetContentSiteListCallback =
    base::Callback<void(std::unique_ptr<ContentSiteList>)>;
using GetWalletPassphraseCallback = base::Callback<void(const std::string&)>;
using GetContributionAmountCallback = base::Callback<void(double)>;
using GetAutoContributePropsCallback = base::Callback<void(
    std::unique_ptr<brave_rewards::AutoContributeProps>)>;
using GetPublisherMinVisitTimeCallback = base::Callback<void(uint64_t)>;
using GetPublisherMinVisitsCallback = base::Callback<void(uint32_t)>;
using GetPublisherAllowNonVerifiedCallback = base::Callback<void(bool)>;
using GetPublisherAllowVideosCallback = base::Callback<void(bool)>;
using GetAutoContributeCallback = base::OnceCallback<void(bool)>;
using GetReconcileStampCallback = base::Callback<void(uint64_t)>;
using IsWalletCreatedCallback = base::Callback<void(bool)>;
using GetPendingContributionsTotalCallback = base::Callback<void(double)>;
using GetRewardsMainEnabledCallback = base::Callback<void(bool)>;
using GetTransactionHistoryCallback =
    base::OnceCallback<void(double, uint64_t, uint64_t)>;
using GetRewardsInternalsInfoCallback = base::OnceCallback<void(
    std::unique_ptr<brave_rewards::RewardsInternalsInfo>)>;
using SaveRecurringTipCallback = base::OnceCallback<void(bool)>;
using GetRecurringTipsCallback = base::OnceCallback<void(
    std::unique_ptr<brave_rewards::ContentSiteList>)>;
using GetOneTimeTipsCallback = base::OnceCallback<void(
    std::unique_ptr<brave_rewards::ContentSiteList>)>;
using GetPublisherBannerCallback =
    base::OnceCallback<void(std::unique_ptr<brave_rewards::PublisherBanner>)>;
using RefreshPublisherCallback =
    base::OnceCallback<void(uint32_t, const std::string&)>;
using SaveMediaInfoCallback =
    base::OnceCallback<void(std::unique_ptr<brave_rewards::ContentSite>)>;
using GetInlineTipSettingCallback = base::OnceCallback<void(bool)>;
using GetShareURLCallback = base::OnceCallback<void(const std::string&)>;
using GetPendingContributionsCallback = base::OnceCallback<void(
    std::unique_ptr<brave_rewards::PendingContributionInfoList>)>;
using GetCurrentCountryCallback = base::OnceCallback<void(const std::string&)>;
using FetchBalanceCallback = base::OnceCallback<void(
    int32_t,
    std::unique_ptr<brave_rewards::Balance>)>;
using GetExternalWalletCallback = base::OnceCallback<void(
    int32_t result,
    std::unique_ptr<brave_rewards::ExternalWallet> wallet)>;
using ProcessRewardsPageUrlCallback = base::OnceCallback<void(
    int32_t result,
    const std::string&,
    const std::string&,
    const std::map<std::string, std::string>&)>;
using CreateWalletCallback = base::OnceCallback<void(int32_t)>;
using ClaimPromotionCallback = base::OnceCallback<void(
    const int32_t,
    const std::string&,
    const std::string&,
    const std::string&)>;
using AttestPromotionCallback = base::OnceCallback<void(
    const int32_t,
    std::unique_ptr<brave_rewards::Promotion> promotion)>;
using GetAnonWalletStatusCallback = base::OnceCallback<void(const uint32_t)>;

using GetBalanceReportCallback = base::OnceCallback<void(
    const int32_t,
    const brave_rewards::BalanceReport&)>;

using GetMonthlyReportCallback = base::OnceCallback<void(
    const MonthlyReport&)>;

class RewardsService : public KeyedService {
 public:
  RewardsService();
  ~RewardsService() override;

  virtual void CreateWallet(CreateWalletCallback callback) = 0;
  virtual void FetchWalletProperties() = 0;
  virtual void GetContentSiteList(
      uint32_t start,
      uint32_t limit,
      uint64_t min_visit_time,
      uint64_t reconcile_stamp,
      bool allow_non_verified,
      uint32_t min_visits,
      const GetContentSiteListCallback& callback) = 0;
  virtual void GetExcludedList(const GetContentSiteListCallback& callback) = 0;
  virtual void FetchPromotions() = 0;
  // Used by desktop
  virtual void ClaimPromotion(ClaimPromotionCallback callback) = 0;
  // Used by Android
  virtual void ClaimPromotion(
      const std::string& promotion_id,
      AttestPromotionCallback callback) = 0;
  virtual void AttestPromotion(
      const std::string& promotion_id,
      const std::string& solution,
      AttestPromotionCallback callback) = 0;
  virtual void GetWalletPassphrase(
      const GetWalletPassphraseCallback& callback) = 0;
  virtual void RecoverWallet(const std::string& passPhrase) = 0;
  virtual void RestorePublishersUI() = 0;
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
  virtual void SetPublisherMinVisitTime(uint64_t duration_in_seconds) const = 0;
  virtual void GetPublisherMinVisits(
      const GetPublisherMinVisitsCallback& callback) = 0;
  virtual void SetPublisherMinVisits(unsigned int visits) const = 0;
  virtual void GetPublisherAllowNonVerified(
      const GetPublisherAllowNonVerifiedCallback& callback) = 0;
  virtual void SetPublisherAllowNonVerified(bool allow) const = 0;
  virtual void GetPublisherAllowVideos(
      const GetPublisherAllowVideosCallback& callback) = 0;
  virtual void SetPublisherAllowVideos(bool allow) const = 0;
  virtual void SetContributionAmount(double amount) const = 0;
  virtual void SetUserChangedContribution() const = 0;
  virtual void GetAutoContribute(
      GetAutoContributeCallback callback) = 0;
  virtual void SetAutoContribute(bool enabled) = 0;
  virtual void UpdateAdsRewards() const = 0;
  virtual void SetTimer(uint64_t time_offset, uint32_t* timer_id) = 0;
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
  virtual void GetContributionAmount(
      const GetContributionAmountCallback& callback) = 0;
  virtual void GetPublisherBanner(const std::string& publisher_id,
                                  GetPublisherBannerCallback callback) = 0;
  virtual void OnTip(const std::string& publisher_key,
                     double amount,
                     bool recurring) = 0;
  virtual void OnTip(const std::string& publisher_key, double amount,
      bool recurring, std::unique_ptr<brave_rewards::ContentSite> site) = 0;
  virtual void RemoveRecurringTipUI(const std::string& publisher_key) = 0;
  virtual void GetRecurringTipsUI(GetRecurringTipsCallback callback) = 0;
  virtual void GetOneTimeTipsUI(GetOneTimeTipsCallback callback) = 0;
  virtual void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) = 0;
  virtual RewardsNotificationService* GetNotificationService() const = 0;
  virtual bool CheckImported() = 0;
  virtual void SetBackupCompleted() = 0;
  virtual void GetAutoContributeProps(
    const GetAutoContributePropsCallback& callback) = 0;
  virtual void GetPendingContributionsTotalUI(
    const GetPendingContributionsTotalCallback& callback) = 0;
  virtual void GetRewardsMainEnabled(
    const GetRewardsMainEnabledCallback& callback) const = 0;
  // TODO(Terry Mancey): remove this hack when ads is moved to the same process
  // as ledger
  virtual void SetCatalogIssuers(const std::string& json) = 0;
  virtual void ConfirmAdNotification(
      const std::string& json) = 0;
  virtual void ConfirmAction(const std::string& creative_instance_id,
                             const std::string& creative_set_id,
                             const std::string& confirmation_type) = 0;
  virtual void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) = 0;
  virtual void AddPrivateObserver(
      RewardsServicePrivateObserver* observer) = 0;
  virtual void RemovePrivateObserver(
      RewardsServicePrivateObserver* observer) = 0;
  virtual void GetTransactionHistory(
      GetTransactionHistoryCallback callback) = 0;
  virtual void OnAdsEnabled(bool ads_enabled) = 0;

  virtual void RefreshPublisher(
      const std::string& publisher_key,
      RefreshPublisherCallback callback) = 0;

  virtual void GetPendingContributionsUI(
    GetPendingContributionsCallback callback) = 0;

  virtual void RemovePendingContributionUI(const uint64_t id) = 0;
  virtual void RemoveAllPendingContributionsUI() = 0;
  virtual void ResetTheWholeState(
      const base::Callback<void(bool)>& callback) = 0;

  void AddObserver(RewardsServiceObserver* observer);
  void RemoveObserver(RewardsServiceObserver* observer);

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  virtual void SaveRecurringTipUI(const std::string& publisher_key,
                                  const double amount,
                                  SaveRecurringTipCallback callback) = 0;

  virtual const RewardsNotificationService::RewardsNotificationsMap&
  GetAllNotifications() = 0;

  virtual void SaveInlineMediaInfo(
      const std::string& media_type,
      const std::map<std::string, std::string>& args,
      SaveMediaInfoCallback callback) = 0;

  virtual void SetInlineTipSetting(const std::string& key, bool enabled) = 0;

  virtual void GetInlineTipSetting(
      const std::string& key,
      GetInlineTipSettingCallback callback) = 0;

  virtual void GetShareURL(
      const std::string& type,
      const std::map<std::string, std::string>& args,
      GetShareURLCallback callback) = 0;

  virtual void FetchBalance(FetchBalanceCallback callback) = 0;

  virtual void GetExternalWallet(const std::string& wallet_type,
                                 GetExternalWalletCallback callback) = 0;

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

 protected:
  base::ObserverList<RewardsServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RewardsService);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_
