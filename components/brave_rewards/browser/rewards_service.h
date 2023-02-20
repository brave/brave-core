/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/observer_list.h"
#include "base/types/expected.h"
#include "base/version.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/mojom_structs.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;

namespace content {
class NavigationHandle;
}

namespace brave_rewards {

class RewardsNotificationService;
class RewardsServiceObserver;

using GetPublisherInfoListCallback =
    base::OnceCallback<void(std::vector<ledger::mojom::PublisherInfoPtr> list)>;
using GetAutoContributionAmountCallback = base::OnceCallback<void(double)>;
using GetAutoContributePropertiesCallback =
    base::OnceCallback<void(ledger::mojom::AutoContributePropertiesPtr)>;
using GetPublisherMinVisitTimeCallback = base::OnceCallback<void(int)>;
using GetPublisherMinVisitsCallback = base::OnceCallback<void(int)>;
using GetPublisherAllowNonVerifiedCallback = base::OnceCallback<void(bool)>;
using GetPublisherAllowVideosCallback = base::OnceCallback<void(bool)>;
using GetAutoContributeEnabledCallback = base::OnceCallback<void(bool)>;
using GetReconcileStampCallback = base::OnceCallback<void(uint64_t)>;
using GetPendingContributionsTotalCallback = base::OnceCallback<void(double)>;
using GetRewardsInternalsInfoCallback =
    base::OnceCallback<void(ledger::mojom::RewardsInternalsInfoPtr info)>;
using GetRecurringTipsCallback =
    base::OnceCallback<void(std::vector<ledger::mojom::PublisherInfoPtr> list)>;
using GetOneTimeTipsCallback =
    base::OnceCallback<void(std::vector<ledger::mojom::PublisherInfoPtr> list)>;
using GetPublisherBannerCallback =
    base::OnceCallback<void(ledger::mojom::PublisherBannerPtr banner)>;
using RefreshPublisherCallback =
    base::OnceCallback<void(const ledger::mojom::PublisherStatus,
                            const std::string&)>;
using GetPublisherInfoCallback =
    base::OnceCallback<void(const ledger::mojom::Result,
                            ledger::mojom::PublisherInfoPtr)>;
using SavePublisherInfoCallback =
    base::OnceCallback<void(const ledger::mojom::Result)>;
using GetInlineTippingPlatformEnabledCallback = base::OnceCallback<void(bool)>;
using GetShareURLCallback = base::OnceCallback<void(const std::string&)>;
using GetPendingContributionsCallback = base::OnceCallback<void(
    std::vector<ledger::mojom::PendingContributionInfoPtr> list)>;
using FetchBalanceCallback =
    base::OnceCallback<void(const ledger::mojom::Result,
                            ledger::mojom::BalancePtr)>;
using GetExternalWalletResult =
    base::expected<ledger::mojom::ExternalWalletPtr,
                   ledger::mojom::GetExternalWalletError>;
using GetExternalWalletCallback =
    base::OnceCallback<void(GetExternalWalletResult)>;
using ConnectExternalWalletResult =
    base::expected<void, ledger::mojom::ConnectExternalWalletError>;
using ConnectExternalWalletCallback =
    base::OnceCallback<void(ConnectExternalWalletResult)>;
using ClaimPromotionCallback =
    base::OnceCallback<void(const ledger::mojom::Result,
                            const std::string&,
                            const std::string&,
                            const std::string&)>;
using AttestPromotionCallback =
    base::OnceCallback<void(const ledger::mojom::Result result,
                            ledger::mojom::PromotionPtr promotion)>;

using GetBalanceReportCallback =
    base::OnceCallback<void(const ledger::mojom::Result,
                            ledger::mojom::BalanceReportInfoPtr report)>;

using GetMonthlyReportCallback =
    base::OnceCallback<void(ledger::mojom::MonthlyReportInfoPtr report)>;

using GetAllMonthlyReportIdsCallback =
    base::OnceCallback<void(const std::vector<std::string>&)>;

using GetAllContributionsCallback = base::OnceCallback<void(
    std::vector<ledger::mojom::ContributionInfoPtr> contributions)>;

using GetAllPromotionsCallback =
    base::OnceCallback<void(std::vector<ledger::mojom::PromotionPtr> list)>;

using GetRewardsParametersCallback =
    base::OnceCallback<void(ledger::mojom::RewardsParametersPtr)>;

using LoadDiagnosticLogCallback = base::OnceCallback<void(const std::string&)>;

using ClearDiagnosticLogCallback = base::OnceCallback<void(const bool success)>;

using SuccessCallback = base::OnceCallback<void(const bool success)>;

using GetEventLogsCallback =
    base::OnceCallback<void(std::vector<ledger::mojom::EventLogPtr> logs)>;

using GetRewardsWalletCallback =
    base::OnceCallback<void(ledger::mojom::RewardsWalletPtr wallet)>;

using OnTipCallback = base::OnceCallback<void(ledger::mojom::Result)>;

using GetEnvironmentCallback =
    base::OnceCallback<void(ledger::mojom::Environment)>;

class RewardsService : public KeyedService {
 public:
  RewardsService();
  RewardsService(const RewardsService&) = delete;
  RewardsService& operator=(const RewardsService&) = delete;
  ~RewardsService() override;

  virtual bool IsInitialized() = 0;

  using CreateRewardsWalletCallback =
      base::OnceCallback<void(ledger::mojom::CreateRewardsWalletResult)>;

  // Creates a Rewards wallet for the current profile. If a Rewards wallet has
  // already been created, then the existing wallet information will be
  // returned. Ads and AC will be enabled if those prefs have not been
  // previously set.
  virtual void CreateRewardsWallet(const std::string& country,
                                   CreateRewardsWalletCallback callback) = 0;

  // Returns the country code associated with the user's Rewards profile.
  virtual std::string GetCountryCode() const = 0;

  // Returns the Rewards user type for the current profile.
  virtual void GetUserType(
      base::OnceCallback<void(ledger::mojom::UserType)> callback) = 0;

  using GetAvailableCountriesCallback =
      base::OnceCallback<void(std::vector<std::string>)>;

  // Asynchronously returns a vector of ISO country codes that the user can
  // select when creating a Rewards ID.
  virtual void GetAvailableCountries(
      GetAvailableCountriesCallback callback) const = 0;

  virtual void GetRewardsParameters(GetRewardsParametersCallback callback) = 0;
  virtual void GetActivityInfoList(const uint32_t start,
                                   const uint32_t limit,
                                   ledger::mojom::ActivityInfoFilterPtr filter,
                                   GetPublisherInfoListCallback callback) = 0;

  // Returns a count of publishers that a user has visited. This value is
  // displayed to unverified users to indicate the level of support they are
  // providing to the creator community.
  virtual void GetPublishersVisitedCount(
      base::OnceCallback<void(int)> callback) = 0;

  virtual void GetExcludedList(GetPublisherInfoListCallback callback) = 0;

  using FetchPromotionsCallback =
      base::OnceCallback<void(std::vector<ledger::mojom::PromotionPtr>)>;

  virtual void FetchPromotions(FetchPromotionsCallback callback) = 0;

  // Used by desktop
  virtual void ClaimPromotion(
      const std::string& promotion_id,
      ClaimPromotionCallback callback) = 0;
  // Used by Android
  virtual void ClaimPromotion(
      const std::string& promotion_id,
      AttestPromotionCallback callback) = 0;
  virtual void AttestPromotion(const std::string& promotion_id,
                               const std::string& solution,
                               AttestPromotionCallback callback) = 0;
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

  virtual void GetReconcileStamp(GetReconcileStampCallback callback) = 0;
  virtual void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) = 0;
  virtual void SetPublisherMinVisitTime(int duration_in_seconds) const = 0;
  virtual void GetPublisherMinVisits(
      GetPublisherMinVisitsCallback callback) = 0;
  virtual void SetPublisherMinVisits(int visits) const = 0;
  virtual void GetPublisherAllowNonVerified(
      GetPublisherAllowNonVerifiedCallback callback) = 0;
  virtual void SetPublisherAllowNonVerified(bool allow) const = 0;
  virtual void GetPublisherAllowVideos(
      GetPublisherAllowVideosCallback callback) = 0;
  virtual void SetPublisherAllowVideos(bool allow) const = 0;
  virtual void SetAutoContributionAmount(double amount) const = 0;
  virtual void GetAutoContributeEnabled(
      GetAutoContributeEnabledCallback callback) = 0;
  virtual void SetAutoContributeEnabled(bool enabled) = 0;

  virtual void GetBalanceReport(
      const uint32_t month,
      const uint32_t year,
      GetBalanceReportCallback callback) = 0;
  virtual void GetPublisherActivityFromUrl(
      uint64_t windowId,
      const std::string& url,
      const std::string& favicon_url,
      const std::string& publisher_blob) = 0;
  virtual void GetAutoContributionAmount(
      GetAutoContributionAmountCallback callback) = 0;
  virtual void GetPublisherBanner(const std::string& publisher_id,
                                  GetPublisherBannerCallback callback) = 0;
  virtual void OnTip(const std::string& publisher_key,
                     double amount,
                     bool recurring,
                     OnTipCallback callback) = 0;

  // Used in importer from muon days
  virtual void OnTip(const std::string& publisher_key,
                     double amount,
                     const bool recurring,
                     ledger::mojom::PublisherInfoPtr publisher) = 0;

  virtual void RemoveRecurringTip(const std::string& publisher_key) = 0;
  virtual void GetRecurringTips(GetRecurringTipsCallback callback) = 0;
  virtual void GetOneTimeTips(GetOneTimeTipsCallback callback) = 0;
  virtual void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) = 0;
  virtual RewardsNotificationService* GetNotificationService() const = 0;
  virtual void GetAutoContributeProperties(
      GetAutoContributePropertiesCallback callback) = 0;
  virtual void GetPendingContributionsTotal(
      GetPendingContributionsTotalCallback callback) = 0;
  virtual void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) = 0;

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

  virtual void SaveRecurringTip(const std::string& publisher_key,
                                double amount,
                                OnTipCallback callback) = 0;

  virtual const RewardsNotificationService::RewardsNotificationsMap&
  GetAllNotifications() = 0;

  virtual void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      const uint64_t duration,
      const bool firstVisit) = 0;

  virtual void IsPublisherRegistered(
      const std::string& publisher_id,
      base::OnceCallback<void(bool)> callback) = 0;

  virtual void GetPublisherInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) = 0;

  virtual void GetPublisherPanelInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) = 0;

  virtual void SavePublisherInfo(const uint64_t window_id,
                                 ledger::mojom::PublisherInfoPtr publisher_info,
                                 SavePublisherInfoCallback callback) = 0;

  virtual void SetInlineTippingPlatformEnabled(
      const std::string& key,
      bool enabled) = 0;

  virtual void GetInlineTippingPlatformEnabled(
      const std::string& key,
      GetInlineTippingPlatformEnabledCallback callback) = 0;

  virtual void GetShareURL(
      const base::flat_map<std::string, std::string>& args,
      GetShareURLCallback callback) = 0;

  virtual void FetchBalance(FetchBalanceCallback callback) = 0;

  virtual bool IsAutoContributeSupported() const = 0;

  virtual void GetExternalWallet(GetExternalWalletCallback callback) = 0;

  virtual std::string GetExternalWalletType() const = 0;

  virtual std::vector<std::string> GetExternalWalletProviders() const = 0;

  // Connects Rewards with a custodial wallet service (e.g. bitFlyer, Gemini,
  // Uphold).
  // |path| is the authorization URL's path
  // |query| is the authorization URL's query
  // The callback is called with a ConnectExternalWalletError on failure,
  // and with an empty result on success.
  virtual void ConnectExternalWallet(const std::string& path,
                                     const std::string& query,
                                     ConnectExternalWalletCallback) = 0;

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

  virtual void WriteDiagnosticLog(const std::string& file,
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

  virtual void GetRewardsWallet(GetRewardsWalletCallback callback) = 0;

  virtual void SetExternalWalletType(const std::string& wallet_type) = 0;

  virtual void GetEnvironment(GetEnvironmentCallback callback) = 0;

 protected:
  base::ObserverList<RewardsServiceObserver> observers_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_H_
