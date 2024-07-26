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
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_util.h"
#include "brave/components/brave_ads/core/public/history/ad_history_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/targeting/geographical/subdivision/supported_subdivisions.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_page_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/country_code_util.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "chrome/browser/browser_process.h"
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
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/base/l10n/l10n_util.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "chrome/browser/ui/browser_finder.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/webui/favicon_source.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "content/public/browser/url_data_source.h"
#endif

using content::WebUIMessageHandler;

namespace {

PrefService* GetLocalState() {
  return g_browser_process->local_state();
}

#if !BUILDFLAG(IS_ANDROID)

brave_rewards::RewardsPanelCoordinator* GetPanelCoordinator(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  if (auto* browser = chrome::FindBrowserWithTab(web_contents)) {
    return brave_rewards::RewardsPanelCoordinator::FromBrowser(browser);
  }
  return nullptr;
}

#endif

// The handler for Javascript messages for Brave about: pages
class RewardsDOMHandler
    : public WebUIMessageHandler,
      public bat_ads::mojom::BatAdsObserver,
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
  void GetUserType(const base::Value::List& args);
  void OnGetUserType(brave_rewards::mojom::UserType user_type);
  void IsTermsOfServiceUpdateRequired(const base::Value::List& args);
  void AcceptTermsOfServiceUpdate(const base::Value::List& args);
  void GetRewardsParameters(const base::Value::List& args);
  void IsAutoContributeSupported(const base::Value::List& args);
  void GetAutoContributeProperties(const base::Value::List& args);
  void GetReconcileStamp(const base::Value::List& args);
  void SaveSetting(const base::Value::List& args);
  void OnPublisherList(
      std::vector<brave_rewards::mojom::PublisherInfoPtr> list);
  void OnExcludedSiteList(
      std::vector<brave_rewards::mojom::PublisherInfoPtr> list);
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
  void OnToggleAdThumbUp(const bool success);
  void ToggleAdThumbDown(const base::Value::List& args);
  void OnToggleAdThumbDown(const bool success);
  void ToggleAdOptIn(const base::Value::List& args);
  void OnToggleAdOptIn(const bool success);
  void ToggleAdOptOut(const base::Value::List& args);
  void OnToggleAdOptOut(const bool success);
  void ToggleSavedAd(const base::Value::List& args);
  void OnToggleSavedAd(const bool success);
  void ToggleFlaggedAd(const base::Value::List& args);
  void OnToggleFlaggedAd(const bool success);
  void SaveAdsSetting(const base::Value::List& args);
  void OnGetContributionAmount(double amount);
  void OnIsAutoContributeSupported(bool is_ac_supported);
  void OnGetAutoContributeProperties(
      brave_rewards::mojom::AutoContributePropertiesPtr properties);
  void OnGetReconcileStamp(uint64_t reconcile_stamp);
  void OnAutoContributePropsReady(
      brave_rewards::mojom::AutoContributePropertiesPtr properties);
  void GetStatement(const base::Value::List& args);
  void GetStatementOfAccounts();
  void OnGetStatementOfAccounts(brave_ads::mojom::StatementInfoPtr statement);
  void GetExcludedSites(const base::Value::List& args);

  void OnGetRecurringTips(
      std::vector<brave_rewards::mojom::PublisherInfoPtr> list);

  void OnGetOneTimeTips(
      std::vector<brave_rewards::mojom::PublisherInfoPtr> list);

  void FetchBalance(const base::Value::List& args);
  void OnFetchBalance(brave_rewards::mojom::BalancePtr balance);

  void GetExternalWallet(const base::Value::List& args);
  void OnGetExternalWallet(brave_rewards::mojom::ExternalWalletPtr wallet);

  void ConnectExternalWallet(const base::Value::List& args);
  void OnConnectExternalWallet(
      brave_rewards::mojom::ConnectExternalWalletResult result);

  void GetBalanceReport(const base::Value::List& args);

  void OnGetBalanceReport(const uint32_t month,
                          const uint32_t year,
                          const brave_rewards::mojom::Result result,
                          brave_rewards::mojom::BalanceReportInfoPtr report);

  void GetCountryCode(const base::Value::List& args);

  void OnGetRewardsParameters(
      brave_rewards::mojom::RewardsParametersPtr parameters);

  void CompleteReset(const base::Value::List& args);

  void GetOnboardingStatus(const base::Value::List& args);
  void EnableRewards(const base::Value::List& args);
  void GetExternalWalletProviders(const base::Value::List& args);

  void BeginExternalWalletLogin(const base::Value::List& args);
  void OnBeginExternalWalletLogin(
      brave_rewards::mojom::ExternalWalletLoginParamsPtr params);

  void ReconnectExternalWallet(const base::Value::List& args);

  void GetIsUnsupportedRegion(const base::Value::List& args);

  void GetPluralString(const base::Value::List& args);

  // RewardsServiceObserver implementation
  void OnRewardsInitialized(
      brave_rewards::RewardsService* rewards_service) override;
  void OnExcludedSitesChanged(brave_rewards::RewardsService* rewards_service,
                              std::string publisher_id,
                              bool excluded) override;
  void OnReconcileComplete(
      brave_rewards::RewardsService* rewards_service,
      const brave_rewards::mojom::Result result,
      const std::string& contribution_id,
      const double amount,
      const brave_rewards::mojom::RewardsType type,
      const brave_rewards::mojom::ContributionProcessor processor) override;

  void OnPublisherListNormalized(
      brave_rewards::RewardsService* rewards_service,
      std::vector<brave_rewards::mojom::PublisherInfoPtr> list) override;

  void OnStatementChanged(
      brave_rewards::RewardsService* rewards_service) override;

  void OnRecurringTipSaved(brave_rewards::RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(brave_rewards::RewardsService* rewards_service,
                             bool success) override;

  void OnExternalWalletLoggedOut() override;

  void OnExternalWalletDisconnected() override;

  void OnRewardsWalletCreated() override;

  void OnTermsOfServiceUpdateAccepted() override;

  void ReconcileStampReset() override;

  void OnCompleteReset(const bool success) override;

  // bat_ads::mojom::BatAdsObserver implementation
  void OnAdRewardsDidChange() override;
  void OnBrowserUpgradeRequiredToServeAds() override;
  void OnIneligibleRewardsWalletToServeAds() override;
  void OnRemindUser(brave_ads::mojom::ReminderType type) override {}

  void InitPrefChangeRegistrar();
  void OnPrefChanged(const std::string& key);

  raw_ptr<brave_rewards::RewardsService> rewards_service_ =
      nullptr;  // NOT OWNED
  base::ScopedObservation<brave_rewards::RewardsService,
                          brave_rewards::RewardsServiceObserver>
      rewards_service_observation_{this};

  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;  // NOT OWNED
  mojo::Receiver<bat_ads::mojom::BatAdsObserver> bat_ads_observer_receiver_{
      this};

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<RewardsDOMHandler> weak_factory_;
};

namespace {

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
      "brave_rewards.getUserType",
      base::BindRepeating(&RewardsDOMHandler::GetUserType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.acceptTermsOfServiceUpdate",
      base::BindRepeating(&RewardsDOMHandler::AcceptTermsOfServiceUpdate,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.isTermsOfServiceUpdateRequired",
      base::BindRepeating(&RewardsDOMHandler::IsTermsOfServiceUpdateRequired,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getRewardsParameters",
      base::BindRepeating(&RewardsDOMHandler::GetRewardsParameters,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.isAutoContributeSupported",
      base::BindRepeating(&RewardsDOMHandler::IsAutoContributeSupported,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getAutoContributeProperties",
      base::BindRepeating(&RewardsDOMHandler::GetAutoContributeProperties,
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
      "brave_rewards.getStatement",
      base::BindRepeating(&RewardsDOMHandler::GetStatement,
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
      "brave_rewards.beginExternalWalletLogin",
      base::BindRepeating(&RewardsDOMHandler::BeginExternalWalletLogin,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.reconnectExternalWallet",
      base::BindRepeating(&RewardsDOMHandler::ReconnectExternalWallet,
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

  if (rewards_service_) {
    rewards_service_->OnRewardsPageShown();
  }

  // Configure a pref change registrar to update brave://rewards when settings
  // are changed via brave://settings
  InitPrefChangeRegistrar();
}

void RewardsDOMHandler::InitPrefChangeRegistrar() {
  Profile* profile = Profile::FromWebUI(web_ui());
  pref_change_registrar_.Init(profile->GetPrefs());

  pref_change_registrar_.Add(
      brave_ads::prefs::kOptedInToNotificationAds,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_ads::prefs::kMaximumNotificationAdsPerHour,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_ads::prefs::kSubdivisionTargetingSubdivision,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_ads::prefs::kOptedInToSearchResultAds,
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
      brave_news::prefs::kBraveNewsOptedIn,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_news::prefs::kNewTabPageShowToday,
      base::BindRepeating(&RewardsDOMHandler::OnPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      ntp_background_images::prefs::
          kNewTabPageShowSponsoredImagesBackgroundImage,
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

void RewardsDOMHandler::GetUserType(const base::Value::List&) {
  if (!IsJavascriptAllowed() || !rewards_service_) {
    return;
  }
  rewards_service_->GetUserType(base::BindOnce(
      &RewardsDOMHandler::OnGetUserType, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::IsTermsOfServiceUpdateRequired(
    const base::Value::List&) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  if (rewards_service_) {
    CallJavascriptFunction("brave_rewards.isTermsOfServiceUpdateRequired",
                           rewards_service_->IsTermsOfServiceUpdateRequired());
  }
}

void RewardsDOMHandler::AcceptTermsOfServiceUpdate(
    const base::Value::List& args) {
  if (rewards_service_) {
    rewards_service_->AcceptTermsOfServiceUpdate();
  }
}

void RewardsDOMHandler::OnGetUserType(
    brave_rewards::mojom::UserType user_type) {
  CallJavascriptFunction("brave_rewards.userType",
                         base::Value(static_cast<int>(user_type)));
}

void RewardsDOMHandler::OnJavascriptAllowed() {
  if (rewards_service_) {
    rewards_service_observation_.Reset();
    rewards_service_observation_.Observe(rewards_service_);
  }

  bat_ads_observer_receiver_.reset();
  if (ads_service_) {
    ads_service_->AddBatAdsObserver(
        bat_ads_observer_receiver_.BindNewPipeAndPassRemote());
  }
}

void RewardsDOMHandler::OnJavascriptDisallowed() {
  rewards_service_observation_.Reset();

  bat_ads_observer_receiver_.reset();

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
    brave_rewards::mojom::RewardsParametersPtr parameters) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  double rate = 0.0, auto_contribute_choice = 0.0;
  base::Value::List auto_contribute_choices;
  base::Value::Dict payout_status;
  base::Value::Dict wallet_provider_regions;
  base::Time vbat_deadline;
  bool vbat_expired = false;

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

    vbat_deadline = parameters->vbat_deadline;
    vbat_expired = parameters->vbat_expired;
  }

  data.Set("rate", rate);
  data.Set("autoContributeChoice", auto_contribute_choice);
  data.Set("autoContributeChoices", std::move(auto_contribute_choices));
  data.Set("payoutStatus", std::move(payout_status));
  data.Set("walletProviderRegions", std::move(wallet_provider_regions));
  if (!vbat_deadline.is_null()) {
    data.Set("vbatDeadline", floor(vbat_deadline.InSecondsFSinceUnixEpoch() *
                                   base::Time::kMillisecondsPerSecond));
  }
  data.Set("vbatExpired", vbat_expired);

  CallJavascriptFunction("brave_rewards.rewardsParameters", data);
}

void RewardsDOMHandler::OnRewardsInitialized(
    brave_rewards::RewardsService* rewards_service) {
  if (!IsJavascriptAllowed())
    return;

  CallJavascriptFunction("brave_rewards.initialized");
}

void RewardsDOMHandler::IsAutoContributeSupported(const base::Value::List&) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->IsAutoContributeSupported(
      base::BindOnce(&RewardsDOMHandler::OnIsAutoContributeSupported,
                     weak_factory_.GetWeakPtr()));
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

void RewardsDOMHandler::BeginExternalWalletLogin(
    const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  if (!rewards_service_)
    return;

  AllowJavascript();
  const std::string wallet_type = args[0].GetString();

  rewards_service_->BeginExternalWalletLogin(
      wallet_type,
      base::BindOnce(&RewardsDOMHandler::OnBeginExternalWalletLogin,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnBeginExternalWalletLogin(
    brave_rewards::mojom::ExternalWalletLoginParamsPtr params) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.externalWalletLogin",
                           base::Value(params ? params->url : ""));
  }
}

void RewardsDOMHandler::ReconnectExternalWallet(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }
  AllowJavascript();
  rewards_service_->BeginExternalWalletLogin(
      rewards_service_->GetExternalWalletType(),
      base::BindOnce(&RewardsDOMHandler::OnBeginExternalWalletLogin,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnIsAutoContributeSupported(bool is_ac_supported) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.onIsAutoContributeSupported",
                           base::Value(is_ac_supported));
  }
}

void RewardsDOMHandler::OnGetAutoContributeProperties(
    brave_rewards::mojom::AutoContributePropertiesPtr properties) {
  if (!IsJavascriptAllowed() || !properties)
    return;

  base::Value::Dict values;
  values.Set("enabledContribute", properties->enabled_contribute);
  values.Set("contributionMinTime",
             static_cast<int>(properties->contribution_min_time));
  values.Set("contributionMinVisits", properties->contribution_min_visits);

  CallJavascriptFunction("brave_rewards.autoContributeProperties", values);
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
    brave_rewards::mojom::AutoContributePropertiesPtr properties) {
  if (!properties) {
    return;
  }

  auto filter = brave_rewards::mojom::ActivityInfoFilter::New();
  auto pair = brave_rewards::mojom::ActivityInfoFilterOrderPair::New(
      "ai.percent", false);
  filter->order_by.push_back(std::move(pair));
  filter->min_duration = properties->contribution_min_time;
  filter->reconcile_stamp = properties->reconcile_stamp;
  filter->excluded =
      brave_rewards::mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED;
  filter->percent = 1;
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

void RewardsDOMHandler::SaveSetting(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  AllowJavascript();

  if (rewards_service_) {
    const std::string key = args[0].GetString();
    const std::string value = args[1].GetString();

    if (key == "contributionMonthly") {
      double double_value = 0;
      if (!base::StringToDouble(value, &double_value)) {
        LOG(ERROR) << "Auto contribution amount not a double";
        return;
      }
      rewards_service_->SetAutoContributionAmount(double_value);
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
    std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
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

  CallJavascriptFunction("brave_rewards.contributeList", publishers);
}

void RewardsDOMHandler::OnExcludedSiteList(
    std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
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

  CallJavascriptFunction("brave_rewards.excludedList", publishers);
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
    const brave_rewards::mojom::Result result,
    const std::string& contribution_id,
    const double amount,
    const brave_rewards::mojom::RewardsType type,
    const brave_rewards::mojom::ContributionProcessor processor) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict complete;
  complete.Set("result", static_cast<int>(result));
  complete.Set("type", static_cast<int>(type));

  CallJavascriptFunction("brave_rewards.reconcileComplete", complete);
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
    std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
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

  CallJavascriptFunction("brave_rewards.recurringTips", publishers);
}

void RewardsDOMHandler::OnGetOneTimeTips(
    std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
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

  CallJavascriptFunction("brave_rewards.currentTips", publishers);
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

  auto* prefs = Profile::FromWebUI(web_ui())->GetPrefs();

  base::Value::Dict ads_data;
  ads_data.Set("adsIsSupported", brave_ads::IsSupportedRegion());
  ads_data.Set(
      "adsPerHour",
      static_cast<double>(ads_service_->GetMaximumNotificationAdsPerHour()));
  ads_data.Set(
      kAdsSubdivisionTargeting,
      prefs->GetString(brave_ads::prefs::kSubdivisionTargetingSubdivision));
  ads_data.Set(
      kAutoDetectedSubdivisionTargeting,
      prefs->GetString(
          brave_ads::prefs::kSubdivisionTargetingAutoDetectedSubdivision));
  ads_data.Set(
      "shouldAllowAdsSubdivisionTargeting",
      prefs->GetBoolean(brave_ads::prefs::kShouldAllowSubdivisionTargeting));
  ads_data.Set("adsUIEnabled", true);
  ads_data.Set("needsBrowserUpgradeToServeAds",
               ads_service_->IsBrowserUpgradeRequiredToServeAds());

  const std::string country_code = brave_l10n::GetCountryCode(GetLocalState());
  ads_data.Set("subdivisions",
               brave_ads::GetSupportedSubdivisionsAsValueList(country_code));

  ads_data.Set("notificationAdsEnabled",
               prefs->GetBoolean(brave_ads::prefs::kOptedInToNotificationAds));

  ads_data.Set(
      "newTabAdsEnabled",
      prefs->GetBoolean(ntp_background_images::prefs::
                            kNewTabPageShowSponsoredImagesBackgroundImage));
  ads_data.Set("searchAdsEnabled",
               prefs->GetBoolean(brave_ads::prefs::kOptedInToSearchResultAds));

  ads_data.Set("newsAdsEnabled",
               prefs->GetBoolean(brave_news::prefs::kBraveNewsOptedIn) &&
                   prefs->GetBoolean(brave_news::prefs::kNewTabPageShowToday));

  CallJavascriptFunction("brave_rewards.adsData", ads_data);
}

void RewardsDOMHandler::GetAdsHistory(const base::Value::List& args) {
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  const base::Time now = base::Time::Now();

  const base::Time from_time =
      now - brave_ads::kAdHistoryRetentionPeriod - base::Days(1);
  const base::Time from_time_at_local_midnight = from_time.LocalMidnight();

  brave_rewards::p3a::RecordAdsHistoryView();

  ads_service_->GetAdHistory(from_time_at_local_midnight, now,
                             base::BindOnce(&RewardsDOMHandler::OnGetAdsHistory,
                                            weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetAdsHistory(base::Value::List ads_history) {
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

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleLikeAd(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleAdThumbUp,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdThumbUp(const bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleAdThumbUp", success);
}

void RewardsDOMHandler::ToggleAdThumbDown(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleDislikeAd(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleAdThumbDown,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdThumbDown(const bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleAdThumbDown", success);
}

void RewardsDOMHandler::ToggleAdOptIn(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleLikeCategory(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleAdOptIn,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdOptIn(const bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleAdOptIn", success);
}

void RewardsDOMHandler::ToggleAdOptOut(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleDislikeCategory(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleAdOptOut,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdOptOut(const bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleAdOptOut", success);
}

void RewardsDOMHandler::ToggleSavedAd(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleSaveAd(dict->Clone(),
                             base::BindOnce(&RewardsDOMHandler::OnToggleSavedAd,
                                            weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleSavedAd(const bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleSavedAd", success);
}

void RewardsDOMHandler::ToggleFlaggedAd(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());

  if (!ads_service_) {
    return;
  }

  const base::Value::Dict* dict = args[0].GetIfDict();
  if (!dict) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  AllowJavascript();

  ads_service_->ToggleMarkAdAsInappropriate(
      dict->Clone(), base::BindOnce(&RewardsDOMHandler::OnToggleFlaggedAd,
                                    weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleFlaggedAd(const bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onToggleFlaggedAd", success);
}

void RewardsDOMHandler::SaveAdsSetting(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  auto* prefs = Profile::FromWebUI(web_ui())->GetPrefs();

  const std::string key = args[0].GetString();
  const std::string value = args[1].GetString();

  if (key == "notificationAdsEnabled") {
    prefs->SetBoolean(brave_ads::prefs::kOptedInToNotificationAds,
                      value == "true");
  } else if (key == "adsPerHour") {
    int64_t int64_value;
    if (!base::StringToInt64(value, &int64_value)) {
      LOG(ERROR) << "Ads per hour was not converted to int64";
      return;
    }

    prefs->SetInt64(brave_ads::prefs::kMaximumNotificationAdsPerHour,
                    int64_value);
  } else if (key == "newTabAdsEnabled") {
    prefs->SetBoolean(ntp_background_images::prefs::
                          kNewTabPageShowSponsoredImagesBackgroundImage,
                      value == "true");
  } else if (key == "searchAdsEnabled") {
    prefs->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds,
                      value == "true");
  } else if (key == kAdsSubdivisionTargeting) {
    prefs->SetString(brave_ads::prefs::kSubdivisionTargetingSubdivision, value);
  }

  GetAdsData(base::Value::List());
}

void RewardsDOMHandler::OnPublisherListNormalized(
    brave_rewards::RewardsService* rewards_service,
    std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
  OnPublisherList(std::move(list));
}

void RewardsDOMHandler::GetStatement(const base::Value::List& args) {
  GetStatementOfAccounts();
}

void RewardsDOMHandler::GetStatementOfAccounts() {
  if (ads_service_) {
    AllowJavascript();
    ads_service_->GetStatementOfAccounts(
        base::BindOnce(&RewardsDOMHandler::OnGetStatementOfAccounts,
                       weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetStatementOfAccounts(
    brave_ads::mojom::StatementInfoPtr statement) {
  if (!statement) {
    return;
  }

  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict dict;
  dict.Set("adsNextPaymentDate",
           statement->next_payment_date.InSecondsFSinceUnixEpoch() * 1000);
  dict.Set("adsReceivedThisMonth", statement->ads_received_this_month);
  dict.Set("adsMinEarningsThisMonth", statement->min_earnings_this_month);
  dict.Set("adsMaxEarningsThisMonth", statement->max_earnings_this_month);
  dict.Set("adsMinEarningsLastMonth", statement->min_earnings_last_month);
  dict.Set("adsMaxEarningsLastMonth", statement->max_earnings_last_month);

  base::Value::Dict ads_summary;
  for (const auto& [ad_type, count] : statement->ads_summary_this_month) {
    ads_summary.Set(ad_type, base::Value(count));
  }
  dict.Set("adTypesReceivedThisMonth", std::move(ads_summary));

  CallJavascriptFunction("brave_rewards.statement", dict);
}

void RewardsDOMHandler::OnStatementChanged(
    brave_rewards::RewardsService* rewards_service) {
  if (IsJavascriptAllowed()) {
    CallJavascriptFunction("brave_rewards.statementChanged");
  }
}

void RewardsDOMHandler::OnAdRewardsDidChange() {
  GetStatementOfAccounts();
}

void RewardsDOMHandler::OnBrowserUpgradeRequiredToServeAds() {
  GetAdsData(base::Value::List());
}

void RewardsDOMHandler::OnIneligibleRewardsWalletToServeAds() {
  // TODO(https://github.com/brave/brave-browser/issues/32201): Add isEligible
  // UI.
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

void RewardsDOMHandler::OnFetchBalance(
    brave_rewards::mojom::BalancePtr balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  if (!balance) {
    CallJavascriptFunction("brave_rewards.balance");
    return;
  }

  base::Value::Dict data;
  data.Set("total", balance->total);
  data.Set("wallets",
           base::Value::Dict(std::move_iterator(balance->wallets.begin()),
                             std::move_iterator(balance->wallets.end())));

  CallJavascriptFunction("brave_rewards.balance", std::move(data));
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
    brave_rewards::mojom::ExternalWalletPtr wallet) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  if (!wallet) {
    CallJavascriptFunction("brave_rewards.onGetExternalWallet");
    return;
  }

  base::Value::Dict wallet_dict;
  wallet_dict.Set("type", wallet->type);
  wallet_dict.Set("address", wallet->address);
  wallet_dict.Set("status", static_cast<int>(wallet->status));
  wallet_dict.Set("userName", wallet->user_name);
  wallet_dict.Set("accountUrl", wallet->account_url);
  wallet_dict.Set("activityUrl", wallet->activity_url);

  CallJavascriptFunction("brave_rewards.onGetExternalWallet", wallet_dict);
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
    brave_rewards::mojom::ConnectExternalWalletResult result) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onConnectExternalWallet",
                         base::Value(static_cast<int>(result)));
}

void RewardsDOMHandler::OnExternalWalletLoggedOut() {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onExternalWalletLoggedOut");
}

void RewardsDOMHandler::OnExternalWalletDisconnected() {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards.onExternalWalletDisconnected");
}

void RewardsDOMHandler::OnRewardsWalletCreated() {
  if (!IsJavascriptAllowed()) {
    return;
  }

  GetAdsData(base::Value::List());
  GetAutoContributeProperties(base::Value::List());
  GetOnboardingStatus(base::Value::List());
  GetUserType(base::Value::List());
  GetExternalWallet(base::Value::List());
  GetCountryCode(base::Value::List());
}

void RewardsDOMHandler::OnTermsOfServiceUpdateAccepted() {
  if (!IsJavascriptAllowed()) {
    return;
  }
  IsTermsOfServiceUpdateRequired(base::Value::List());
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
    const brave_rewards::mojom::Result result,
    brave_rewards::mojom::BalanceReportInfoPtr report) {
  if (!IsJavascriptAllowed() || !report) {
    return;
  }

  base::Value::Dict report_base;
  report_base.Set("ads", report->earning_from_ads);
  report_base.Set("contribute", report->auto_contribute);
  report_base.Set("monthly", report->recurring_donation);
  report_base.Set("tips", report->one_time_donation);

  base::Value::Dict data;
  data.Set("month", static_cast<int>(month));
  data.Set("year", static_cast<int>(year));
  data.Set("report", std::move(report_base));

  CallJavascriptFunction("brave_rewards.balanceReport", data);
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
      {"publisherCountText", IDS_REWARDS_PUBLISHER_COUNT_TEXT}};

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
  CallJavascriptFunction("brave_rewards.onboardingStatus", data);
}

void RewardsDOMHandler::EnableRewards(const base::Value::List& args) {
#if !BUILDFLAG(IS_ANDROID)
  AllowJavascript();
  if (auto* coordinator = GetPanelCoordinator(web_ui()->GetWebContents())) {
    coordinator->ShowRewardsSetup();
  }
#else
  // On Android, a native onboarding modal is displayed when the user navigates
  // to the Rewards page. This message handler should not be called.
  NOTREACHED_IN_MIGRATION();
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

  CallJavascriptFunction("brave_rewards.externalWalletProviderList", data);
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
