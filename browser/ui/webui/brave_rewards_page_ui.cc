/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_page_ui.h"

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "bat/ads/ad_content_info.h"
#include "bat/ads/pref_names.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_page_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/bindings_policy.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/webui/favicon_source.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "content/public/browser/url_data_source.h"
#endif

using content::WebUIMessageHandler;

namespace {

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
  void GetRewardsParameters(const base::Value::List& args);
  void GetAutoContributeProperties(const base::Value::List& args);
  void FetchPromotions(const base::Value::List& args);
  void ClaimPromotion(const base::Value::List& args);
  void AttestPromotion(const base::Value::List& args);
  void RecoverWallet(const base::Value::List& args);
  void GetReconcileStamp(const base::Value::List& args);
  void SaveSetting(const base::Value::List& args);
  void OnPublisherList(ledger::type::PublisherInfoList list);
  void OnExcludedSiteList(ledger::type::PublisherInfoList list);
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
  void OnGetAdsHistory(const base::ListValue& history);
  void ToggleAdThumbUp(const base::Value::List& args);
  void OnToggleAdThumbUp(const std::string& json);
  void ToggleAdThumbDown(const base::Value::List& args);
  void OnToggleAdThumbDown(const std::string& json);
  void ToggleAdOptIn(const base::Value::List& args);
  void OnToggleAdOptIn(const std::string& category, const int action);
  void ToggleAdOptOut(const base::Value::List& args);
  void OnToggleAdOptOut(const std::string& category, const int action);
  void ToggleSavedAd(const base::Value::List& args);
  void OnToggleSavedAd(const std::string& json);
  void ToggleFlaggedAd(const base::Value::List& args);
  void OnToggleFlaggedAd(const std::string& json);
  void SaveAdsSetting(const base::Value::List& args);
  void SetBackupCompleted(const base::Value::List& args);
  void OnGetContributionAmount(double amount);
  void OnGetAutoContributeProperties(
      ledger::type::AutoContributePropertiesPtr properties);
  void OnGetReconcileStamp(uint64_t reconcile_stamp);
  void OnAutoContributePropsReady(
      ledger::type::AutoContributePropertiesPtr properties);
  void GetPendingContributionsTotal(const base::Value::List& args);
  void OnGetPendingContributionsTotal(double amount);
  void GetStatement(const base::Value::List& args);
  void GetExcludedSites(const base::Value::List& args);

  void OnGetStatement(const bool success,
                      const double next_payment_date,
                      const int ads_received_this_month,
                      const double earnings_this_month,
                      const double earnings_last_month);

  void OnGetRecurringTips(ledger::type::PublisherInfoList list);

  void OnGetOneTimeTips(ledger::type::PublisherInfoList list);

  void GetEnabledInlineTippingPlatforms(const base::Value::List& args);
  void SetInlineTippingPlatformEnabled(const base::Value::List& args);

  void GetPendingContributions(const base::Value::List& args);
  void OnGetPendingContributions(
      ledger::type::PendingContributionInfoList list);
  void RemovePendingContribution(const base::Value::List& args);
  void RemoveAllPendingContributions(const base::Value::List& args);
  void FetchBalance(const base::Value::List& args);
  void OnFetchBalance(const ledger::type::Result result,
                      ledger::type::BalancePtr balance);

  void GetExternalWallet(const base::Value::List& args);
  void OnGetExternalWallet(const ledger::type::Result result,
                           ledger::type::ExternalWalletPtr wallet);

  void ProcessRewardsPageUrl(const base::Value::List& args);

  void OnProcessRewardsPageUrl(
      const ledger::type::Result result,
      const std::string& wallet_type,
      const std::string& action,
      const base::flat_map<std::string, std::string>& args);

  void DisconnectWallet(const base::Value::List& args);

  void GetBalanceReport(const base::Value::List& args);

  void OnGetBalanceReport(const uint32_t month,
                          const uint32_t year,
                          const ledger::type::Result result,
                          ledger::type::BalanceReportInfoPtr report);

  void GetMonthlyReport(const base::Value::List& args);

  void GetAllMonthlyReportIds(const base::Value::List& args);
  void GetCountryCode(const base::Value::List& args);

  void OnGetMonthlyReport(const uint32_t month,
                          const uint32_t year,
                          ledger::type::MonthlyReportInfoPtr report);

  void OnGetAllMonthlyReportIds(const std::vector<std::string>& ids);

  void OnGetRewardsParameters(ledger::type::RewardsParametersPtr parameters);

  void CompleteReset(const base::Value::List& args);

  void GetPaymentId(const base::Value::List& args);

  void OnWalletCreatedForPaymentId(ledger::type::Result result);

  void OnGetPaymentId(ledger::type::BraveWalletPtr wallet);

  void GetWalletPassphrase(const base::Value::List& args);

  void OnGetWalletPassphrase(const std::string& pass);

  void GetOnboardingStatus(const base::Value::List& args);
  void SaveOnboardingResult(const base::Value::List& args);
  void GetExternalWalletProviders(const base::Value::List& args);
  void SetExternalWalletType(const base::Value::List& args);

  void OnExternalWalletTypeUpdated(const ledger::type::Result result,
                                   ledger::type::ExternalWalletPtr wallet);

  // RewardsServiceObserver implementation
  void OnRewardsInitialized(
      brave_rewards::RewardsService* rewards_service) override;
  void OnFetchPromotions(brave_rewards::RewardsService* rewards_service,
                         const ledger::type::Result result,
                         const ledger::type::PromotionList& list) override;
  void OnPromotionFinished(brave_rewards::RewardsService* rewards_service,
                           const ledger::type::Result result,
                           ledger::type::PromotionPtr promotion) override;
  void OnRecoverWallet(brave_rewards::RewardsService* rewards_service,
                       const ledger::type::Result result) override;
  void OnExcludedSitesChanged(brave_rewards::RewardsService* rewards_service,
                              std::string publisher_id,
                              bool excluded) override;
  void OnReconcileComplete(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::type::RewardsType type,
      const ledger::type::ContributionProcessor processor) override;

  void OnPendingContributionSaved(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result) override;

  void OnPublisherListNormalized(brave_rewards::RewardsService* rewards_service,
                                 ledger::type::PublisherInfoList list) override;

  void OnStatementChanged(
      brave_rewards::RewardsService* rewards_service) override;

  void OnRecurringTipSaved(brave_rewards::RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(brave_rewards::RewardsService* rewards_service,
                             bool success) override;

  void OnPendingContributionRemoved(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result) override;

  void OnDisconnectWallet(brave_rewards::RewardsService* rewards_service,
                          const ledger::type::Result result,
                          const std::string& wallet_type) override;

  void OnAdsEnabled(brave_rewards::RewardsService* rewards_service,
                    bool ads_enabled) override;

  void OnClaimPromotion(const std::string& promotion_id,
                        const ledger::type::Result result,
                        const std::string& captcha_image,
                        const std::string& hint,
                        const std::string& captcha_id);

  void OnAttestPromotion(const std::string& promotion_id,
                         const ledger::type::Result result,
                         ledger::type::PromotionPtr promotion);

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

const int kDaysOfAdsHistory = 30;

const char kShouldAllowAdsSubdivisionTargeting[] =
    "shouldAllowAdsSubdivisionTargeting";
const char kAdsSubdivisionTargeting[] = "adsSubdivisionTargeting";
const char kAutoDetectedSubdivisionTargeting[] =
    "automaticallyDetectedAdsSubdivisionTargeting";
const char kNeedsBrowserUpgradeToServeAds[] = "needsBrowserUpgradeToServeAds";

}  // namespace

RewardsDOMHandler::RewardsDOMHandler() : weak_factory_(this) {}

RewardsDOMHandler::~RewardsDOMHandler() {}

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
      "brave_rewards.attestPromotion",
      base::BindRepeating(&RewardsDOMHandler::AttestPromotion,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.recoverWallet",
      base::BindRepeating(&RewardsDOMHandler::RecoverWallet,
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
      "brave_rewards.setBackupCompleted",
      base::BindRepeating(&RewardsDOMHandler::SetBackupCompleted,
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
      "brave_rewards.processRewardsPageUrl",
      base::BindRepeating(&RewardsDOMHandler::ProcessRewardsPageUrl,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.disconnectWallet",
      base::BindRepeating(&RewardsDOMHandler::DisconnectWallet,
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
      "brave_rewards.getPaymentId",
      base::BindRepeating(&RewardsDOMHandler::GetPaymentId,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getWalletPassphrase",
      base::BindRepeating(&RewardsDOMHandler::GetWalletPassphrase,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getOnboardingStatus",
      base::BindRepeating(&RewardsDOMHandler::GetOnboardingStatus,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.saveOnboardingResult",
      base::BindRepeating(&RewardsDOMHandler::SaveOnboardingResult,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getExternalWalletProviders",
      base::BindRepeating(&RewardsDOMHandler::GetExternalWalletProviders,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.setExternalWalletType",
      base::BindRepeating(&RewardsDOMHandler::SetExternalWalletType,
                          base::Unretained(this)));
}

void RewardsDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());

  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  rewards_service_->StartProcess(base::DoNothing());

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
      ads::prefs::kAdsPerHour,
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
    CallJavascriptFunction("brave_rewards.initialized", base::Value(0));
  }
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
    ledger::type::RewardsParametersPtr parameters) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  if (parameters) {
    base::Value::List auto_contribute_choices;
    for (double const& item : parameters->auto_contribute_choices) {
      auto_contribute_choices.Append(item);
    }

    base::Value::Dict payout_status;
    for (const auto& [key, value] : parameters->payout_status) {
      payout_status.Set(key, value);
    }

    data.Set("rate", parameters->rate);
    data.Set("autoContributeChoice",
                      parameters->auto_contribute_choice);
    data.Set("autoContributeChoices", std::move(auto_contribute_choices));
    data.Set("payoutStatus", std::move(payout_status));
  }
  CallJavascriptFunction("brave_rewards.rewardsParameters",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::OnRewardsInitialized(
    brave_rewards::RewardsService* rewards_service) {
  if (!IsJavascriptAllowed())
    return;

  CallJavascriptFunction("brave_rewards.initialized", base::Value(0));
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
    ledger::type::Result,
    ledger::type::ExternalWalletPtr wallet) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.externalWalletLogin",
                           base::Value(wallet ? wallet->login_url : ""));
  }
}

void RewardsDOMHandler::OnGetAutoContributeProperties(
    ledger::type::AutoContributePropertiesPtr properties) {
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
    const ledger::type::Result result,
    const ledger::type::PromotionList& list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::List promotions;
  for (const auto& item : list) {
    base::Value::Dict dict;
    dict.Set("promotionId", item->id);
    dict.Set("type", static_cast<int>(item->type));
    dict.Set("status", static_cast<int>(item->status));
    dict.Set("createdAt", static_cast<int>(item->created_at));
    dict.Set("claimableUntil", static_cast<int>(item->claimable_until));
    dict.Set("expiresAt", static_cast<int>(item->expires_at));
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
    rewards_service_->FetchPromotions();
  }
}

void RewardsDOMHandler::OnClaimPromotion(const std::string& promotion_id,
                                         const ledger::type::Result result,
                                         const std::string& captcha_image,
                                         const std::string& hint,
                                         const std::string& captcha_id) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict response;
  response.Set("result", static_cast<int>(result));
  response.Set("promotionId", promotion_id);
  response.Set("captchaImage", captcha_image);
  response.Set("captchaId", captcha_id);
  response.Set("hint", hint);

  CallJavascriptFunction("brave_rewards.claimPromotion",
                         base::Value(std::move(response)));
}

void RewardsDOMHandler::ClaimPromotion(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  const std::string promotion_id = args[0].GetString();

#if !BUILDFLAG(IS_ANDROID)
  rewards_service_->ClaimPromotion(
      promotion_id, base::BindOnce(&RewardsDOMHandler::OnClaimPromotion,
                                   weak_factory_.GetWeakPtr(), promotion_id));
#else
  // No need for a callback. The UI receives "brave_rewards.promotionFinish".
  brave_rewards::AttestPromotionCallback callback = base::DoNothing();
  rewards_service_->ClaimPromotion(promotion_id, std::move(callback));
#endif
}

void RewardsDOMHandler::AttestPromotion(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  AllowJavascript();

  if (!rewards_service_) {
    base::Value::Dict finish;
    finish.Set("status", 1);
    CallJavascriptFunction("brave_rewards.promotionFinish",
                           base::Value(std::move(finish)));
    return;
  }

  const std::string promotion_id = args[0].GetString();
  const std::string solution = args[1].GetString();
  rewards_service_->AttestPromotion(
      promotion_id, solution,
      base::BindOnce(&RewardsDOMHandler::OnAttestPromotion,
                     weak_factory_.GetWeakPtr(), promotion_id));
}

void RewardsDOMHandler::OnAttestPromotion(
    const std::string& promotion_id,
    const ledger::type::Result result,
    ledger::type::PromotionPtr promotion) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict promotion_dict;
  promotion_dict.Set("promotionId", promotion_id);

  if (promotion) {
    promotion_dict.Set("expiresAt", static_cast<int>(promotion->expires_at));
    promotion_dict.Set("amount", promotion->approximate_value);
    promotion_dict.Set("type", static_cast<int>(promotion->type));
  }

  base::Value::Dict finish;
  finish.Set("result", static_cast<int>(result));
  finish.Set("promotion", std::move(promotion_dict));

  CallJavascriptFunction("brave_rewards.promotionFinish",
                         base::Value(std::move(finish)));
}

void RewardsDOMHandler::OnPromotionFinished(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result,
    ledger::type::PromotionPtr promotion) {
  if (result != ledger::type::Result::LEDGER_OK) {
    return;
  }

  OnAttestPromotion(promotion->id, result, promotion->Clone());
}

void RewardsDOMHandler::RecoverWallet(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (rewards_service_) {
    AllowJavascript();
    const std::string pass_phrase = args[0].GetString();
    rewards_service_->RecoverWallet(pass_phrase);
  }
}

void RewardsDOMHandler::OnRecoverWallet(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.recoverWalletData",
                         base::Value(static_cast<int>(result)));
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
    ledger::type::AutoContributePropertiesPtr properties) {
  auto filter = ledger::type::ActivityInfoFilter::New();
  auto pair =
      ledger::type::ActivityInfoFilterOrderPair::New("ai.percent", false);
  filter->order_by.push_back(std::move(pair));
  filter->min_duration = properties->contribution_min_time;
  filter->reconcile_stamp = properties->reconcile_stamp;
  filter->excluded = ledger::type::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED;
  filter->percent = 1;
  filter->non_verified = properties->contribution_non_verified;
  filter->min_visits = properties->contribution_min_visits;

  rewards_service_->GetActivityInfoList(
      0, 0, std::move(filter),
      base::BindOnce(&RewardsDOMHandler::OnPublisherList,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetExcludedSites(const base::Value::List& args) {
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

void RewardsDOMHandler::OnPublisherList(ledger::type::PublisherInfoList list) {
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
    ledger::type::PublisherInfoList list) {
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
    const ledger::type::Result result,
    const std::string& contribution_id,
    const double amount,
    const ledger::type::RewardsType type,
    const ledger::type::ContributionProcessor processor) {
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
    ledger::type::PublisherInfoList list) {
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

void RewardsDOMHandler::OnGetOneTimeTips(ledger::type::PublisherInfoList list) {
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
    publisher.Set("tipDate", static_cast<int>(item->reconcile_stamp));
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
  ads_data.Set("adsPerHour", static_cast<int>(ads_service_->GetNotificationAdsPerHour()));
  ads_data.Set(kAdsSubdivisionTargeting,
               ads_service_->GetSubdivisionTargetingCode());
  ads_data.Set(kAutoDetectedSubdivisionTargeting,
               ads_service_->GetAutoDetectedSubdivisionTargetingCode());
  ads_data.Set(kShouldAllowAdsSubdivisionTargeting,
               ads_service_->ShouldAllowSubdivisionTargeting());
  ads_data.Set("adsUIEnabled", true);
  ads_data.Set(kNeedsBrowserUpgradeToServeAds,
               ads_service_->NeedsBrowserUpgradeToServeAds());
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

void RewardsDOMHandler::OnGetAdsHistory(const base::ListValue& ads_history) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.adsHistory", ads_history);
}

void RewardsDOMHandler::ToggleAdThumbUp(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  ads::AdContentInfo ad_content;
  const base::Value& value = args[0];
  if (value.is_dict())
    ad_content.FromValue(value.GetDict());

  ads_service_->ToggleAdThumbUp(
      ad_content.ToJson(), base::BindOnce(&RewardsDOMHandler::OnToggleAdThumbUp,
                                          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdThumbUp(const std::string& json) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  ads::AdContentInfo ad_content;
  const bool success = ad_content.FromJson(json);
  DCHECK(success);

  CallJavascriptFunction("brave_rewards.onToggleAdThumbUp",
                         base::Value(ad_content.ToValue()));
}

void RewardsDOMHandler::ToggleAdThumbDown(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  ads::AdContentInfo ad_content;
  const base::Value& value = args[0];
  if (value.is_dict())
    ad_content.FromValue(value.GetDict());

  ads_service_->ToggleAdThumbDown(
      ad_content.ToJson(),
      base::BindOnce(&RewardsDOMHandler::OnToggleAdThumbDown,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdThumbDown(const std::string& json) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  ads::AdContentInfo ad_content;
  const bool success = ad_content.FromJson(json);
  DCHECK(success);

  CallJavascriptFunction("brave_rewards.onToggleAdThumbDown",
                         base::Value(ad_content.ToValue()));
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

  AllowJavascript();

  ads::AdContentInfo ad_content;
  const base::Value& value = args[0];
  if (value.is_dict())
    ad_content.FromValue(value.GetDict());

  ads_service_->ToggleSavedAd(
      ad_content.ToJson(), base::BindOnce(&RewardsDOMHandler::OnToggleSavedAd,
                                          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleSavedAd(const std::string& json) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  ads::AdContentInfo ad_content;
  const bool success = ad_content.FromJson(json);
  DCHECK(success);

  CallJavascriptFunction("brave_rewards.onToggleSavedAd",
                         base::Value(ad_content.ToValue()));
}

void RewardsDOMHandler::ToggleFlaggedAd(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  ads::AdContentInfo ad_content;
  const base::Value& value = args[0];
  if (value.is_dict())
    ad_content.FromValue(value.GetDict());

  ads_service_->ToggleFlaggedAd(
      ad_content.ToJson(), base::BindOnce(&RewardsDOMHandler::OnToggleFlaggedAd,
                                          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleFlaggedAd(const std::string& json) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  ads::AdContentInfo ad_content;
  const bool success = ad_content.FromJson(json);
  DCHECK(success);

  CallJavascriptFunction("brave_rewards.onToggleFlaggedAd",
                         base::Value(ad_content.ToValue()));
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
    const auto is_enabled =
        value == "true" && ads_service_->IsSupportedLocale();
    rewards_service_->SetAdsEnabled(is_enabled);
  } else if (key == "adsPerHour") {
    ads_service_->SetNotificationAdsPerHour(std::stoull(value));
  } else if (key == kAdsSubdivisionTargeting) {
    ads_service_->SetSubdivisionTargetingCode(value);
  } else if (key == kAutoDetectedSubdivisionTargeting) {
    ads_service_->SetAutoDetectedSubdivisionTargetingCode(value);
  }

  GetAdsData(base::Value::List());
}

void RewardsDOMHandler::SetBackupCompleted(const base::Value::List& args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->SetBackupCompleted();
  }
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
    const ledger::type::Result result) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  CallJavascriptFunction("brave_rewards.onPendingContributionSaved",
                         base::Value(static_cast<int>(result)));
}

void RewardsDOMHandler::OnPublisherListNormalized(
    brave_rewards::RewardsService* rewards_service,
    ledger::type::PublisherInfoList list) {
  OnPublisherList(std::move(list));
}

void RewardsDOMHandler::GetStatement(const base::Value::List& args) {
  AllowJavascript();
  ads_service_->GetStatementOfAccounts(base::BindOnce(
      &RewardsDOMHandler::OnGetStatement, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetStatement(const bool success,
                                       const double next_payment_date,
                                       const int ads_received_this_month,
                                       const double earnings_this_month,
                                       const double earnings_last_month) {
  if (!success) {
    return;
  }

  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict statement;

  statement.Set("adsNextPaymentDate", next_payment_date * 1000);
  statement.Set("adsReceivedThisMonth", ads_received_this_month);
  statement.Set("adsEarningsThisMonth", earnings_this_month);
  statement.Set("adsEarningsLastMonth", earnings_last_month);

  CallJavascriptFunction("brave_rewards.statement",
                         base::Value(std::move(statement)));
}

void RewardsDOMHandler::OnStatementChanged(
    brave_rewards::RewardsService* rewards_service) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.statementChanged");
  }
}

void RewardsDOMHandler::OnAdRewardsDidChange() {
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
    ledger::type::PendingContributionInfoList list) {
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
    contribution.Set("addedDate", std::to_string(item->added_date));
    contribution.Set("type", static_cast<int>(item->type));
    contribution.Set("viewingId", item->viewing_id);
    contribution.Set("expirationDate", std::to_string(item->expiration_date));
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
    const ledger::type::Result result) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.onRemovePendingContribution",
                           base::Value(static_cast<int>(result)));
  }
}

void RewardsDOMHandler::OnFetchBalance(const ledger::type::Result result,
                                       ledger::type::BalancePtr balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict balance_value;

  if (balance) {
    balance_value.Set("total", balance->total);

    if (result == ledger::type::Result::LEDGER_OK) {
      base::Value::Dict wallets;
      for (auto const& wallet : balance->wallets) {
        wallets.Set(wallet.first, wallet.second);
      }
      balance_value.Set("wallets", std::move(wallets));
    }
  } else {
    balance_value.Set("total", 0.0);
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
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet) {
  if (IsJavascriptAllowed()) {
    base::Value::Dict data;
    data.Set("result", static_cast<int>(result));
    base::Value::Dict wallet_dict;

    if (wallet) {
      wallet_dict.Set("type", wallet->type);
      wallet_dict.Set("address", wallet->address);
      wallet_dict.Set("status", static_cast<int>(wallet->status));
      wallet_dict.Set("addUrl", wallet->add_url);
      wallet_dict.Set("withdrawUrl", wallet->withdraw_url);
      wallet_dict.Set("userName", wallet->user_name);
      wallet_dict.Set("accountUrl", wallet->account_url);
      wallet_dict.Set("loginUrl", wallet->login_url);
      wallet_dict.Set("activityUrl", wallet->activity_url);
    }

    data.Set("wallet", std::move(wallet_dict));
    CallJavascriptFunction("brave_rewards.externalWallet",
                           base::Value(std::move(data)));
  }
}

void RewardsDOMHandler::OnProcessRewardsPageUrl(
    const ledger::type::Result result,
    const std::string& wallet_type,
    const std::string& action,
    const base::flat_map<std::string, std::string>& args) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  data.Set("result", static_cast<int>(result));
  data.Set("walletType", wallet_type);
  data.Set("action", action);

  base::Value::Dict new_args;
  for (auto const& arg : args) {
    new_args.Set(arg.first, arg.second);
  }
  data.Set("args", std::move(new_args));

  CallJavascriptFunction("brave_rewards.processRewardsPageUrl",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::ProcessRewardsPageUrl(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  const std::string path = args[0].GetString();
  const std::string query = args[1].GetString();
  rewards_service_->ProcessRewardsPageUrl(
      path, query,
      base::BindOnce(&RewardsDOMHandler::OnProcessRewardsPageUrl,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::DisconnectWallet(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }
  AllowJavascript();
  rewards_service_->DisconnectWallet();
}

void RewardsDOMHandler::OnDisconnectWallet(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result,
    const std::string& wallet_type) {
  base::Value::Dict data;
  data.Set("result", static_cast<int>(result));
  data.Set("walletType", wallet_type);

  CallJavascriptFunction("brave_rewards.disconnectWallet",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::OnAdsEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool ads_enabled) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  GetAdsData(base::Value::List());
  GetAutoContributeProperties(base::Value::List());
  GetOnboardingStatus(base::Value::List());
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
    const ledger::type::Result result,
    ledger::type::BalanceReportInfoPtr report) {
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
    ledger::type::MonthlyReportInfoPtr report) {
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
    transaction_report.Set("created_at", static_cast<int>(item->created_at));

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
                            static_cast<int>(contribution->created_at));
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
  AllowJavascript();

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  CallJavascriptFunction("brave_rewards.countryCode",
                         base::Value(country_code));
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

void RewardsDOMHandler::GetPaymentId(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  // Ensure that a wallet has been created for the user before attempting
  // to retrieve a payment ID.
  rewards_service_->CreateWallet(
      base::BindOnce(&RewardsDOMHandler::OnWalletCreatedForPaymentId,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnWalletCreatedForPaymentId(
    ledger::type::Result result) {
  rewards_service_->GetBraveWallet(base::BindOnce(
      &RewardsDOMHandler::OnGetPaymentId, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetPaymentId(ledger::type::BraveWalletPtr wallet) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  std::string payment_id;
  if (wallet) {
    payment_id = wallet->payment_id;
  }

  CallJavascriptFunction("brave_rewards.paymentId", base::Value(payment_id));
}

void RewardsDOMHandler::GetWalletPassphrase(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  rewards_service_->GetWalletPassphrase(base::BindOnce(
      &RewardsDOMHandler::OnGetWalletPassphrase, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetWalletPassphrase(const std::string& passphrase) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.walletPassphrase",
                         base::Value(passphrase));
}

void RewardsDOMHandler::GetOnboardingStatus(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }
  AllowJavascript();
  base::Value::Dict data;
  data.Set("showOnboarding", rewards_service_->ShouldShowOnboarding());
  CallJavascriptFunction("brave_rewards.onboardingStatus",
                         base::Value(std::move(data)));
}

void RewardsDOMHandler::SaveOnboardingResult(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_)
    return;

  AllowJavascript();
  if (args[0].GetString() == "opted-in")
    rewards_service_->EnableRewards();
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
  CreateAndAddWebUIDataSource(web_ui, name, kBraveRewardsPageGenerated,
                              kBraveRewardsPageGeneratedSize,
#if BUILDFLAG(IS_ANDROID)
                              IDR_BRAVE_REWARDS_ANDROID_PAGE_HTML,
#else
                              IDR_BRAVE_REWARDS_PAGE_HTML,
#endif
                              /*disable_trusted_types_csp=*/true);
  auto handler_owner = std::make_unique<RewardsDOMHandler>();
  RewardsDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsPageUI::~BraveRewardsPageUI() {}
