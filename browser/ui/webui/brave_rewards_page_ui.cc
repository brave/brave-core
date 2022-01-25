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
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_page_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/bindings_policy.h"

#if defined(OS_ANDROID)
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
  void RestartBrowser(base::Value::ConstListView args);
  void IsInitialized(base::Value::ConstListView args);
  void GetRewardsParameters(base::Value::ConstListView args);
  void GetAutoContributeProperties(base::Value::ConstListView args);
  void FetchPromotions(base::Value::ConstListView args);
  void ClaimPromotion(base::Value::ConstListView args);
  void AttestPromotion(base::Value::ConstListView args);
  void RecoverWallet(base::Value::ConstListView args);
  void GetReconcileStamp(base::Value::ConstListView args);
  void SaveSetting(base::Value::ConstListView args);
  void OnPublisherList(ledger::type::PublisherInfoList list);
  void OnExcludedSiteList(ledger::type::PublisherInfoList list);
  void ExcludePublisher(base::Value::ConstListView args);
  void RestorePublishers(base::Value::ConstListView args);
  void RestorePublisher(base::Value::ConstListView args);
  void GetAutoContributionAmount(base::Value::ConstListView args);
  void RemoveRecurringTip(base::Value::ConstListView args);
  void GetRecurringTips(base::Value::ConstListView args);
  void GetOneTimeTips(base::Value::ConstListView args);
  void GetContributionList(base::Value::ConstListView args);
  void GetAdsData(base::Value::ConstListView args);
  void GetAdsHistory(base::Value::ConstListView args);
  void OnGetAdsHistory(const base::ListValue& history);
  void ToggleAdThumbUp(base::Value::ConstListView args);
  void OnToggleAdThumbUp(const std::string& json);
  void ToggleAdThumbDown(base::Value::ConstListView args);
  void OnToggleAdThumbDown(const std::string& json);
  void ToggleAdOptIn(base::Value::ConstListView args);
  void OnToggleAdOptIn(const std::string& category, const int action);
  void ToggleAdOptOut(base::Value::ConstListView args);
  void OnToggleAdOptOut(const std::string& category, const int action);
  void ToggleSavedAd(base::Value::ConstListView args);
  void OnToggleSavedAd(const std::string& json);
  void ToggleFlaggedAd(base::Value::ConstListView args);
  void OnToggleFlaggedAd(const std::string& json);
  void SaveAdsSetting(base::Value::ConstListView args);
  void SetBackupCompleted(base::Value::ConstListView args);
  void OnGetContributionAmount(double amount);
  void OnGetAutoContributeProperties(
      ledger::type::AutoContributePropertiesPtr properties);
  void OnGetReconcileStamp(uint64_t reconcile_stamp);
  void OnAutoContributePropsReady(
      ledger::type::AutoContributePropertiesPtr properties);
  void GetPendingContributionsTotal(base::Value::ConstListView args);
  void OnGetPendingContributionsTotal(double amount);
  void GetStatement(base::Value::ConstListView args);
  void GetExcludedSites(base::Value::ConstListView args);

  void OnGetStatement(const bool success,
                      const double next_payment_date,
                      const int ads_received_this_month,
                      const double earnings_this_month,
                      const double earnings_last_month);

  void OnGetRecurringTips(ledger::type::PublisherInfoList list);

  void OnGetOneTimeTips(ledger::type::PublisherInfoList list);

  void GetEnabledInlineTippingPlatforms(base::Value::ConstListView args);
  void SetInlineTippingPlatformEnabled(base::Value::ConstListView args);

  void GetPendingContributions(base::Value::ConstListView args);
  void OnGetPendingContributions(
      ledger::type::PendingContributionInfoList list);
  void RemovePendingContribution(base::Value::ConstListView args);
  void RemoveAllPendingContributions(base::Value::ConstListView args);
  void FetchBalance(base::Value::ConstListView args);
  void OnFetchBalance(const ledger::type::Result result,
                      ledger::type::BalancePtr balance);

  void GetExternalWallet(base::Value::ConstListView args);
  void OnGetExternalWallet(const ledger::type::Result result,
                           ledger::type::ExternalWalletPtr wallet);

  void ProcessRewardsPageUrl(base::Value::ConstListView args);

  void OnProcessRewardsPageUrl(
      const ledger::type::Result result,
      const std::string& wallet_type,
      const std::string& action,
      const base::flat_map<std::string, std::string>& args);

  void DisconnectWallet(base::Value::ConstListView args);

  void GetBalanceReport(base::Value::ConstListView args);

  void OnGetBalanceReport(const uint32_t month,
                          const uint32_t year,
                          const ledger::type::Result result,
                          ledger::type::BalanceReportInfoPtr report);

  void GetMonthlyReport(base::Value::ConstListView args);

  void GetAllMonthlyReportIds(base::Value::ConstListView args);
  void GetCountryCode(base::Value::ConstListView args);

  void OnGetMonthlyReport(const uint32_t month,
                          const uint32_t year,
                          ledger::type::MonthlyReportInfoPtr report);

  void OnGetAllMonthlyReportIds(const std::vector<std::string>& ids);

  void OnGetRewardsParameters(ledger::type::RewardsParametersPtr parameters);

  void CompleteReset(base::Value::ConstListView args);

  void GetPaymentId(base::Value::ConstListView args);

  void OnWalletCreatedForPaymentId(ledger::type::Result result);

  void OnGetPaymentId(ledger::type::BraveWalletPtr wallet);

  void GetWalletPassphrase(base::Value::ConstListView args);

  void OnGetWalletPassphrase(const std::string& pass);

  void GetOnboardingStatus(base::Value::ConstListView args);
  void SaveOnboardingResult(base::Value::ConstListView args);
  void GetExternalWalletProviders(base::Value::ConstListView args);
  void SetExternalWalletType(base::Value::ConstListView args);

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
  void OnAdRewardsChanged() override;

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
const char kAutoDetectedAdsSubdivisionTargeting[] =
    "automaticallyDetectedAdsSubdivisionTargeting";

}  // namespace

RewardsDOMHandler::RewardsDOMHandler() : weak_factory_(this) {}

RewardsDOMHandler::~RewardsDOMHandler() {}

void RewardsDOMHandler::RegisterMessages() {
#if defined(OS_ANDROID)
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
      ads::prefs::kAdsSubdivisionTargetingCode,
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

void RewardsDOMHandler::RestartBrowser(base::Value::ConstListView args) {
  AllowJavascript();
  chrome::AttemptRestart();
}

void RewardsDOMHandler::IsInitialized(base::Value::ConstListView args) {
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

void RewardsDOMHandler::GetRewardsParameters(base::Value::ConstListView args) {
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

  base::DictionaryValue data;
  if (parameters) {
    auto choices = std::make_unique<base::ListValue>();
    for (double const& choice : parameters->auto_contribute_choices) {
      choices->Append(choice);
    }

    data.SetDouble("rate", parameters->rate);
    data.SetDouble("autoContributeChoice", parameters->auto_contribute_choice);
    data.SetList("autoContributeChoices", std::move(choices));
  }
  CallJavascriptFunction("brave_rewards.rewardsParameters", data);
}

void RewardsDOMHandler::OnRewardsInitialized(
    brave_rewards::RewardsService* rewards_service) {
  if (!IsJavascriptAllowed())
    return;

  CallJavascriptFunction("brave_rewards.initialized", base::Value(0));
}

void RewardsDOMHandler::GetAutoContributeProperties(
    base::Value::ConstListView args) {
  if (!rewards_service_)
    return;

  AllowJavascript();

  rewards_service_->GetAutoContributeProperties(
      base::BindOnce(&RewardsDOMHandler::OnGetAutoContributeProperties,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::SetExternalWalletType(base::Value::ConstListView args) {
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

  base::DictionaryValue values;
  values.SetBoolean("enabledContribute", properties->enabled_contribute);
  values.SetInteger("contributionMinTime", properties->contribution_min_time);
  values.SetInteger("contributionMinVisits",
                    properties->contribution_min_visits);
  values.SetBoolean("contributionNonVerified",
                    properties->contribution_non_verified);
  values.SetBoolean("contributionVideos", properties->contribution_videos);

  CallJavascriptFunction("brave_rewards.autoContributeProperties", values);
}

void RewardsDOMHandler::OnFetchPromotions(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result,
    const ledger::type::PromotionList& list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::ListValue promotions;
  for (const auto& item : list) {
    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetString("promotionId", item->id);
    dict->SetInteger("type", static_cast<int>(item->type));
    dict->SetInteger("status", static_cast<int>(item->status));
    dict->SetInteger("expiresAt", item->expires_at);
    dict->SetDouble("amount", item->approximate_value);
    promotions.Append(std::move(dict));
  }

  base::DictionaryValue dict;
  dict.SetInteger("result", static_cast<int>(result));
  dict.SetKey("promotions", std::move(promotions));

  CallJavascriptFunction("brave_rewards.promotions", dict);
}

void RewardsDOMHandler::FetchPromotions(base::Value::ConstListView args) {
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

  base::DictionaryValue response;
  response.SetInteger("result", static_cast<int>(result));
  response.SetString("promotionId", promotion_id);
  response.SetString("captchaImage", captcha_image);
  response.SetString("captchaId", captcha_id);
  response.SetString("hint", hint);

  CallJavascriptFunction("brave_rewards.claimPromotion", response);
}

void RewardsDOMHandler::ClaimPromotion(base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  const std::string promotion_id = args[0].GetString();

#if !defined(OS_ANDROID)
  rewards_service_->ClaimPromotion(
      promotion_id, base::BindOnce(&RewardsDOMHandler::OnClaimPromotion,
                                   weak_factory_.GetWeakPtr(), promotion_id));
#else
  // No need for a callback. The UI receives "brave_rewards.promotionFinish".
  brave_rewards::AttestPromotionCallback callback = base::DoNothing();
  rewards_service_->ClaimPromotion(promotion_id, std::move(callback));
#endif
}

void RewardsDOMHandler::AttestPromotion(base::Value::ConstListView args) {
  CHECK_EQ(2U, args.size());
  AllowJavascript();

  if (!rewards_service_) {
    base::DictionaryValue finish;
    finish.SetInteger("status", 1);
    CallJavascriptFunction("brave_rewards.promotionFinish", finish);
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

  base::DictionaryValue promotion_dict;
  promotion_dict.SetString("promotionId", promotion_id);

  if (promotion) {
    promotion_dict.SetInteger("expiresAt", promotion->expires_at);
    promotion_dict.SetDouble("amount", promotion->approximate_value);
    promotion_dict.SetInteger("type", static_cast<int>(promotion->type));
  }

  base::DictionaryValue finish;
  finish.SetInteger("result", static_cast<int>(result));
  finish.SetKey("promotion", std::move(promotion_dict));

  CallJavascriptFunction("brave_rewards.promotionFinish", finish);
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

void RewardsDOMHandler::RecoverWallet(base::Value::ConstListView args) {
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

void RewardsDOMHandler::GetReconcileStamp(base::Value::ConstListView args) {
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

void RewardsDOMHandler::GetExcludedSites(base::Value::ConstListView args) {
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
#if defined(OS_ANDROID)
  if (notification.type_ == brave_rewards::RewardsNotificationService::
                                REWARDS_NOTIFICATION_GRANT &&
      IsJavascriptAllowed()) {
    base::DictionaryValue finish;
    finish.SetInteger("status", false);
    finish.SetInteger("expiryTime", 0);
    finish.SetString("probi", "0");

    CallJavascriptFunction("brave_rewards.grantFinish", finish);
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

void RewardsDOMHandler::SaveSetting(base::Value::ConstListView args) {
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

void RewardsDOMHandler::ExcludePublisher(base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  const std::string publisherKey = args[0].GetString();
  rewards_service_->SetPublisherExclude(publisherKey, true);
}

void RewardsDOMHandler::RestorePublishers(base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();
  rewards_service_->RestorePublishers();
}

void RewardsDOMHandler::RestorePublisher(base::Value::ConstListView args) {
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

  auto publishers = std::make_unique<base::ListValue>();
  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("id", item->id);
    publisher->SetDouble("percentage", item->percent);
    publisher->SetString("publisherKey", item->id);
    publisher->SetInteger("status", static_cast<int>(item->status));
    publisher->SetInteger("excluded", static_cast<int>(item->excluded));
    publisher->SetString("name", item->name);
    publisher->SetString("provider", item->provider);
    publisher->SetString("url", item->url);
    publisher->SetString("favIcon", item->favicon_url);
    publishers->Append(std::move(publisher));
  }

  CallJavascriptFunction("brave_rewards.contributeList", *publishers);
}

void RewardsDOMHandler::OnExcludedSiteList(
    ledger::type::PublisherInfoList list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  auto publishers = std::make_unique<base::ListValue>();
  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("id", item->id);
    publisher->SetInteger("status", static_cast<int>(item->status));
    publisher->SetString("name", item->name);
    publisher->SetString("provider", item->provider);
    publisher->SetString("url", item->url);
    publisher->SetString("favIcon", item->favicon_url);
    publishers->Append(std::move(publisher));
  }

  CallJavascriptFunction("brave_rewards.excludedList", *publishers);
}

void RewardsDOMHandler::OnGetContributionAmount(double amount) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.contributionAmount",
                           base::Value(amount));
  }
}

void RewardsDOMHandler::GetAutoContributionAmount(
    base::Value::ConstListView args) {
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

  base::DictionaryValue complete;
  complete.SetKey("result", base::Value(static_cast<int>(result)));
  complete.SetKey("type", base::Value(static_cast<int>(type)));

  CallJavascriptFunction("brave_rewards.reconcileComplete", complete);
}

void RewardsDOMHandler::RemoveRecurringTip(base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());
  if (rewards_service_) {
    AllowJavascript();
    const std::string publisherKey = args[0].GetString();
    rewards_service_->RemoveRecurringTip(publisherKey);
  }
}

void RewardsDOMHandler::GetRecurringTips(base::Value::ConstListView args) {
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
  auto publishers = std::make_unique<base::ListValue>();

  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("id", item->id);
    publisher->SetDouble("percentage", item->weight);
    publisher->SetString("publisherKey", item->id);
    publisher->SetInteger("status", static_cast<int>(item->status));
    publisher->SetInteger("excluded", static_cast<int>(item->excluded));
    publisher->SetString("name", item->name);
    publisher->SetString("provider", item->provider);
    publisher->SetString("url", item->url);
    publisher->SetString("favIcon", item->favicon_url);
    publisher->SetInteger("tipDate", 0);
    publishers->Append(std::move(publisher));
  }

  CallJavascriptFunction("brave_rewards.recurringTips", *publishers);
}

void RewardsDOMHandler::OnGetOneTimeTips(ledger::type::PublisherInfoList list) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  auto publishers = std::make_unique<base::ListValue>();

  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("id", item->id);
    publisher->SetDouble("percentage", item->weight);
    publisher->SetString("publisherKey", item->id);
    publisher->SetInteger("status", static_cast<int>(item->status));
    publisher->SetInteger("excluded", static_cast<int>(item->excluded));
    publisher->SetString("name", item->name);
    publisher->SetString("provider", item->provider);
    publisher->SetString("url", item->url);
    publisher->SetString("favIcon", item->favicon_url);
    publisher->SetInteger("tipDate", item->reconcile_stamp);
    publishers->Append(std::move(publisher));
  }

  CallJavascriptFunction("brave_rewards.currentTips", *publishers);
}

void RewardsDOMHandler::GetOneTimeTips(base::Value::ConstListView args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->GetOneTimeTips(base::BindOnce(
        &RewardsDOMHandler::OnGetOneTimeTips, weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::GetContributionList(base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetAutoContributeProperties(
      base::BindOnce(&RewardsDOMHandler::OnAutoContributePropsReady,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetAdsData(base::Value::ConstListView args) {
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  base::DictionaryValue ads_data;

  auto is_supported_locale = ads_service_->IsSupportedLocale();
  ads_data.SetBoolean("adsIsSupported", is_supported_locale);

  auto is_enabled = ads_service_->IsEnabled();
  ads_data.SetBoolean("adsEnabled", is_enabled);

  auto ads_per_hour = ads_service_->GetAdsPerHour();
  ads_data.SetInteger("adsPerHour", ads_per_hour);

  const std::string subdivision_targeting_code =
      ads_service_->GetAdsSubdivisionTargetingCode();
  ads_data.SetString(kAdsSubdivisionTargeting, subdivision_targeting_code);

  const std::string auto_detected_subdivision_targeting_code =
      ads_service_->GetAutoDetectedAdsSubdivisionTargetingCode();
  ads_data.SetString(kAutoDetectedAdsSubdivisionTargeting,
                     auto_detected_subdivision_targeting_code);

  const bool should_allow_subdivision_ad_targeting =
      ads_service_->ShouldAllowAdsSubdivisionTargeting();
  ads_data.SetBoolean(kShouldAllowAdsSubdivisionTargeting,
                      should_allow_subdivision_ad_targeting);

  ads_data.SetBoolean("adsUIEnabled", true);

  CallJavascriptFunction("brave_rewards.adsData", ads_data);
}

void RewardsDOMHandler::GetAdsHistory(base::Value::ConstListView args) {
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  const base::Time to_time = base::Time::Now();
  const uint64_t to_timestamp = to_time.ToDoubleT();

  const base::Time from_time = to_time - base::Days(kDaysOfAdsHistory - 1);
  const base::Time from_time_local_midnight = from_time.LocalMidnight();
  const uint64_t from_timestamp = from_time_local_midnight.ToDoubleT();

  ads_service_->GetAdsHistory(
      from_timestamp, to_timestamp,
      base::BindOnce(&RewardsDOMHandler::OnGetAdsHistory,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetAdsHistory(const base::ListValue& ads_history) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.adsHistory", ads_history);
}

void RewardsDOMHandler::ToggleAdThumbUp(base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  ads::AdContentInfo ad_content;
  const base::Value& value = args[0];
  ad_content.FromValue(value);

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

  const base::Value value = ad_content.ToValue();
  CallJavascriptFunction("brave_rewards.onToggleAdThumbUp", value);
}

void RewardsDOMHandler::ToggleAdThumbDown(base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  ads::AdContentInfo ad_content;
  const base::Value& value = args[0];
  ad_content.FromValue(value);

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

  const base::Value value = ad_content.ToValue();
  CallJavascriptFunction("brave_rewards.onToggleAdThumbDown", value);
}

void RewardsDOMHandler::ToggleAdOptIn(base::Value::ConstListView args) {
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

  base::Value value(base::Value::Type::DICTIONARY);
  value.SetKey("category", base::Value(category));
  value.SetKey("action", base::Value(action));
  CallJavascriptFunction("brave_rewards.onToggleAdOptIn", value);
}

void RewardsDOMHandler::ToggleAdOptOut(base::Value::ConstListView args) {
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

  base::Value value(base::Value::Type::DICTIONARY);
  value.SetKey("category", base::Value(category));
  value.SetKey("action", base::Value(action));
  CallJavascriptFunction("brave_rewards.onToggleAdOptOut", value);
}

void RewardsDOMHandler::ToggleSavedAd(base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  ads::AdContentInfo ad_content;
  const base::Value& value = args[0];
  ad_content.FromValue(value);

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

  const base::Value value = ad_content.ToValue();
  CallJavascriptFunction("brave_rewards.onToggleSavedAd", value);
}

void RewardsDOMHandler::ToggleFlaggedAd(base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  ads::AdContentInfo ad_content;
  const base::Value& value = args[0];
  ad_content.FromValue(value);

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

  const base::Value value = ad_content.ToValue();
  CallJavascriptFunction("brave_rewards.onToggleFlaggedAd", value);
}

void RewardsDOMHandler::SaveAdsSetting(base::Value::ConstListView args) {
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
    ads_service_->SetAdsPerHour(std::stoull(value));
  } else if (key == kAdsSubdivisionTargeting) {
    ads_service_->SetAdsSubdivisionTargetingCode(value);
  } else if (key == kAutoDetectedAdsSubdivisionTargeting) {
    ads_service_->SetAutoDetectedAdsSubdivisionTargetingCode(value);
  }

  GetAdsData(base::Value::ConstListView());
}

void RewardsDOMHandler::SetBackupCompleted(base::Value::ConstListView args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->SetBackupCompleted();
  }
}

void RewardsDOMHandler::GetPendingContributionsTotal(
    base::Value::ConstListView args) {
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

void RewardsDOMHandler::GetStatement(base::Value::ConstListView args) {
  AllowJavascript();
  ads_service_->GetAccountStatement(base::BindOnce(
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

  base::DictionaryValue history;

  history.SetDouble("adsNextPaymentDate", next_payment_date * 1000);
  history.SetInteger("adsReceivedThisMonth", ads_received_this_month);
  history.SetDouble("adsEarningsThisMonth", earnings_this_month);
  history.SetDouble("adsEarningsLastMonth", earnings_last_month);

  CallJavascriptFunction("brave_rewards.statement", history);
}

void RewardsDOMHandler::OnStatementChanged(
    brave_rewards::RewardsService* rewards_service) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.statementChanged");
  }
}

void RewardsDOMHandler::OnAdRewardsChanged() {
  ads_service_->GetAccountStatement(base::BindOnce(
      &RewardsDOMHandler::OnGetStatement, weak_factory_.GetWeakPtr()));
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
    base::Value::ConstListView args) {
  AllowJavascript();

  // TODO(zenparsing): Consider using a PrefChangeRegistrar to monitor changes
  // to these values.
  auto* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  base::Value list(base::Value::Type::LIST);

  if (prefs->GetBoolean(brave_rewards::prefs::kInlineTipGithubEnabled))
    list.Append("github");

  if (prefs->GetBoolean(brave_rewards::prefs::kInlineTipRedditEnabled))
    list.Append("reddit");

  if (prefs->GetBoolean(brave_rewards::prefs::kInlineTipTwitterEnabled))
    list.Append("twitter");

  CallJavascriptFunction("brave_rewards.enabledInlineTippingPlatforms",
                         std::move(list));
}

void RewardsDOMHandler::SetInlineTippingPlatformEnabled(
    base::Value::ConstListView args) {
  AllowJavascript();

  std::string key = args[0].GetString();
  std::string value = args[1].GetString();

  if (rewards_service_) {
    rewards_service_->SetInlineTippingPlatformEnabled(key, value == "true");
  }
}

void RewardsDOMHandler::GetPendingContributions(
    base::Value::ConstListView args) {
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

  auto contributions = std::make_unique<base::ListValue>();
  for (auto const& item : list) {
    auto contribution =
        std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    contribution->SetKey("id", base::Value(static_cast<int>(item->id)));
    contribution->SetKey("publisherKey", base::Value(item->publisher_key));
    contribution->SetKey("status", base::Value(static_cast<int>(item->status)));
    contribution->SetKey("name", base::Value(item->name));
    contribution->SetKey("provider", base::Value(item->provider));
    contribution->SetKey("url", base::Value(item->url));
    contribution->SetKey("favIcon", base::Value(item->favicon_url));
    contribution->SetKey("amount", base::Value(item->amount));
    contribution->SetKey("addedDate",
                         base::Value(std::to_string(item->added_date)));
    contribution->SetKey("type", base::Value(static_cast<int>(item->type)));
    contribution->SetKey("viewingId", base::Value(item->viewing_id));
    contribution->SetKey("expirationDate",
                         base::Value(std::to_string(item->expiration_date)));
    contributions->Append(std::move(contribution));
  }

  CallJavascriptFunction("brave_rewards.pendingContributions", *contributions);
}

void RewardsDOMHandler::RemovePendingContribution(
    base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  const uint64_t id = args[0].GetInt();
  rewards_service_->RemovePendingContribution(id);
}

void RewardsDOMHandler::RemoveAllPendingContributions(
    base::Value::ConstListView args) {
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

  base::Value balance_value(base::Value::Type::DICTIONARY);

  if (balance) {
    balance_value.SetDoubleKey("total", balance->total);

    if (result == ledger::type::Result::LEDGER_OK) {
      base::Value wallets(base::Value::Type::DICTIONARY);
      for (auto const& wallet : balance->wallets) {
        wallets.SetDoubleKey(wallet.first, wallet.second);
      }
      balance_value.SetKey("wallets", std::move(wallets));
    }
  } else {
    balance_value.SetDoubleKey("total", 0.0);
  }

  base::DictionaryValue data;
  data.SetIntKey("status", static_cast<int>(result));
  data.SetKey("balance", std::move(balance_value));
  CallJavascriptFunction("brave_rewards.balance", data);
}

void RewardsDOMHandler::FetchBalance(base::Value::ConstListView args) {
  if (rewards_service_) {
    AllowJavascript();
    rewards_service_->FetchBalance(base::BindOnce(
        &RewardsDOMHandler::OnFetchBalance, weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::GetExternalWallet(base::Value::ConstListView args) {
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
    base::Value data(base::Value::Type::DICTIONARY);
    data.SetIntKey("result", static_cast<int>(result));
    base::Value wallet_dict(base::Value::Type::DICTIONARY);

    if (wallet) {
      wallet_dict.SetStringKey("type", wallet->type);
      wallet_dict.SetStringKey("address", wallet->address);
      wallet_dict.SetIntKey("status", static_cast<int>(wallet->status));
      wallet_dict.SetStringKey("verifyUrl", wallet->verify_url);
      wallet_dict.SetStringKey("addUrl", wallet->add_url);
      wallet_dict.SetStringKey("withdrawUrl", wallet->withdraw_url);
      wallet_dict.SetStringKey("userName", wallet->user_name);
      wallet_dict.SetStringKey("accountUrl", wallet->account_url);
      wallet_dict.SetStringKey("loginUrl", wallet->login_url);
      wallet_dict.SetStringKey("activityUrl", wallet->activity_url);
    }

    data.SetKey("wallet", std::move(wallet_dict));
    CallJavascriptFunction("brave_rewards.externalWallet", data);
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

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("result", static_cast<int>(result));
  data.SetStringKey("walletType", wallet_type);
  data.SetStringKey("action", action);

  base::Value new_args(base::Value::Type::DICTIONARY);
  for (auto const& arg : args) {
    new_args.SetStringKey(arg.first, arg.second);
  }
  data.SetKey("args", std::move(new_args));

  CallJavascriptFunction("brave_rewards.processRewardsPageUrl", data);
}

void RewardsDOMHandler::ProcessRewardsPageUrl(base::Value::ConstListView args) {
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

void RewardsDOMHandler::DisconnectWallet(base::Value::ConstListView args) {
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
  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("result", static_cast<int>(result));
  data.SetStringKey("walletType", wallet_type);

  CallJavascriptFunction("brave_rewards.disconnectWallet", data);
}

void RewardsDOMHandler::OnAdsEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool ads_enabled) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  GetAdsData(base::Value::ConstListView());
  GetAutoContributeProperties(base::Value::ConstListView());
  GetOnboardingStatus(base::Value::ConstListView());
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

  base::Value report_base(base::Value::Type::DICTIONARY);
  report_base.SetDoubleKey("grant", report->grants);
  report_base.SetDoubleKey("ads", report->earning_from_ads);
  report_base.SetDoubleKey("contribute", report->auto_contribute);
  report_base.SetDoubleKey("monthly", report->recurring_donation);
  report_base.SetDoubleKey("tips", report->one_time_donation);

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("month", month);
  data.SetIntKey("year", year);
  data.SetKey("report", std::move(report_base));

  CallJavascriptFunction("brave_rewards.balanceReport", data);
}

void RewardsDOMHandler::GetBalanceReport(base::Value::ConstListView args) {
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

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("month", month);
  data.SetIntKey("year", year);

  base::Value balance_report(base::Value::Type::DICTIONARY);
  balance_report.SetDoubleKey("grant", report->balance->grants);
  balance_report.SetDoubleKey("ads", report->balance->earning_from_ads);
  balance_report.SetDoubleKey("contribute", report->balance->auto_contribute);
  balance_report.SetDoubleKey("monthly", report->balance->recurring_donation);
  balance_report.SetDoubleKey("tips", report->balance->one_time_donation);

  base::Value transactions(base::Value::Type::LIST);
  for (const auto& item : report->transactions) {
    base::Value transaction_report(base::Value::Type::DICTIONARY);
    transaction_report.SetDoubleKey("amount", item->amount);
    transaction_report.SetIntKey("type", static_cast<int>(item->type));
    transaction_report.SetIntKey("processor",
                                 static_cast<int>(item->processor));
    transaction_report.SetIntKey("created_at", item->created_at);

    transactions.Append(std::move(transaction_report));
  }

  base::Value contributions(base::Value::Type::LIST);
  for (const auto& contribution : report->contributions) {
    base::Value publishers(base::Value::Type::LIST);
    for (const auto& item : contribution->publishers) {
      base::Value publisher(base::Value::Type::DICTIONARY);
      publisher.SetStringKey("id", item->id);
      publisher.SetDoubleKey("percentage", item->percent);
      publisher.SetDoubleKey("weight", item->weight);
      publisher.SetStringKey("publisherKey", item->id);
      publisher.SetIntKey("status", static_cast<int>(item->status));
      publisher.SetStringKey("name", item->name);
      publisher.SetStringKey("provider", item->provider);
      publisher.SetStringKey("url", item->url);
      publisher.SetStringKey("favIcon", item->favicon_url);
      publishers.Append(std::move(publisher));
    }

    base::Value contribution_report(base::Value::Type::DICTIONARY);
    contribution_report.SetDoubleKey("amount", contribution->amount);
    contribution_report.SetIntKey("type", static_cast<int>(contribution->type));
    contribution_report.SetIntKey("processor",
                                  static_cast<int>(contribution->processor));
    contribution_report.SetIntKey("created_at", contribution->created_at);
    contribution_report.SetKey("publishers", std::move(publishers));
    contributions.Append(std::move(contribution_report));
  }

  base::Value report_base(base::Value::Type::DICTIONARY);
  report_base.SetKey("balance", std::move(balance_report));
  report_base.SetKey("transactions", std::move(transactions));
  report_base.SetKey("contributions", std::move(contributions));

  data.SetKey("report", std::move(report_base));

  CallJavascriptFunction("brave_rewards.monthlyReport", data);
}

void RewardsDOMHandler::GetMonthlyReport(base::Value::ConstListView args) {
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
  base::Value list(base::Value::Type::LIST);
  for (const auto& item : ids) {
    list.Append(base::Value(item));
  }

  CallJavascriptFunction("brave_rewards.monthlyReportIds", list);
}

void RewardsDOMHandler::GetAllMonthlyReportIds(
    base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetAllMonthlyReportIds(
      base::BindOnce(&RewardsDOMHandler::OnGetAllMonthlyReportIds,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetCountryCode(base::Value::ConstListView args) {
  AllowJavascript();

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  CallJavascriptFunction("brave_rewards.countryCode",
                         base::Value(country_code));
}

void RewardsDOMHandler::CompleteReset(base::Value::ConstListView args) {
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

void RewardsDOMHandler::GetPaymentId(base::Value::ConstListView args) {
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

void RewardsDOMHandler::GetWalletPassphrase(base::Value::ConstListView args) {
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

void RewardsDOMHandler::GetOnboardingStatus(base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }
  AllowJavascript();
  base::Value data(base::Value::Type::DICTIONARY);
  data.SetBoolKey("showOnboarding", rewards_service_->ShouldShowOnboarding());
  CallJavascriptFunction("brave_rewards.onboardingStatus", data);
}

void RewardsDOMHandler::SaveOnboardingResult(base::Value::ConstListView args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_)
    return;

  AllowJavascript();
  if (args[0].GetString() == "opted-in")
    rewards_service_->EnableRewards();
}

void RewardsDOMHandler::GetExternalWalletProviders(
    base::Value::ConstListView args) {
  if (!rewards_service_)
    return;

  AllowJavascript();
  base::Value data(base::Value::Type::LIST);

  std::vector<std::string> providers =
      rewards_service_->GetExternalWalletProviders();
  for (std::string provider : providers) {
    data.Append(provider);
  }

  CallJavascriptFunction("brave_rewards.externalWalletProviderList", data);
}

}  // namespace

BraveRewardsPageUI::BraveRewardsPageUI(content::WebUI* web_ui,
                                       const std::string& name)
    : WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kBraveRewardsPageGenerated,
                              kBraveRewardsPageGeneratedSize,
#if defined(OS_ANDROID)
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
