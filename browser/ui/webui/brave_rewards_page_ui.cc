/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_page_ui.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "bat/ads/supported_subdivisions.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_page_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/locale_util.h"
#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/bindings_policy.h"
#include "ui/base/l10n/l10n_util.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_rewards/rewards_panel/rewards_panel_coordinator.h"
#include "chrome/browser/ui/browser_finder.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/webui/favicon_source.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "content/public/browser/url_data_source.h"
#endif

using content::WebUIMessageHandler;

namespace {

#if !BUILDFLAG(IS_ANDROID)

brave_rewards::RewardsPanelCoordinator* GetPanelCoordinator(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  if (auto* browser = chrome::FindBrowserWithWebContents(web_contents)) {
    return brave_rewards::RewardsPanelCoordinator::FromBrowser(browser);
  }
  return nullptr;
}

#endif

// The handler for Javascript messages for Brave about: pages
class RewardsDOMHandler
    : public WebUIMessageHandler,
      public brave_ads::AdsServiceObserver,
      public brave_rewards::RewardsNotificationServiceObserver,
      public brave_rewards::RewardsServiceObserver {
 public:
  RewardsDOMHandler();
  RewardsDOMHandler(const RewardsDOMHandler&) = delete;
  RewardsDOMHandler& operator=(const RewardsDOMHandler&) = delete;
  ~RewardsDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;
  void RegisterMessages() override;

 private:
  void RestartBrowser(const base::Value::List& args);
  void IsInitialized(const base::Value::List& args);
  void GetUserVersion(const base::Value::List& args);
  void GetRewardsParameters(const base::Value::List& args);
  void GetAutoContributeProperties(const base::Value::List& args);
  void FetchPromotions(const base::Value::List& args);
  void ClaimPromotion(const base::Value::List& args);
  void GetReconcileStamp(const base::Value::List& args);
  void SaveSetting(const base::Value::List& args);
  void OnPublisherList(std::vector<ledger::mojom::PublisherInfoPtr> list);
  void OnExcludedSiteList(std::vector<ledger::mojom::PublisherInfoPtr> list);
  void ExcludePublisher(const base::Value::List& args);
  void RestorePublishers(const base::Value::List& args);
  void RestorePublisher(const base::Value::List& args);
  void GetAutoContributionAmount(const base::Value::List& args);
  void RemoveRecurringTip(const base::Value::List& args);
  void GetRecurringTips(const base::Value::List& args);
  void GetOneTimeTips(const base::Value::List& args);
  void GetContributionList(const base::Value::List& args);
  void GetAdsData(const base::Value::List& args);
  void GetAdsHistory(const base::Value::List& args);
  void OnGetAdsHistory(base::Value::List history);
  void ToggleAdThumbUp(const base::Value::List& args);
  void OnToggleAdThumbUp(base::Value::Dict dict);
  void ToggleAdThumbDown(const base::Value::List& args);
  void OnToggleAdThumbDown(base::Value::Dict dict);
  void ToggleAdOptIn(const base::Value::List& args);
  void OnToggleAdOptIn(const std::string& category, const int action);
  void ToggleAdOptOut(const base::Value::List& args);
  void OnToggleAdOptOut(const std::string& category, const int action);
  void ToggleSavedAd(const base::Value::List& args);
  void OnToggleSavedAd(base::Value::Dict dict);
  void ToggleFlaggedAd(const base::Value::List& args);
  void OnToggleFlaggedAd(base::Value::Dict dict);
  void SaveAdsSetting(const base::Value::List& args);
  void OnGetContributionAmount(double amount);
  void OnGetAutoContributeProperties(
      ledger::mojom::AutoContributePropertiesPtr properties);
  void OnGetReconcileStamp(uint64_t reconcile_stamp);
  void OnAutoContributePropsReady(
      ledger::mojom::AutoContributePropertiesPtr properties);
  void GetPendingContributionsTotal(const base::Value::List& args);
  void OnGetPendingContributionsTotal(double amount);
  void GetStatement(const base::Value::List& args);
  void OnGetStatement(ads::mojom::StatementInfoPtr statement);
  void GetExcludedSites(const base::Value::List& args);

  void OnGetRecurringTips(std::vector<ledger::mojom::PublisherInfoPtr> list);

  void OnGetOneTimeTips(std::vector<ledger::mojom::PublisherInfoPtr> list);

  void GetEnabledInlineTippingPlatforms(const base::Value::List& args);
  void SetInlineTippingPlatformEnabled(const base::Value::List& args);

  void GetPendingContributions(const base::Value::List& args);
  void OnGetPendingContributions(
      std::vector<ledger::mojom::PendingContributionInfoPtr> list);
  void RemovePendingContribution(const base::Value::List& args);
  void RemoveAllPendingContributions(const base::Value::List& args);
  void FetchBalance(const base::Value::List& args);
  void OnFetchBalance(const ledger::mojom::Result result,
                      ledger::mojom::BalancePtr balance);

  void GetExternalWallet(const base::Value::List& args);
  void OnGetExternalWallet(brave_rewards::GetExternalWalletResult);

  void ConnectExternalWallet(const base::Value::List& args);
  void OnConnectExternalWallet(brave_rewards::ConnectExternalWalletResult);

  void GetBalanceReport(const base::Value::List& args);

  void OnGetBalanceReport(const uint32_t month,
                          const uint32_t year,
                          const ledger::mojom::Result result,
                          ledger::mojom::BalanceReportInfoPtr report);

  void GetMonthlyReport(const base::Value::List& args);

  void GetAllMonthlyReportIds(const base::Value::List& args);
  void GetCountryCode(const base::Value::List& args);

  void OnGetMonthlyReport(const uint32_t month,
                          const uint32_t year,
                          ledger::mojom::MonthlyReportInfoPtr report);

  void OnGetAllMonthlyReportIds(const std::vector<std::string>& ids);

  void OnGetRewardsParameters(ledger::mojom::RewardsParametersPtr parameters);

  void CompleteReset(const base::Value::List& args);

  void GetOnboardingStatus(const base::Value::List& args);
  void EnableRewards(const base::Value::List& args);
  void GetExternalWalletProviders(const base::Value::List& args);
  void SetExternalWalletType(const base::Value::List& args);

  void OnExternalWalletTypeUpdated(brave_rewards::GetExternalWalletResult);
  void GetIsUnsupportedRegion(const base::Value::List& args);

  void GetPluralString(const base::Value::List& args);

  // RewardsServiceObserver implementation
  void OnRewardsInitialized(
      brave_rewards::RewardsService* rewards_service) override;
  void OnFetchPromotions(
      brave_rewards::RewardsService* rewards_service,
      const ledger::mojom::Result result,
      const std::vector<ledger::mojom::PromotionPtr>& list) override;
  void OnPromotionFinished(brave_rewards::RewardsService* rewards_service,
                           const ledger::mojom::Result result,
                           ledger::mojom::PromotionPtr promotion) override;
  void OnExcludedSitesChanged(brave_rewards::RewardsService* rewards_service,
                              std::string publisher_id,
                              bool excluded) override;
  void OnReconcileComplete(
      brave_rewards::RewardsService* rewards_service,
      const ledger::mojom::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::mojom::RewardsType type,
      const ledger::mojom::ContributionProcessor processor) override;

  void OnPendingContributionSaved(
      brave_rewards::RewardsService* rewards_service,
      const ledger::mojom::Result result) override;

  void OnPublisherListNormalized(
      brave_rewards::RewardsService* rewards_service,
      std::vector<ledger::mojom::PublisherInfoPtr> list) override;

  void OnStatementChanged(
      brave_rewards::RewardsService* rewards_service) override;

  void OnRecurringTipSaved(brave_rewards::RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(brave_rewards::RewardsService* rewards_service,
                             bool success) override;

  void OnPendingContributionRemoved(
      brave_rewards::RewardsService* rewards_service,
      const ledger::mojom::Result result) override;

  void OnExternalWalletLoggedOut() override;

  void OnRewardsWalletUpdated() override;

  void OnUnblindedTokensReady(
      brave_rewards::RewardsService* rewards_service) override;

  void ReconcileStampReset() override;

  void OnCompleteReset(const bool success) override;

  // RewardsNotificationsServiceObserver implementation
  void OnNotificationAdded(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
          notification) override;
  void OnNotificationDeleted(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
          notification) override;
  void OnAllNotificationsDeleted(brave_rewards::RewardsNotificationService*
                                     rewards_notification_service) override;
  void OnGetNotification(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
          notification) override;
  void OnGetAllNotifications(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotificationsList&
          notifications_list) override;

  // AdsServiceObserver implementation
  void OnAdRewardsDidChange() override;
  void OnNeedsBrowserUpgradeToServeAds() override;

  void InitPrefChangeRegistrar();
  void OnPrefChanged(const std::string& key);

  raw_ptr<brave_rewards::RewardsService> rewards_service_ =
      nullptr;                                            // NOT OWNED
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;  // NOT OWNED

  base::ScopedObservation<brave_rewards::RewardsService,
                          brave_rewards::RewardsServiceObserver>
      rewards_service_observation_{this};
  base::ScopedObservation<brave_ads::AdsService, brave_ads::AdsServiceObserver>
      ads_service_observation_{this};

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<RewardsDOMHandler> weak_factory_;
};

namespace {

constexpr int kDaysOfAdsHistory = 30;

constexpr char kAdsSubdivisionTargeting[] = "adsSubdivisionTargeting";
constexpr char kAutoDetectedSubdivisionTargeting[] =
    "automaticallyDetectedAdsSubdivisionTargeting";

}  // namespace

RewardsDOMHandler::RewardsDOMHandler() : weak_factory_(this) {}

RewardsDOMHandler::~RewardsDOMHandler() = default;

void RewardsDOMHandler::RegisterMessages() {
#if BUILDFLAG(IS_ANDROID)
  // Create our favicon data source.
  Profile* profile = Profile::FromWebUI(web_ui());
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFaviconLegacy));
#endif

  web_ui()->RegisterMessageCallback(
      "brave_rewards.restartBrowser",
      base::BindRepeating(&RewardsDOMHandler::RestartBrowser,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.isInitialized",
      base::BindRepeating(&RewardsDOMHandler::IsInitialized,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getUserVersion",
      base::BindRepeating(&RewardsDOMHandler::GetUserVersion,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getRewardsParameters",
      base::BindRepeating(&RewardsDOMHandler::GetRewardsParameters,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getAutoContributeProperties",
      base::BindRepeating(&RewardsDOMHandler::GetAutoContributeProperties,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.fetchPromotions",
      base::BindRepeating(&RewardsDOMHandler::FetchPromotions,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.claimPromotion",
      base::BindRepeating(&RewardsDOMHandler::ClaimPromotion,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getReconcileStamp",
      base::BindRepeating(&RewardsDOMHandler::GetReconcileStamp,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.saveSetting",
      base::BindRepeating(&RewardsDOMHandler::SaveSetting,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.excludePublisher",
      base::BindRepeating(&RewardsDOMHandler::ExcludePublisher,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.restorePublishers",
      base::BindRepeating(&RewardsDOMHandler::RestorePublishers,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.restorePublisher",
      base::BindRepeating(&RewardsDOMHandler::RestorePublisher,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getContributionAmount",
      base::BindRepeating(&RewardsDOMHandler::GetAutoContributionAmount,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.removeRecurringTip",
      base::BindRepeating(&RewardsDOMHandler::RemoveRecurringTip,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getRecurringTips",
      base::BindRepeating(&RewardsDOMHandler::GetRecurringTips,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getOneTimeTips",
      base::BindRepeating(&RewardsDOMHandler::GetOneTimeTips,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getContributionList",
      base::BindRepeating(&RewardsDOMHandler::GetContributionList,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getAdsData",
      base::BindRepeating(&RewardsDOMHandler::GetAdsData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getAdsHistory",
      base::BindRepeating(&RewardsDOMHandler::GetAdsHistory,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.toggleAdThumbUp",
      base::BindRepeating(&RewardsDOMHandler::ToggleAdThumbUp,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.toggleAdThumbDown",
      base::BindRepeating(&RewardsDOMHandler::ToggleAdThumbDown,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.toggleAdOptIn",
      base::BindRepeating(&RewardsDOMHandler::ToggleAdOptIn,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.toggleAdOptOut",
      base::BindRepeating(&RewardsDOMHandler::ToggleAdOptOut,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.toggleSavedAd",
      base::BindRepeating(&RewardsDOMHandler::ToggleSavedAd,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.toggleFlaggedAd",
      base::BindRepeating(&RewardsDOMHandler::ToggleFlaggedAd,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.saveAdsSetting",
      base::BindRepeating(&RewardsDOMHandler::SaveAdsSetting,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getPendingContributionsTotal",
      base::BindRepeating(&RewardsDOMHandler::GetPendingContributionsTotal,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getStatement",
      base::BindRepeating(&RewardsDOMHandler::GetStatement,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getEnabledInlineTippingPlatforms",
      base::BindRepeating(&RewardsDOMHandler::GetEnabledInlineTippingPlatforms,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.setInlineTippingPlatformEnabled",
      base::BindRepeating(&RewardsDOMHandler::SetInlineTippingPlatformEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getPendingContributions",
      base::BindRepeating(&RewardsDOMHandler::GetPendingContributions,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.removePendingContribution",
      base::BindRepeating(&RewardsDOMHandler::RemovePendingContribution,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.removeAllPendingContribution",
      base::BindRepeating(&RewardsDOMHandler::RemoveAllPendingContributions,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getExcludedSites",
      base::BindRepeating(&RewardsDOMHandler::GetExcludedSites,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.fetchBalance",
      base::BindRepeating(&RewardsDOMHandler::FetchBalance,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getExternalWallet",
      base::BindRepeating(&RewardsDOMHandler::GetExternalWallet,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.connectExternalWallet",
      base::BindRepeating(&RewardsDOMHandler::ConnectExternalWallet,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getBalanceReport",
      base::BindRepeating(&RewardsDOMHandler::GetBalanceReport,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getMonthlyReport",
      base::BindRepeating(&RewardsDOMHandler::GetMonthlyReport,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getMonthlyReportIds",
      base::BindRepeating(&RewardsDOMHandler::GetAllMonthlyReportIds,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getCountryCode",
      base::BindRepeating(&RewardsDOMHandler::GetCountryCode,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.completeReset",
      base::BindRepeating(&RewardsDOMHandler::CompleteReset,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getOnboardingStatus",
      base::BindRepeating(&RewardsDOMHandler::GetOnboardingStatus,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.enableRewards",
      base::BindRepeating(&RewardsDOMHandler::EnableRewards,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getExternalWalletProviders",
      base::BindRepeating(&RewardsDOMHandler::GetExternalWalletProviders,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.setExternalWalletType",
      base::BindRepeating(&RewardsDOMHandler::SetExternalWalletType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getIsUnsupportedRegion",
      base::BindRepeating(&RewardsDOMHandler::GetIsUnsupportedRegion,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getPluralString",
      base::BindRepeating(&RewardsDOMHandler::GetPluralString,
                          base::Unretained(this)));
}

void RewardsDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());

  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);

  // Configure a pref change registrar to update brave://rewards when settings
  // are changed via brave://settings
  InitPrefChangeRegistrar();
}

void RewardsDOMHandler::InitPrefChangeRegistrar() {
  Profile* profile = Profile::FromWebUI(web_ui());
  pref_change_registrar_.Init(profile->GetPrefs());

  pref_change_registrar_.Add(
      ads::prefs::kEnabled,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      ads::prefs::kMaximumNotificationAdsPerHour,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      ads::prefs::kSubdivisionTargetingCode,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_rewards::prefs::kAutoContributeEnabled,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_rewards::prefs::kAutoContributeAmount,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_rewards::prefs::kMinVisitTime,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_rewards::prefs::kMinVisits,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_rewards::prefs::kAllowNonVerified,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_rewards::prefs::kAllowVideoContribution,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_rewards::prefs::kInlineTipTwitterEnabled,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_rewards::prefs::kInlineTipRedditEnabled,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_rewards::prefs::kInlineTipGithubEnabled,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
}

void RewardsDOMHandler::OnPrefChanged(const std::string& path) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onPrefChanged", base::Value(path));
}

void RewardsDOMHandler::RestartBrowser(const base::Value::List& args) {
  AllowJavascript();
  chrome::AttemptRestart();
}

void RewardsDOMHandler::IsInitialized(const base::Value::List& args) {
  AllowJavascript();

  if (rewards_service_ && rewards_service_->IsInitialized()) {
    CallJavascriptFunction("brave_rewards.initialized");
  }
}

void RewardsDOMHandler::GetUserVersion(const base::Value::List&) {
  if (!IsJavascriptAllowed() || !rewards_service_) {
    return;
  }
  base::Version version = rewards_service_->GetUserVersion();
  CallJavascriptFunction("brave_rewards.userVersion",
                         base::Value(version.GetString()));
}

void RewardsDOMHandler::OnJavascriptAllowed() {
  if (rewards_service_) {
    rewards_service_observation_.Reset();
    rewards_service_observation_.Observe(rewards_service_);
  }

  if (ads_service_) {
    ads_service_observation_.Reset();
    ads_service_observation_.Observe(ads_service_);
  }
}

void RewardsDOMHandler::OnJavascriptDisallowed() {
  rewards_service_observation_.Reset();
  ads_service_observation_.Reset();

  weak_factory_.InvalidateWeakPtrs();
}

void RewardsDOMHandler::GetRewardsParameters(const base::Value::List& args) {
  if (!rewards_service_)
    return;

  AllowJavascript();

  rewards_service_->GetRewardsParameters(base::BindOnce(
      &RewardsDOMHandler::OnGetRewardsParameters, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetRewardsParameters(
    ledger::mojom::RewardsParametersPtr parameters) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  double rate = 0.0, auto_contribute_choice = 0.0;
  base::Value::List auto_contribute_choices;
  base::Value::Dict payout_status;
  base::Value::Dict wallet_provider_regions;
  if (parameters) {
    rate = parameters->rate;
    auto_contribute_choice = parameters->auto_contribute_choice;
    for (double const& item : parameters->auto_contribute_choices) {
      auto_contribute_choices.Append(item);
    }
    for (const auto& [key, value] : parameters->payout_status) {
      payout_status.Set(key, value);
    }

    for (const auto& [wallet_provider, regions] :
         parameters->wallet_provider_regions) {
      base::Value::List allow;
      for (const auto& country : regions->allow) {
        allow.Append(country);
      }

      base::Value::List block;
      for (const auto& country : regions->block) {
        block.Append(country);
      }

      base::Value::Dict regions_dict;
      regions_dict.Set("allow", std::move(allow));
      regions_dict.Set("block", std::move(block));

      wallet_provider_regions.Set(wallet_provider, std::move(regions_dict));
    }
  }

  data.Set("rate", rate);
  data.Set("autoContributeChoice", auto_contribute_choice);
  data.Set("autoContributeChoices", std::move(auto_contribute_choices));
  data.Set("payoutStatus", std::move(payout_status));
  data.Set("walletProviderRegions", std::move(wallet_provider_regions));

  CallJavascriptFunction("brave_rewards.rewardsParameters",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::OnRewardsInitialized(
    brave_rewards::RewardsService* rewards_service) {
  if (!IsJavascriptAllowed())
    return;

  CallJavascriptFunction("brave_rewards.initialized");
}

void RewardsDOMHandler::GetAutoContributeProperties(
    const base::Value::List& args) {
  if (!rewards_service_)
    return;

  AllowJavascript();

  rewards_service_->GetAutoContributeProperties(
      base::BindOnce(&RewardsDOMHandler::OnGetAutoContributeProperties,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::SetExternalWalletType(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_)
    return;

  AllowJavascript();
  const std::string wallet_type = args[0].GetString();
  rewards_service_->SetExternalWalletType(wallet_type);

  rewards_service_->GetExternalWallet(
      base::BindOnce(&RewardsDOMHandler::OnExternalWalletTypeUpdated,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnExternalWalletTypeUpdated(
    brave_rewards::GetExternalWalletResult result) {
  if (IsJavascriptAllowed()) {
    auto wallet = std::move(result).value_or(nullptr);
    CallJavascriptFunction("brave_rewards.externalWalletLogin",
                           base::Value(wallet ? wallet->login_url : ""));
  }
}

void RewardsDOMHandler::OnGetAutoContributeProperties(
    ledger::mojom::AutoContributePropertiesPtr properties) {
  if (!IsJavascriptAllowed() || !properties)
    return;

  base::Value::Dict values;
  values.Set("enabledContribute", properties->enabled_contribute);
  values.Set("contributionMinTime",
             static_cast<int>(properties->contribution_min_time));
  values.Set("contributionMinVisits", properties->contribution_min_visits);
  values.Set("contributionNonVerified", properties->contribution_non_verified);
  values.Set("contributionVideos", properties->contribution_videos);

  CallJavascriptFunction("brave_rewards.autoContributeProperties",
                         base::Value(std::move(values)));
}

void RewardsDOMHandler::OnFetchPromotions(
    brave_rewards::RewardsService* rewards_service,
    const ledger::mojom::Result result,
    const std::vector<ledger::mojom::PromotionPtr>& list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::List promotions;
  for (const auto& item : list) {
    base::Value::Dict dict;
    dict.Set("promotionId", item->id);
    dict.Set("type", static_cast<int>(item->type));
    dict.Set("status", static_cast<int>(item->status));
    dict.Set("createdAt", static_cast<double>(item->created_at));
    dict.Set("claimableUntil", static_cast<double>(item->claimable_until));
    dict.Set("expiresAt", static_cast<double>(item->expires_at));
    dict.Set("amount", item->approximate_value);
    promotions.Append(std::move(dict));
  }

  base::Value::Dict dict;
  dict.Set("result", static_cast<int>(result));
  dict.Set("promotions", std::move(promotions));

  CallJavascriptFunction("brave_rewards.promotions",
                         base::Value(std::move(dict)));
}

void RewardsDOMHandler::FetchPromotions(const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->FetchPromotions(base::DoNothing());
  }
}

void RewardsDOMHandler::ClaimPromotion(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  const std::string promotion_id = args[0].GetString();

#if !BUILDFLAG(IS_ANDROID)
  if (auto* coordinator = GetPanelCoordinator(web_ui()->GetWebContents())) {
    coordinator->ShowGrantCaptcha(promotion_id);
  }
#else
  // Notify the UI that the claim process for this promotion has started.
  CallJavascriptFunction("brave_rewards.promotionClaimStarted",
                         base::Value(promotion_id));

  // No need for a callback. The UI receives "brave_rewards.promotionFinish".
  brave_rewards::AttestPromotionCallback callback = base::DoNothing();
  rewards_service_->ClaimPromotion(promotion_id, std::move(callback));
#endif
}

void RewardsDOMHandler::OnPromotionFinished(
    brave_rewards::RewardsService* rewards_service,
    const ledger::mojom::Result result,
    ledger::mojom::PromotionPtr promotion) {
  if (result != ledger::mojom::Result::LEDGER_OK) {
    return;
  }

  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict promotion_dict;
  if (promotion) {
    promotion_dict.Set("promotionId", promotion->id);
    promotion_dict.Set("expiresAt", static_cast<double>(promotion->expires_at));
    promotion_dict.Set("amount", promotion->approximate_value);
    promotion_dict.Set("type", static_cast<int>(promotion->type));
  }

  base::Value::Dict finish;
  finish.Set("result", static_cast<int>(result));
  finish.Set("promotion", std::move(promotion_dict));

  CallJavascriptFunction("brave_rewards.promotionFinish",
                         base::Value(std::move(finish)));
}

void RewardsDOMHandler::OnGetReconcileStamp(uint64_t reconcile_stamp) {
  if (IsJavascriptAllowed()) {
    std::string stamp = std::to_string(reconcile_stamp);
    CallJavascriptFunction("brave_rewards.reconcileStamp", base::Value(stamp));
  }
}

void RewardsDOMHandler::GetReconcileStamp(const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->GetReconcileStamp(base::BindOnce(
        &RewardsDOMHandler::OnGetReconcileStamp, weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnAutoContributePropsReady(
    ledger::mojom::AutoContributePropertiesPtr properties) {
  if (!properties) {
    return;
  }

  auto filter = ledger::mojom::ActivityInfoFilter::New();
  auto pair =
      ledger::mojom::ActivityInfoFilterOrderPair::New("ai.percent", false);
  filter->order_by.push_back(std::move(pair));
  filter->min_duration = properties->contribution_min_time;
  filter->reconcile_stamp = properties->reconcile_stamp;
  filter->excluded = ledger::mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED;
  filter->percent = 1;
  filter->non_verified = properties->contribution_non_verified;
  filter->min_visits = properties->contribution_min_visits;

  rewards_service_->GetActivityInfoList(
      0, 0, std::move(filter),
      base::BindOnce(&RewardsDOMHandler::OnPublisherList,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetExcludedSites(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  rewards_service_->GetExcludedList(base::BindOnce(
      &RewardsDOMHandler::OnExcludedSiteList, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnExcludedSitesChanged(
    brave_rewards::RewardsService* rewards_service,
    std::string publisher_id,
    bool excluded) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.excludedSiteChanged");
}

void RewardsDOMHandler::OnNotificationAdded(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) {}

void RewardsDOMHandler::OnNotificationDeleted(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) {
#if BUILDFLAG(IS_ANDROID)
  if (notification.type_ == brave_rewards::RewardsNotificationService::
                                REWARDS_NOTIFICATION_GRANT &&
      IsJavascriptAllowed()) {
    base::Value::Dict finish;
    finish.Set("status", false);
    finish.Set("expiryTime", 0);
    finish.Set("probi", "0");

    CallJavascriptFunction("brave_rewards.grantFinish",
                           base::Value(std::move(finish)));
  }
#endif
}

void RewardsDOMHandler::OnAllNotificationsDeleted(
    brave_rewards::RewardsNotificationService* rewards_notification_service) {}

void RewardsDOMHandler::OnGetNotification(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) {}

void RewardsDOMHandler::OnGetAllNotifications(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotificationsList&
        notifications_list) {}

void RewardsDOMHandler::SaveSetting(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  AllowJavascript();

  if (rewards_service_) {
    const std::string key = args[0].GetString();
    const std::string value = args[1].GetString();

    if (key == "contributionMonthly") {
      rewards_service_->SetAutoContributionAmount(std::stod(value));
    }

    if (key == "contributionMinTime") {
      int int_value;
      if (!base::StringToInt(value, &int_value)) {
        LOG(ERROR) << "Min time was not converted to int";
        return;
      }

      rewards_service_->SetPublisherMinVisitTime(int_value);
    }

    if (key == "contributionMinVisits") {
      int int_value;
      if (!base::StringToInt(value, &int_value)) {
        LOG(ERROR) << "Min visits was not converted to int";
        return;
      }

      rewards_service_->SetPublisherMinVisits(int_value);
    }

    if (key == "contributionNonVerified") {
      rewards_service_->SetPublisherAllowNonVerified(value == "true");
    }

    if (key == "contributionVideos") {
      rewards_service_->SetPublisherAllowVideos(value == "true");
    }

    if (key == "enabledContribute") {
      rewards_service_->SetAutoContributeEnabled(value == "true");
    }
  }
}

void RewardsDOMHandler::ExcludePublisher(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  const std::string publisherKey = args[0].GetString();
  rewards_service_->SetPublisherExclude(publisherKey, true);
}

void RewardsDOMHandler::RestorePublishers(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  rewards_service_->RestorePublishers();
}

void RewardsDOMHandler::RestorePublisher(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  std::string publisherKey = args[0].GetString();
  rewards_service_->SetPublisherExclude(publisherKey, false);
}

void RewardsDOMHandler::OnPublisherList(
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::List publishers;
  for (auto const& item : list) {
    base::Value::Dict publisher;
    publisher.Set("id", item->id);
    publisher.Set("percentage", static_cast<double>(item->percent));
    publisher.Set("publisherKey", item->id);
    publisher.Set("status", static_cast<int>(item->status));
    publisher.Set("excluded", static_cast<int>(item->excluded));
    publisher.Set("name", item->name);
    publisher.Set("provider", item->provider);
    publisher.Set("url", item->url);
    publisher.Set("favIcon", item->favicon_url);
    publishers.Append(std::move(publisher));
  }

  CallJavascriptFunction("brave_rewards.contributeList",
                         base::Value(std::move(publishers)));
}

void RewardsDOMHandler::OnExcludedSiteList(
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::List publishers;
  for (auto const& item : list) {
    base::Value::Dict publisher;
    publisher.Set("id", item->id);
    publisher.Set("status", static_cast<int>(item->status));
    publisher.Set("name", item->name);
    publisher.Set("provider", item->provider);
    publisher.Set("url", item->url);
    publisher.Set("favIcon", item->favicon_url);
    publishers.Append(std::move(publisher));
  }

  CallJavascriptFunction("brave_rewards.excludedList",
                         base::Value(std::move(publishers)));
}

void RewardsDOMHandler::OnGetContributionAmount(double amount) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.contributionAmount",
                           base::Value(amount));
  }
}

void RewardsDOMHandler::GetAutoContributionAmount(
    const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->GetAutoContributionAmount(
        base::BindOnce(&RewardsDOMHandler::OnGetContributionAmount,
                       weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnReconcileComplete(
    brave_rewards::RewardsService* rewards_service,
    const ledger::mojom::Result result,
    const std::string& contribution_id,
    const double amount,
    const ledger::mojom::RewardsType type,
    const ledger::mojom::ContributionProcessor processor) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict complete;
  complete.Set("result", static_cast<int>(result));
  complete.Set("type", static_cast<int>(type));

  CallJavascriptFunction("brave_rewards.reconcileComplete",
                         base::Value(std::move(complete)));
}

void RewardsDOMHandler::RemoveRecurringTip(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (rewards_service_) {
    AllowJavascript();
    const std::string publisherKey = args[0].GetString();
    rewards_service_->RemoveRecurringTip(publisherKey);
  }
}

void RewardsDOMHandler::GetRecurringTips(const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->GetRecurringTips(base::BindOnce(
        &RewardsDOMHandler::OnGetRecurringTips, weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetRecurringTips(
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  base::Value::List publishers;

  for (auto const& item : list) {
    base::Value::Dict publisher;
    publisher.Set("id", item->id);
    publisher.Set("percentage", item->weight);
    publisher.Set("publisherKey", item->id);
    publisher.Set("status", static_cast<int>(item->status));
    publisher.Set("excluded", static_cast<int>(item->excluded));
    publisher.Set("name", item->name);
    publisher.Set("provider", item->provider);
    publisher.Set("url", item->url);
    publisher.Set("favIcon", item->favicon_url);
    publisher.Set("tipDate", 0);
    publishers.Append(std::move(publisher));
  }

  CallJavascriptFunction("brave_rewards.recurringTips",
                         base::Value(std::move(publishers)));
}

void RewardsDOMHandler::OnGetOneTimeTips(
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  base::Value::List publishers;

  for (auto const& item : list) {
    base::Value::Dict publisher;
    publisher.Set("id", item->id);
    publisher.Set("percentage", item->weight);
    publisher.Set("publisherKey", item->id);
    publisher.Set("status", static_cast<int>(item->status));
    publisher.Set("excluded", static_cast<int>(item->excluded));
    publisher.Set("name", item->name);
    publisher.Set("provider", item->provider);
    publisher.Set("url", item->url);
    publisher.Set("favIcon", item->favicon_url);
    publisher.Set("tipDate", static_cast<double>(item->reconcile_stamp));
    publishers.Append(std::move(publisher));
  }

  CallJavascriptFunction("brave_rewards.currentTips",
                         base::Value(std::move(publishers)));
}

void RewardsDOMHandler::GetOneTimeTips(const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->GetOneTimeTips(base::BindOnce(
        &RewardsDOMHandler::OnGetOneTimeTips, weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::GetContributionList(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetAutoContributeProperties(
      base::BindOnce(&RewardsDOMHandler::OnAutoContributePropsReady,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetAdsData(const base::Value::List& args) {
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  base::Value::Dict ads_data;
  ads_data.Set("adsIsSupported", ads_service_->IsSupportedLocale());
  ads_data.Set("adsEnabled", ads_service_->IsEnabled());
  ads_data.Set(
      "adsPerHour",
      static_cast<int>(ads_service_->GetMaximumNotificationAdsPerHour()));
  ads_data.Set(kAdsSubdivisionTargeting,
               ads_service_->GetSubdivisionTargetingCode());
  ads_data.Set(kAutoDetectedSubdivisionTargeting,
               ads_service_->GetAutoDetectedSubdivisionTargetingCode());
  ads_data.Set("shouldAllowAdsSubdivisionTargeting",
               ads_service_->ShouldAllowSubdivisionTargeting());
  ads_data.Set("adsUIEnabled", true);
  ads_data.Set("needsBrowserUpgradeToServeAds",
               ads_service_->NeedsBrowserUpgradeToServeAds());

  base::Value::List subdivisions;
  const auto supported_subdivisions = ads::GetSupportedSubdivisions();
  for (const auto& subdivision : supported_subdivisions) {
    base::Value::Dict subdivision_dict;
    subdivision_dict.Set("code", subdivision.first);
    subdivision_dict.Set("name", subdivision.second);
    subdivisions.Append(std::move(subdivision_dict));
  }

  ads_data.Set("subdivisions", std::move(subdivisions));
  CallJavascriptFunction("brave_rewards.adsData",
                         base::Value(std::move(ads_data)));
}

void RewardsDOMHandler::GetAdsHistory(const base::Value::List& args) {
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  const base::Time now = base::Time::Now();

  const base::Time from_time = now - base::Days(kDaysOfAdsHistory - 1);
  const base::Time from_time_at_local_midnight = from_time.LocalMidnight();

  ads_service_->GetHistory(from_time_at_local_midnight, now,
                           base::BindOnce(&RewardsDOMHandler::OnGetAdsHistory,
                                          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetAdsHistory(base::Value::List ads_history) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.adsHistory",
                         base::Value(std::move(ads_history)));
}

void RewardsDOMHandler::ToggleAdThumbUp(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleAdThumbUp(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleAdThumbUp,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdThumbUp(base::Value::Dict dict) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleAdThumbUp",
                         base::Value(std::move(dict)));
}

void RewardsDOMHandler::ToggleAdThumbDown(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleAdThumbDown(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleAdThumbDown,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdThumbDown(base::Value::Dict dict) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleAdThumbDown",
                         base::Value(std::move(dict)));
}

void RewardsDOMHandler::ToggleAdOptIn(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  const std::string category = args[0].GetString();
  const int action = args[1].GetInt();
  ads_service_->ToggleAdOptIn(
      category, action,
      base::BindOnce(&RewardsDOMHandler::OnToggleAdOptIn,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdOptIn(const std::string& category,
                                        int action) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict value;
  value.Set("category", category);
  value.Set("action", action);
  CallJavascriptFunction("brave_rewards.onToggleAdOptIn",
                         base::Value(std::move(value)));
}

void RewardsDOMHandler::ToggleAdOptOut(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  const std::string category = args[0].GetString();
  const int action = args[1].GetInt();
  ads_service_->ToggleAdOptOut(
      category, action,
      base::BindOnce(&RewardsDOMHandler::OnToggleAdOptOut,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdOptOut(const std::string& category,
                                         int action) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict value;
  value.Set("category", category);
  value.Set("action", action);
  CallJavascriptFunction("brave_rewards.onToggleAdOptOut",
                         base::Value(std::move(value)));
}

void RewardsDOMHandler::ToggleSavedAd(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleSavedAd(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleSavedAd,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleSavedAd(base::Value::Dict dict) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleSavedAd",
                         base::Value(std::move(dict)));
}

void RewardsDOMHandler::ToggleFlaggedAd(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleFlaggedAd(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleFlaggedAd,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleFlaggedAd(base::Value::Dict dict) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleFlaggedAd",
                         base::Value(std::move(dict)));
}

void RewardsDOMHandler::SaveAdsSetting(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  const std::string key = args[0].GetString();
  const std::string value = args[1].GetString();

  if (key == "adsEnabled") {
    ads_service_->SetEnabled(value == "true" &&
                             ads_service_->IsSupportedLocale());
  } else if (key == "adsPerHour") {
    int64_t int64_value;
    if (!base::StringToInt64(value, &int64_value)) {
      LOG(ERROR) << "Ads per hour was not converted to int64";
      return;
    }

    ads_service_->SetMaximumNotificationAdsPerHour(int64_value);
  } else if (key == kAdsSubdivisionTargeting) {
    ads_service_->SetSubdivisionTargetingCode(value);
  } else if (key == kAutoDetectedSubdivisionTargeting) {
    ads_service_->SetAutoDetectedSubdivisionTargetingCode(value);
  }

  GetAdsData(base::Value::List());
}

void RewardsDOMHandler::GetPendingContributionsTotal(
    const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->GetPendingContributionsTotal(
        base::BindOnce(&RewardsDOMHandler::OnGetPendingContributionsTotal,
                       weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetPendingContributionsTotal(double amount) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.pendingContributionTotal",
                           base::Value(amount));
  }
}

void RewardsDOMHandler::OnPendingContributionSaved(
    brave_rewards::RewardsService* rewards_service,
    const ledger::mojom::Result result) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  CallJavascriptFunction("brave_rewards.onPendingContributionSaved",
                         base::Value(static_cast<int>(result)));
}

void RewardsDOMHandler::OnPublisherListNormalized(
    brave_rewards::RewardsService* rewards_service,
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  OnPublisherList(std::move(list));
}

void RewardsDOMHandler::GetStatement(const base::Value::List& args) {
  if (!ads_service_) {
    return;
  }

  AllowJavascript();
  ads_service_->GetStatementOfAccounts(base::BindOnce(
      &RewardsDOMHandler::OnGetStatement, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetStatement(ads::mojom::StatementInfoPtr statement) {
  if (!statement) {
    return;
  }

  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict dict;
  dict.Set("adsNextPaymentDate",
           statement->next_payment_date.ToDoubleT() * 1000);
  dict.Set("adsReceivedThisMonth", statement->ads_received_this_month);
  dict.Set("adsEarningsThisMonth", statement->earnings_this_month);
  dict.Set("adsEarningsLastMonth", statement->earnings_last_month);

  CallJavascriptFunction("brave_rewards.statement",
                         base::Value(std::move(dict)));
}

void RewardsDOMHandler::OnStatementChanged(
    brave_rewards::RewardsService* rewards_service) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.statementChanged");
  }
}

void RewardsDOMHandler::OnAdRewardsDidChange() {
  if (!ads_service_) {
    return;
  }

  ads_service_->GetStatementOfAccounts(base::BindOnce(
      &RewardsDOMHandler::OnGetStatement, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnNeedsBrowserUpgradeToServeAds() {
  GetAdsData(base::Value::List());
}

void RewardsDOMHandler::OnRecurringTipSaved(
    brave_rewards::RewardsService* rewards_service,
    bool success) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.recurringTipSaved",
                           base::Value(success));
  }
}

void RewardsDOMHandler::OnRecurringTipRemoved(
    brave_rewards::RewardsService* rewards_service,
    bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.recurringTipRemoved",
                         base::Value(success));
}

void RewardsDOMHandler::GetEnabledInlineTippingPlatforms(
    const base::Value::List& args) {
  AllowJavascript();

  // TODO(zenparsing): Consider using a PrefChangeRegistrar to monitor changes
  // to these values.
  auto* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  base::Value::List list;

  if (prefs->GetBoolean(brave_rewards::prefs::kInlineTipGithubEnabled))
    list.Append("github");

  if (prefs->GetBoolean(brave_rewards::prefs::kInlineTipRedditEnabled))
    list.Append("reddit");

  if (prefs->GetBoolean(brave_rewards::prefs::kInlineTipTwitterEnabled))
    list.Append("twitter");

  CallJavascriptFunction("brave_rewards.enabledInlineTippingPlatforms",
                         base::Value(std::move(list)));
}

void RewardsDOMHandler::SetInlineTippingPlatformEnabled(
    const base::Value::List& args) {
  AllowJavascript();

  std::string key = args[0].GetString();
  std::string value = args[1].GetString();

  if (rewards_service_) {
    rewards_service_->SetInlineTippingPlatformEnabled(key, value == "true");
  }
}

void RewardsDOMHandler::GetPendingContributions(const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->GetPendingContributions(
        base::BindOnce(&RewardsDOMHandler::OnGetPendingContributions,
                       weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetPendingContributions(
    std::vector<ledger::mojom::PendingContributionInfoPtr> list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::List contributions;
  for (auto const& item : list) {
    base::Value::Dict contribution;
    contribution.Set("id", static_cast<int>(item->id));
    contribution.Set("publisherKey", item->publisher_key);
    contribution.Set("status", static_cast<int>(item->status));
    contribution.Set("name", item->name);
    contribution.Set("provider", item->provider);
    contribution.Set("url", item->url);
    contribution.Set("favIcon", item->favicon_url);
    contribution.Set("amount", item->amount);
    contribution.Set("addedDate", base::NumberToString(item->added_date));
    contribution.Set("type", static_cast<int>(item->type));
    contribution.Set("viewingId", item->viewing_id);
    contribution.Set("expirationDate",
                     base::NumberToString(item->expiration_date));
    contributions.Append(std::move(contribution));
  }

  CallJavascriptFunction("brave_rewards.pendingContributions",
                         base::Value(std::move(contributions)));
}

void RewardsDOMHandler::RemovePendingContribution(
    const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  const uint64_t id = args[0].GetInt();
  rewards_service_->RemovePendingContribution(id);
}

void RewardsDOMHandler::RemoveAllPendingContributions(
    const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->RemoveAllPendingContributions();
  }
}

void RewardsDOMHandler::OnPendingContributionRemoved(
    brave_rewards::RewardsService* rewards_service,
    const ledger::mojom::Result result) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.onRemovePendingContribution",
                           base::Value(static_cast<int>(result)));
  }
}

void RewardsDOMHandler::OnFetchBalance(const ledger::mojom::Result result,
                                       ledger::mojom::BalancePtr balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict balance_value;

  if (balance) {
    balance_value.Set("total", balance->total);

    if (result == ledger::mojom::Result::LEDGER_OK) {
      base::Value::Dict wallets;
      for (auto const& wallet : balance->wallets) {
        wallets.Set(wallet.first, wallet.second);
      }
      balance_value.Set("wallets", std::move(wallets));
    }
  } else {
    balance_value.Set("total", 0.0);
    balance_value.Set("wallets", base::Value::Dict());
  }

  base::Value::Dict data;
  data.Set("status", static_cast<int>(result));
  data.Set("balance", std::move(balance_value));
  CallJavascriptFunction("brave_rewards.balance", base::Value(std::move(data)));
}

void RewardsDOMHandler::FetchBalance(const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->FetchBalance(base::BindOnce(
        &RewardsDOMHandler::OnFetchBalance, weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::GetExternalWallet(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  rewards_service_->GetExternalWallet(base::BindOnce(
      &RewardsDOMHandler::OnGetExternalWallet, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetExternalWallet(
    brave_rewards::GetExternalWalletResult result) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  if (result.has_value()) {
    auto wallet = std::move(result.value());

    base::Value::Dict wallet_dict;
    wallet_dict.Set("type", wallet->type);
    wallet_dict.Set("address", wallet->address);
    wallet_dict.Set("status", static_cast<int>(wallet->status));
    wallet_dict.Set("userName", wallet->user_name);
    wallet_dict.Set("accountUrl", wallet->account_url);
    wallet_dict.Set("loginUrl", wallet->login_url);
    wallet_dict.Set("activityUrl", wallet->activity_url);

    data.SetByDottedPath("value.wallet", std::move(wallet_dict));
  } else {
    data.Set("error", static_cast<int>(result.error()));
  }

  CallJavascriptFunction("brave_rewards.onGetExternalWallet",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::ConnectExternalWallet(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  CHECK_EQ(2U, args.size());

  AllowJavascript();
  const std::string path = args[0].GetString();
  const std::string query = args[1].GetString();
  rewards_service_->ConnectExternalWallet(
      path, query,
      base::BindOnce(&RewardsDOMHandler::OnConnectExternalWallet,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnConnectExternalWallet(
    brave_rewards::ConnectExternalWalletResult result) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  if (result.has_value()) {
    data.Set("value", base::Value::Dict());
  } else {
    data.Set("error", static_cast<int>(result.error()));
  }

  CallJavascriptFunction("brave_rewards.onConnectExternalWallet",
                         std::move(data));
}

void RewardsDOMHandler::OnExternalWalletLoggedOut() {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onExternalWalletLoggedOut");
}

void RewardsDOMHandler::OnRewardsWalletUpdated() {
  if (!IsJavascriptAllowed()) {
    return;
  }

  GetAdsData(base::Value::List());
  GetAutoContributeProperties(base::Value::List());
  GetOnboardingStatus(base::Value::List());
  GetUserVersion(base::Value::List());
  GetExternalWallet(base::Value::List());
  GetCountryCode(base::Value::List());
}

void RewardsDOMHandler::OnUnblindedTokensReady(
    brave_rewards::RewardsService* rewards_service) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.unblindedTokensReady");
}

void RewardsDOMHandler::ReconcileStampReset() {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.reconcileStampReset");
}

void RewardsDOMHandler::OnGetBalanceReport(
    const uint32_t month,
    const uint32_t year,
    const ledger::mojom::Result result,
    ledger::mojom::BalanceReportInfoPtr report) {
  if (!IsJavascriptAllowed() || !report) {
    return;
  }

  base::Value::Dict report_base;
  report_base.Set("grant", report->grants);
  report_base.Set("ads", report->earning_from_ads);
  report_base.Set("contribute", report->auto_contribute);
  report_base.Set("monthly", report->recurring_donation);
  report_base.Set("tips", report->one_time_donation);

  base::Value::Dict data;
  data.Set("month", static_cast<int>(month));
  data.Set("year", static_cast<int>(year));
  data.Set("report", std::move(report_base));

  CallJavascriptFunction("brave_rewards.balanceReport",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::GetBalanceReport(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  const uint32_t month = args[0].GetInt();
  const uint32_t year = args[1].GetInt();
  rewards_service_->GetBalanceReport(
      month, year,
      base::BindOnce(&RewardsDOMHandler::OnGetBalanceReport,
                     weak_factory_.GetWeakPtr(), month, year));
}

void RewardsDOMHandler::OnGetMonthlyReport(
    const uint32_t month,
    const uint32_t year,
    ledger::mojom::MonthlyReportInfoPtr report) {
  if (!IsJavascriptAllowed() || !report) {
    return;
  }

  base::Value::Dict data;
  data.Set("month", static_cast<int>(month));
  data.Set("year", static_cast<int>(year));

  base::Value::Dict balance_report;
  balance_report.Set("grant", report->balance->grants);
  balance_report.Set("ads", report->balance->earning_from_ads);
  balance_report.Set("contribute", report->balance->auto_contribute);
  balance_report.Set("monthly", report->balance->recurring_donation);
  balance_report.Set("tips", report->balance->one_time_donation);

  base::Value::List transactions;
  for (const auto& item : report->transactions) {
    base::Value::Dict transaction_report;
    transaction_report.Set("amount", item->amount);
    transaction_report.Set("type", static_cast<int>(item->type));
    transaction_report.Set("processor", static_cast<int>(item->processor));
    transaction_report.Set("created_at", static_cast<double>(item->created_at));

    transactions.Append(std::move(transaction_report));
  }

  base::Value::List contributions;
  for (const auto& contribution : report->contributions) {
    base::Value::List publishers;
    for (const auto& item : contribution->publishers) {
      base::Value::Dict publisher;
      publisher.Set("id", item->id);
      publisher.Set("percentage", static_cast<double>(item->percent));
      publisher.Set("weight", item->weight);
      publisher.Set("publisherKey", item->id);
      publisher.Set("status", static_cast<int>(item->status));
      publisher.Set("name", item->name);
      publisher.Set("provider", item->provider);
      publisher.Set("url", item->url);
      publisher.Set("favIcon", item->favicon_url);
      publishers.Append(std::move(publisher));
    }

    base::Value::Dict contribution_report;
    contribution_report.Set("amount", contribution->amount);
    contribution_report.Set("type", static_cast<int>(contribution->type));
    contribution_report.Set("processor",
                            static_cast<int>(contribution->processor));
    contribution_report.Set("created_at",
                            static_cast<double>(contribution->created_at));
    contribution_report.Set("publishers", std::move(publishers));
    contributions.Append(std::move(contribution_report));
  }

  base::Value::Dict report_base;
  report_base.Set("balance", std::move(balance_report));
  report_base.Set("transactions", std::move(transactions));
  report_base.Set("contributions", std::move(contributions));

  data.Set("report", std::move(report_base));

  CallJavascriptFunction("brave_rewards.monthlyReport",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::GetMonthlyReport(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  const uint32_t month = args[0].GetInt();
  const uint32_t year = args[1].GetInt();

  rewards_service_->GetMonthlyReport(
      month, year,
      base::BindOnce(&RewardsDOMHandler::OnGetMonthlyReport,
                     weak_factory_.GetWeakPtr(), month, year));
}

void RewardsDOMHandler::OnGetAllMonthlyReportIds(
    const std::vector<std::string>& ids) {
  base::Value::List list;
  for (const auto& item : ids) {
    list.Append(base::Value(item));
  }

  CallJavascriptFunction("brave_rewards.monthlyReportIds",
                         base::Value(std::move(list)));
}

void RewardsDOMHandler::GetAllMonthlyReportIds(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetAllMonthlyReportIds(
      base::BindOnce(&RewardsDOMHandler::OnGetAllMonthlyReportIds,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetCountryCode(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  CallJavascriptFunction("brave_rewards.countryCode",
                         base::Value(rewards_service_->GetCountryCode()));
}

void RewardsDOMHandler::GetIsUnsupportedRegion(const base::Value::List& args) {
  AllowJavascript();

  CallJavascriptFunction("brave_rewards.onIsUnsupportedRegion",
                         base::Value(brave_rewards::IsUnsupportedRegion()));
}

void RewardsDOMHandler::GetPluralString(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(3U, args.size());

  // Adapted from `chrome/browser/ui/webui/plural_string_handler.cc`. The
  // `PluralStringHandler` class is not current built on Android. Since this
  // WebUI is shared between Android and desktop, we need to provide our own
  // implementation for now.
  const base::Value& callback_id = args[0];
  std::string message_name = args[1].GetString();
  int count = args[2].GetInt();

  static const base::flat_map<std::string, int> name_to_id = {
      {"publisherCountText", IDS_REWARDS_PUBLISHER_COUNT_TEXT},
      {"onboardingSetupAdsPerHour",
       IDS_BRAVE_REWARDS_ONBOARDING_SETUP_ADS_PER_HOUR}};

  auto message_id_it = name_to_id.find(message_name);
  CHECK(name_to_id.end() != message_id_it);
  auto string = l10n_util::GetPluralStringFUTF16(message_id_it->second, count);

  ResolveJavascriptCallback(callback_id, base::Value(string));
}

void RewardsDOMHandler::CompleteReset(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  rewards_service_->CompleteReset(base::DoNothing());
}

void RewardsDOMHandler::OnCompleteReset(const bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.completeReset", base::Value(success));
}

void RewardsDOMHandler::GetOnboardingStatus(const base::Value::List& args) {
  AllowJavascript();
  Profile* profile = Profile::FromWebUI(web_ui());
  base::Value::Dict data;
  data.Set("showOnboarding",
           !profile->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled));
  CallJavascriptFunction("brave_rewards.onboardingStatus",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::EnableRewards(const base::Value::List& args) {
#if !BUILDFLAG(IS_ANDROID)
  AllowJavascript();
  if (auto* coordinator = GetPanelCoordinator(web_ui()->GetWebContents())) {
    coordinator->OpenRewardsPanel();
  }
#else
  // On Android, a native onboarding modal is displayed when the user navigates
  // to the Rewards page. This message handler should not be called.
  NOTREACHED();
#endif
}

void RewardsDOMHandler::GetExternalWalletProviders(
    const base::Value::List& args) {
  if (!rewards_service_)
    return;

  AllowJavascript();
  base::Value::List data;

  std::vector<std::string> providers =
      rewards_service_->GetExternalWalletProviders();
  for (std::string provider : providers) {
    data.Append(provider);
  }

  CallJavascriptFunction("brave_rewards.externalWalletProviderList",
                         base::Value(std::move(data)));
}

}  // namespace

BraveRewardsPageUI::BraveRewardsPageUI(content::WebUI* web_ui,
                                       const std::string& name)
    : WebUIController(web_ui) {
  auto* source = CreateAndAddWebUIDataSource(
      web_ui, name, kBraveRewardsPageGenerated, kBraveRewardsPageGeneratedSize,
      IDR_BRAVE_REWARDS_PAGE_HTML, /*disable_trusted_types_csp=*/true);

#if BUILDFLAG(IS_ANDROID)
  source->AddBoolean("isAndroid", true);
#else
  source->AddBoolean("isAndroid", false);
#endif

  auto handler_owner = std::make_unique<RewardsDOMHandler>();
  RewardsDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsPageUI::~BraveRewardsPageUI() = default;
