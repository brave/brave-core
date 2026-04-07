/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/circular_deque.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_adaptive_captcha/pref_names.h"
#include "brave/components/brave_ads/browser/bat_ads_service_factory.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component.h"
#include "brave/components/brave_ads/browser/device_id/device_id.h"
#include "brave/components/brave_ads/browser/reminder/reminder_util.h"
#include "brave/components/brave_ads/browser/tooltips/ads_tooltips_delegate.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_observer.h"
#include "brave/components/brave_ads/core/browser/virtual_pref/virtual_pref_provider.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_value_util.h"
#include "brave/components/brave_ads/core/public/command_line_switches/command_line_switches_util.h"
#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/user_attention/user_idle_detection/user_idle_detection_feature.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "build/build_config.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/variations/pref_names.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "net/base/network_change_notifier.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/content/rewards_service.h"
#endif

namespace brave_ads {

namespace {

constexpr char kClearDataHistogramName[] = "Brave.Ads.ClearData";

bool WriteOnFileTaskRunner(const base::FilePath& path,
                           const std::string& data) {
  return base::ImportantFileWriter::WriteFileAtomically(
      path, data,
      /*histogram_suffix=*/std::string_view());
}

std::optional<std::string> LoadOnFileTaskRunner(const base::FilePath& path) {
  std::string value;
  if (!base::ReadFileToString(path, &value)) {
    return std::nullopt;
  }

  return value;
}

bool EnsureBaseDirectoryExistsOnFileTaskRunner(const base::FilePath& path) {
  if (base::DirectoryExists(path)) {
    return true;
  }

  return base::CreateDirectory(path);
}

bool DeletePathOnFileTaskRunner(const base::FilePath& path) {
  bool recursive;

  base::File::Info file_info;
  if (base::GetFileInfo(path, &file_info)) {
    recursive = file_info.is_directory;
  } else {
    recursive = false;
  }

  if (recursive) {
    return base::DeletePathRecursively(path);
  }

  return base::DeleteFile(path);
}

}  // namespace

AdsServiceImpl::AdsServiceImpl(
    std::unique_ptr<Delegate> delegate,
    PrefService* prefs,
    PrefService* local_state,
    std::unique_ptr<HttpClient> http_client,
    std::unique_ptr<VirtualPrefProvider::Delegate>
        virtual_pref_provider_delegate,
    std::string_view channel_name,
    const base::FilePath& profile_path,
    std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate,
    std::unique_ptr<DeviceId> device_id,
    std::unique_ptr<BatAdsServiceFactory> bat_ads_service_factory,
    ResourceComponent* resource_component,
    history::HistoryService* history_service,
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
    brave_rewards::RewardsService* rewards_service,
#endif
    HostContentSettingsMap* host_content_settings_map)
    : AdsService(std::move(delegate)),
      prefs_(prefs),
      local_state_(local_state),
      virtual_pref_provider_(std::make_unique<VirtualPrefProvider>(
          prefs_,
          local_state_,
          std::move(virtual_pref_provider_delegate))),
      http_client_(std::move(http_client)),
      channel_name_(channel_name),
      history_service_(history_service),
      host_content_settings_map_(host_content_settings_map),
      ads_tooltips_delegate_(std::move(ads_tooltips_delegate)),
      device_id_(std::move(device_id)),
      bat_ads_service_factory_(std::move(bat_ads_service_factory)),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ads_service_path_(profile_path.AppendASCII("ads_service")),
      bat_ads_client_associated_receiver_(this) {
  CHECK(device_id_);
  CHECK(bat_ads_service_factory_);

  if (!local_state_ || !history_service_) {
    CHECK_IS_TEST();
  }

  if (host_content_settings_map) {
    host_content_settings_map_observation_.Observe(host_content_settings_map);
  } else {
    CHECK_IS_TEST();
  }

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  CHECK(rewards_service);
  rewards_service_observation_.Observe(rewards_service);
#endif

  if (CanStartBatAdsService()) {
    bat_ads_client_notifier_pending_receiver_ =
        bat_ads_client_notifier_remote_.BindNewPipeAndPassReceiver();
  }

  Migrate();

  InitializeNotificationsForCurrentProfile();

  GetDeviceIdAndMaybeStartBatAdsService();

  if (resource_component) {
    resource_component_observation_.Observe(resource_component);
  } else {
    CHECK_IS_TEST();
  }
}

AdsServiceImpl::~AdsServiceImpl() = default;

///////////////////////////////////////////////////////////////////////////////

bool AdsServiceImpl::IsBatAdsServiceBound() const {
  return bat_ads_service_remote_.is_bound();
}

void AdsServiceImpl::RegisterResourceComponents() {
  RegisterResourceComponentsForCurrentCountryCode();

  if (UserHasOptedInToNotificationAds()) {
    // Only utilized for text classification, which requires the user to have
    // joined Brave Rewards and opted into notification ads.
    RegisterResourceComponentsForDefaultLanguageCode();
  }
}

void AdsServiceImpl::Migrate() {
  int64_t ads_per_hour =
      prefs_->GetInt64(prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == 0) {
    prefs_->ClearPref(prefs::kMaximumNotificationAdsPerHour);
    prefs_->SetBoolean(prefs::kOptedInToNotificationAds, false);
  }

  if (!local_state_->HasPrefPath(prefs::kFirstRunAt)) {
    base::Time first_run_at =
        local_state_->HasPrefPath(metrics::prefs::kInstallDate)
            ? base::Time::FromSecondsSinceUnixEpoch(static_cast<double>(
                  local_state_->GetInt64(metrics::prefs::kInstallDate)))
            : base::Time::Now();

    local_state_->SetTime(prefs::kFirstRunAt, first_run_at);
  }
}

void AdsServiceImpl::RegisterResourceComponentsForCurrentCountryCode() {
  if (resource_component_observation_.IsObserving()) {
    resource_component_observation_.GetSource()
        ->RegisterComponentForCountryCode(
            delegate_->GetVariationsCountryCode());
  }
}

void AdsServiceImpl::RegisterResourceComponentsForDefaultLanguageCode() {
  if (resource_component_observation_.IsObserving()) {
    resource_component_observation_.GetSource()
        ->RegisterComponentForLanguageCode(CurrentLanguageCode());
  }
}

bool AdsServiceImpl::UserHasJoinedBraveRewards() const {
  return prefs_->GetBoolean(brave_rewards::prefs::kEnabled);
}

bool AdsServiceImpl::UserHasOptedInToNewTabPageAds() const {
  return prefs_->GetBoolean(
             ntp_background_images::prefs::kNewTabPageShowBackgroundImage) &&
         prefs_->GetBoolean(ntp_background_images::prefs::
                                kNewTabPageShowSponsoredImagesBackgroundImage);
}

bool AdsServiceImpl::UserHasOptedInToNotificationAds() const {
  return prefs_->GetBoolean(brave_rewards::prefs::kEnabled) &&
         prefs_->GetBoolean(prefs::kOptedInToNotificationAds);
}

bool AdsServiceImpl::UserHasOptedInToSearchResultAds() const {
  return prefs_->GetBoolean(prefs::kOptedInToSearchResultAds);
}

void AdsServiceImpl::InitializeNotificationsForCurrentProfile() {
  delegate_->MaybeInitNotificationHelper();
}

void AdsServiceImpl::GetDeviceIdAndMaybeStartBatAdsService() {
  device_id_->GetDeviceId(base::BindOnce(
      &AdsServiceImpl::GetDeviceIdAndMaybeStartBatAdsServiceCallback,
      weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImpl::GetDeviceIdAndMaybeStartBatAdsServiceCallback(
    std::string device_id) {
  sys_info_.device_id = std::move(device_id);

  InitializeLocalStatePrefChangeRegistrar();
  InitializePrefChangeRegistrar();

  MaybeStartBatAdsService();
}

bool AdsServiceImpl::CanStartBatAdsService() const {
  if (UserHasJoinedBraveRewards()) {
    // Always start the service to update brave://ads-internals,
    // brave://rewards, and brave://rewards-internals if the user has joined
    // Brave Rewards, even if all ad units are opted out.
    return true;
  }

  // The user has not joined Brave Rewards, so we only start the service if the
  // user has opted in to new tab takeover, or search result ads.
  return UserHasOptedInToNewTabPageAds() || UserHasOptedInToSearchResultAds();
}

void AdsServiceImpl::MaybeStartBatAdsService() {
  // `is_shutting_down_` is set permanently when `KeyedService::Shutdown` is
  // called and the profile begins tearing down. There is no recovery from that
  // state, so any subsequent attempt to restart the service must be suppressed.
  if (is_shutting_down_ || IsBatAdsServiceBound() || !CanStartBatAdsService()) {
    return;
  }

  StartBatAdsService();
}

void AdsServiceImpl::StartBatAdsService() {
  CHECK(!IsBatAdsServiceBound());

  bat_ads_service_remote_ = bat_ads_service_factory_->Launch();
  bat_ads_service_remote_.set_disconnect_handler(base::BindOnce(
      &AdsServiceImpl::DisconnectHandler, weak_ptr_factory_.GetWeakPtr()));

  CHECK(IsBatAdsServiceBound());

  if (!bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_pending_receiver_ =
        bat_ads_client_notifier_remote_.BindNewPipeAndPassReceiver();
  }

  bat_ads_service_remote_->Create(
      ads_service_path_,
      bat_ads_client_associated_receiver_.BindNewEndpointAndPassRemote(),
      bat_ads_associated_remote_.BindNewEndpointAndPassReceiver(),
      std::move(bat_ads_client_notifier_pending_receiver_),
      base::BindOnce(&AdsServiceImpl::BatAdsServiceCreatedCallback,
                     bat_ads_service_weak_ptr_factory_.GetWeakPtr()));

  bat_ads_associated_remote_.reset_on_disconnect();
  bat_ads_client_notifier_remote_.reset_on_disconnect();

  bat_ads_observer_receiver_.reset();
  AddBatAdsObserver(bat_ads_observer_receiver_.BindNewPipeAndPassRemote());
}

void AdsServiceImpl::DisconnectHandler() {
  VLOG(1) << "Bat Ads Service was disconnected";
  ShutdownAdsService();
}

void AdsServiceImpl::BatAdsServiceCreatedCallback() {
  // Guaranteed by `bat_ads_service_weak_ptr_factory_`. This callback is only
  // reachable while the service is alive, and `ShutdownAdsService` resets
  // `bat_ads_service_remote_` before invalidating the factory.
  CHECK(IsBatAdsServiceBound());

  SetSysInfo();

  SetBuildChannel();

  SetCommandLineSwitches();

  SetContentSettings();

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&EnsureBaseDirectoryExistsOnFileTaskRunner,
                     ads_service_path_),
      base::BindOnce(&AdsServiceImpl::InitializeBasePathDirectoryCallback,
                     bat_ads_service_weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImpl::InitializeBasePathDirectoryCallback(bool success) {
  if (!success) {
    VLOG(0) << "Failed to initialize " << ads_service_path_ << " directory";
    return ShutdownAdsService();
  }

  InitializeRewardsWallet();
}

void AdsServiceImpl::InitializeRewardsWallet() {
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  rewards_service_observation_.GetSource()->GetRewardsWallet(
      base::BindOnce(&AdsServiceImpl::InitializeRewardsWalletCallback,
                     bat_ads_service_weak_ptr_factory_.GetWeakPtr()));
#else
  InitializeRewardsWalletCallback(/*mojom_rewards_wallet=*/nullptr);
#endif
}

void AdsServiceImpl::InitializeRewardsWalletCallback(
    brave_rewards::mojom::RewardsWalletPtr mojom_rewards_wallet) {
  // Guaranteed by `bat_ads_service_weak_ptr_factory_`. `ShutdownAdsService`
  // resets `bat_ads_associated_remote_` before invalidating the factory, so the
  // remote is bound for the lifetime of any pending init callback.
  CHECK(bat_ads_associated_remote_.is_bound());

  if (!mojom_rewards_wallet && UserHasJoinedBraveRewards()) {
    VLOG(0) << "Failed to initialize Brave Rewards wallet";
    return ShutdownAdsService();
  }

  InitializeBatAds(std::move(mojom_rewards_wallet));
}

void AdsServiceImpl::InitializeBatAds(
    brave_rewards::mojom::RewardsWalletPtr mojom_rewards_wallet) {
  // Called synchronously from `InitializeRewardsWalletCallback`, which already
  // checked this invariant.
  CHECK(bat_ads_associated_remote_.is_bound());

  mojom::WalletInfoPtr mojom_wallet;

  if (mojom_rewards_wallet) {
    mojom_wallet = mojom::WalletInfo::New();
    mojom_wallet->payment_id = mojom_rewards_wallet->payment_id;
    mojom_wallet->recovery_seed_base64 =
        base::Base64Encode(mojom_rewards_wallet->recovery_seed);
  }

  bat_ads_associated_remote_->Initialize(
      std::move(mojom_wallet),
      base::BindOnce(&AdsServiceImpl::InitializeBatAdsCallback,
                     bat_ads_service_weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImpl::InitializeBatAdsCallback(bool success) {
  TRACE_EVENT1("brave.ads", "InitializeBatAdsCallback", "success", success);
  if (!success) {
    VLOG(0) << "Failed to initialize Bat Ads";
    return ShutdownAdsService();
  }

  is_bat_ads_initialized_ = true;

  RegisterResourceComponents();

  application_state_monitor_observation_.Observe(
      ApplicationStateMonitor::GetInstance());

  MaybeShowOnboardingNotification();

  MaybeOpenNewTabWithAd();

  CheckIdleStateAfterDelay();

  NotifyDidInitializeAdsService();
}

void AdsServiceImpl::NotifyDidInitializeAdsService() {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyDidInitializeAds();
  }

  observers_.Notify(&AdsServiceObserver::OnDidInitializeAdsService);
}

void AdsServiceImpl::NotifyDidShutdownAdsService() {
  observers_.Notify(&AdsServiceObserver::OnDidShutdownAdsService);
}

void AdsServiceImpl::NotifyDidClearAdsServiceData() {
  observers_.Notify(&AdsServiceObserver::OnDidClearAdsServiceData);
}

void AdsServiceImpl::ClearDataPrefsAndAdsServiceDataAndMaybeRestart(
    ClearDataCallback callback,
    bool shutdown_succeeded) {
  if (!shutdown_succeeded) {
    VLOG(0) << "Failed to clear ads data because Ads Service shutdown failed";
    return std::move(callback).Run(/*success=*/false);
  }

  VLOG(6) << "Clearing ads data";

  // Clear catalog preferences to ensure the catalog is redownloaded on restart
  // because the catalog will be remove from `ads_service`/`database.sqlite`,
  // leaving these preferences orphaned.
  prefs_->ClearPref(prefs::kCatalogId);
  prefs_->ClearPref(prefs::kCatalogVersion);
  prefs_->ClearPref(prefs::kCatalogPing);
  prefs_->ClearPref(prefs::kCatalogLastUpdated);

  // Clear reaction preferences as the history will be removed from
  // `ads_service`/`database.sqlite`, leaving these preferences orphaned.
  prefs_->ClearPref(prefs::kAdReactions);
  prefs_->ClearPref(prefs::kSegmentReactions);
  prefs_->ClearPref(prefs::kSaveAds);
  prefs_->ClearPref(prefs::kMarkedAsInappropriate);

  ClearAdsServiceDataAndMaybeRestart(std::move(callback));
}

void AdsServiceImpl::ClearAllPrefsAndAdsServiceDataAndMaybeRestart(
    ClearDataCallback callback,
    bool shutdown_succeeded) {
  if (!shutdown_succeeded) {
    VLOG(0) << "Failed to clear ads data because Ads Service shutdown failed";
    return std::move(callback).Run(/*success=*/false);
  }
  VLOG(6) << "Clearing ads data";

  // Clear all ads preferences.
  prefs_->ClearPrefsWithPrefixSilently("brave.brave_ads");

  ClearAdsServiceDataAndMaybeRestart(std::move(callback));
}

void AdsServiceImpl::ClearAdsServiceDataAndMaybeRestart(
    ClearDataCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&DeletePathOnFileTaskRunner, ads_service_path_),
      base::BindOnce(
          &AdsServiceImpl::ClearAdsServiceDataAndMaybeRestartCallback,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::ClearAdsServiceDataAndMaybeRestartCallback(
    ClearDataCallback callback,
    bool success) {
  if (!success) {
    VLOG(0) << "Failed to clear ads data";
  } else {
    VLOG(6) << "Cleared ads data";

    NotifyDidClearAdsServiceData();
  }

  std::move(callback).Run(success);

  MaybeStartBatAdsService();
}

void AdsServiceImpl::SetSysInfo() {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->SetSysInfo(sys_info_.Clone());
  }
}

void AdsServiceImpl::SetBuildChannel() {
  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  mojom::BuildChannelInfoPtr mojom_build_channel =
      mojom::BuildChannelInfo::New();
  mojom_build_channel->name = channel_name_;
  mojom_build_channel->is_release = mojom_build_channel->name == "release";

  bat_ads_associated_remote_->SetBuildChannel(std::move(mojom_build_channel));
}

void AdsServiceImpl::SetCommandLineSwitches() {
  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  mojom::CommandLineSwitchesPtr mojom_command_line_switches =
      BuildCommandLineSwitches();
  CHECK(mojom_command_line_switches);
#if BUILDFLAG(IS_ANDROID)
  if (prefs_->GetBoolean(brave_rewards::prefs::kUseRewardsStagingServer)) {
    mojom_command_line_switches->environment_type =
        mojom::EnvironmentType::kStaging;
  }
#endif  // BUILDFLAG(IS_ANDROID)

  bat_ads_associated_remote_->SetCommandLineSwitches(
      std::move(mojom_command_line_switches));
}

void AdsServiceImpl::SetContentSettings() {
  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  if (!host_content_settings_map_) {
    CHECK_IS_TEST();
    return;
  }

  mojom::ContentSettingsPtr mojom_content_settings =
      mojom::ContentSettings::New();
  mojom_content_settings->allow_javascript =
      host_content_settings_map_->GetDefaultContentSetting(
          ContentSettingsType::JAVASCRIPT) == CONTENT_SETTING_ALLOW;

  bat_ads_associated_remote_->SetContentSettings(
      std::move(mojom_content_settings));
}

bool AdsServiceImpl::ShouldShowOnboardingNotification() {
  const bool should_show_onboarding_notification =
      prefs_->GetBoolean(prefs::kShouldShowOnboardingNotification);
  return should_show_onboarding_notification &&
         UserHasOptedInToNotificationAds() && CheckIfCanShowNotificationAds();
}

void AdsServiceImpl::MaybeShowOnboardingNotification() {
  if (!ShouldShowOnboardingNotification()) {
    return;
  }

  if (delegate_->ShowOnboardingNotification()) {
    SetProfilePref(prefs::kShouldShowOnboardingNotification,
                   base::Value(false));
  }
}

void AdsServiceImpl::ShowReminder(mojom::ReminderType mojom_reminder_type) {
  CHECK(mojom::IsKnownEnumValue(mojom_reminder_type));

#if !BUILDFLAG(IS_ANDROID)
  if (UserHasOptedInToNotificationAds() && CheckIfCanShowNotificationAds()) {
    // TODO(https://github.com/brave/brave-browser/issues/29587): Decouple Brave
    // Ads reminders from notification ads.
    ShowNotificationAd(BuildReminder(mojom_reminder_type));
  }
#endif
}

void AdsServiceImpl::CloseAdaptiveCaptcha() {
  delegate_->ClearScheduledCaptcha();
#if !BUILDFLAG(IS_ANDROID)
  ads_tooltips_delegate_->CloseCaptchaTooltip();
#endif  // !BUILDFLAG(IS_ANDROID)
}

void AdsServiceImpl::InitializeLocalStatePrefChangeRegistrar() {
  if (!local_state_) {
    return;
  }
  local_state_pref_change_registrar_.Init(local_state_);

  local_state_pref_change_registrar_.Add(
      variations::prefs::kVariationsCountry,
      base::BindRepeating(&AdsServiceImpl::OnVariationsCountryPrefChanged,
                          base::Unretained(this)));
}

void AdsServiceImpl::InitializePrefChangeRegistrar() {
  pref_change_registrar_.Init(prefs_);

  InitializeBraveRewardsPrefChangeRegistrar();
  InitializeSubdivisionTargetingPrefChangeRegistrar();
  InitializeNewTabPageAdsPrefChangeRegistrar();
  InitializeNotificationAdsPrefChangeRegistrar();
  InitializeSearchResultAdsPrefChangeRegistrar();
}

void AdsServiceImpl::InitializeBraveRewardsPrefChangeRegistrar() {
  pref_change_registrar_.Add(
      brave_rewards::prefs::kEnabled,
      base::BindRepeating(&AdsServiceImpl::NotifyPrefChanged,
                          base::Unretained(this),
                          brave_rewards::prefs::kEnabled));
}

void AdsServiceImpl::InitializeSubdivisionTargetingPrefChangeRegistrar() {
  pref_change_registrar_.Add(
      prefs::kSubdivisionTargetingUserSelectedSubdivision,
      base::BindRepeating(&AdsServiceImpl::NotifyPrefChanged,
                          base::Unretained(this),
                          prefs::kSubdivisionTargetingUserSelectedSubdivision));

  pref_change_registrar_.Add(
      prefs::kSubdivisionTargetingAutoDetectedSubdivision,
      base::BindRepeating(&AdsServiceImpl::NotifyPrefChanged,
                          base::Unretained(this),
                          prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

void AdsServiceImpl::InitializeNewTabPageAdsPrefChangeRegistrar() {
  pref_change_registrar_.Add(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage,
      base::BindRepeating(
          &AdsServiceImpl::OnOptedInToAdsPrefChanged, base::Unretained(this),
          ntp_background_images::prefs::kNewTabPageShowBackgroundImage));

  pref_change_registrar_.Add(
      ntp_background_images::prefs::
          kNewTabPageShowSponsoredImagesBackgroundImage,
      base::BindRepeating(&AdsServiceImpl::OnOptedInToAdsPrefChanged,
                          base::Unretained(this),
                          ntp_background_images::prefs::
                              kNewTabPageShowSponsoredImagesBackgroundImage));
}

void AdsServiceImpl::InitializeNotificationAdsPrefChangeRegistrar() {
  pref_change_registrar_.Add(
      prefs::kOptedInToNotificationAds,
      base::BindRepeating(&AdsServiceImpl::OnOptedInToAdsPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      prefs::kMaximumNotificationAdsPerHour,
      base::BindRepeating(&AdsServiceImpl::NotifyPrefChanged,
                          base::Unretained(this),
                          prefs::kMaximumNotificationAdsPerHour));
}

void AdsServiceImpl::InitializeSearchResultAdsPrefChangeRegistrar() {
  pref_change_registrar_.Add(
      prefs::kOptedInToSearchResultAds,
      base::BindRepeating(&AdsServiceImpl::OnOptedInToAdsPrefChanged,
                          base::Unretained(this)));
}

void AdsServiceImpl::OnOptedInToAdsPrefChanged(const std::string& path) {
  if (!CanStartBatAdsService()) {
    return ShutdownAdsService();
  }

  if (IsBatAdsServiceBound() && UserHasOptedInToNotificationAds() &&
      path == prefs::kOptedInToNotificationAds) {
    // Register language resource components if the user has joined Brave
    // Rewards, opted into notification ads, and the Bat Ads Service has
    // already started.
    RegisterResourceComponentsForDefaultLanguageCode();

    InitializeNotificationsForCurrentProfile();
  }

  MaybeStartBatAdsService();

  NotifyPrefChanged(path);
}

void AdsServiceImpl::OnVariationsCountryPrefChanged() {
  RegisterResourceComponentsForCurrentCountryCode();
}

void AdsServiceImpl::NotifyPrefChanged(const std::string& path) const {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyPrefDidChange(path);
  }
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
void AdsServiceImpl::GetRewardsWallet() {
  rewards_service_observation_.GetSource()->GetRewardsWallet(
      base::BindOnce(&AdsServiceImpl::NotifyRewardsWalletDidUpdate,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImpl::NotifyRewardsWalletDidUpdate(
    brave_rewards::mojom::RewardsWalletPtr mojom_rewards_wallet) {
  if (mojom_rewards_wallet && bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyRewardsWalletDidUpdate(
        mojom_rewards_wallet->payment_id,
        base::Base64Encode(mojom_rewards_wallet->recovery_seed));
  }
}
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

void AdsServiceImpl::CheckIdleStateAfterDelay() {
#if !BUILDFLAG(IS_ANDROID)
  idle_state_timer_.Stop();

  idle_state_timer_.Start(FROM_HERE, base::Seconds(1), this,
                          &AdsServiceImpl::CheckIdleState);
#endif
}

void AdsServiceImpl::CheckIdleState() {
  const int64_t idle_threshold = kUserIdleDetectionThreshold.Get().InSeconds();
  ProcessIdleState(ui::CalculateIdleState(static_cast<int>(idle_threshold)),
                   last_idle_time_);
  last_idle_time_ = base::Seconds(ui::CalculateIdleTime());
}

void AdsServiceImpl::ProcessIdleState(const ui::IdleState idle_state,
                                      base::TimeDelta idle_time) {
  if (idle_state == last_idle_state_) {
    return;
  }

  switch (idle_state) {
    case ui::IdleState::IDLE_STATE_ACTIVE: {
      const bool screen_was_locked =
          last_idle_state_ == ui::IdleState::IDLE_STATE_LOCKED;
      if (bat_ads_client_notifier_remote_.is_bound()) {
        bat_ads_client_notifier_remote_->NotifyUserDidBecomeActive(
            idle_time, screen_was_locked);
      }

      break;
    }

    case ui::IdleState::IDLE_STATE_IDLE:
    case ui::IdleState::IDLE_STATE_LOCKED: {
      if (bat_ads_client_notifier_remote_.is_bound()) {
        bat_ads_client_notifier_remote_->NotifyUserDidBecomeIdle();
      }

      break;
    }

    case ui::IdleState::IDLE_STATE_UNKNOWN: {
      break;
    }
  }

  last_idle_state_ = idle_state;
}

bool AdsServiceImpl::CheckIfCanShowNotificationAds() {
  if (!base::FeatureList::IsEnabled(kNotificationAdFeature)) {
    VLOG(1) << "Notification not made: Ad notifications feature is disabled";
    return false;
  }

  return delegate_->CanShowNotifications();
}

void AdsServiceImpl::StartNotificationAdTimeOutTimer(
    const std::string& placement_id) {
  const base::TimeDelta timeout = kNotificationAdTimeout.Get();
  if (timeout.is_zero()) {
    // Never time out.
    return;
  }

  notification_ad_timers_[placement_id] =
      std::make_unique<base::OneShotTimer>();
  notification_ad_timers_[placement_id]->Start(
      FROM_HERE, timeout,
      base::BindOnce(&AdsServiceImpl::NotificationAdTimedOut,
                     weak_ptr_factory_.GetWeakPtr(), placement_id));

  VLOG(6) << "Timeout notification ad with placement id " << placement_id
          << " in " << timeout;
}

bool AdsServiceImpl::StopNotificationAdTimeOutTimer(
    const std::string& placement_id) {
  const auto iter = notification_ad_timers_.find(placement_id);
  if (iter == notification_ad_timers_.cend()) {
    return false;
  }

  notification_ad_timers_.erase(iter);

  return true;
}

void AdsServiceImpl::NotificationAdTimedOut(const std::string& placement_id) {
  VLOG(2) << "Timed-out notification ad with placement id " << placement_id;

  CloseNotificationAd(placement_id);

  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  bat_ads_associated_remote_->TriggerNotificationAdEvent(
      placement_id, mojom::NotificationAdEventType::kTimedOut,
      /*intentional*/ base::DoNothing());
}

void AdsServiceImpl::CloseAllNotificationAds() {
  const auto& list = prefs_->GetList(prefs::kNotificationAds);

  const base::circular_deque<NotificationAdInfo> ads =
      NotificationAdsFromValue(list);

  for (const auto& ad : ads) {
    CloseNotificationAd(ad.placement_id);
  }

  prefs_->SetList(prefs::kNotificationAds, {});
}

void AdsServiceImpl::MaybeOpenNewTabWithAd() {
  if (!retry_opening_new_tab_for_ad_with_placement_id_) {
    return;
  }

  OpenNewTabWithAd(*retry_opening_new_tab_for_ad_with_placement_id_);

  retry_opening_new_tab_for_ad_with_placement_id_.reset();
}

void AdsServiceImpl::OpenNewTabWithAd(const std::string& placement_id) {
  if (StopNotificationAdTimeOutTimer(placement_id)) {
    VLOG(2) << "Canceled timeout for notification ad with placement id "
            << placement_id;
  }

  if (IsReminder(placement_id)) {
    const GURL target_url = GetReminderTargetUrl();
    OpenNewTabWithUrl(target_url);
    return CloseNotificationAd(placement_id);
  }

  if (!is_bat_ads_initialized_) {
    return RetryOpeningNewTabWithAd(placement_id);
  }

  bat_ads_associated_remote_->MaybeGetNotificationAd(
      placement_id, base::BindOnce(&AdsServiceImpl::OpenNewTabWithAdCallback,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImpl::OpenNewTabWithAdCallback(
    std::optional<base::DictValue> dict) {
  if (!dict) {
    return VLOG(0) << "Failed to get notification ad";
  }

  const NotificationAdInfo notification_ad = NotificationAdFromValue(*dict);

  OpenNewTabWithUrl(notification_ad.target_url);
}

void AdsServiceImpl::OpenNewTabWithUrl(const GURL& url) {
  if (is_shutting_down_) {
    return;
  }

  if (!url.is_valid()) {
    VLOG(0) << "Failed to open new tab due to invalid URL: " << url;
    return;
  }

  delegate_->OpenNewTabWithUrl(url);
}

void AdsServiceImpl::RetryOpeningNewTabWithAd(const std::string& placement_id) {
  VLOG(2) << "Retry opening new tab for ad with placement id " << placement_id;
  retry_opening_new_tab_for_ad_with_placement_id_ = placement_id;
}

void AdsServiceImpl::ShowScheduledCaptchaCallback(
    const std::string& payment_id,
    const std::string& captcha_id) {
  delegate_->ShowScheduledCaptcha(payment_id, captcha_id);
}

void AdsServiceImpl::SnoozeScheduledCaptchaCallback() {
  delegate_->SnoozeScheduledCaptcha();
}

void AdsServiceImpl::ShutdownAds(ShutdownCallback callback) {
  if (!bat_ads_associated_remote_) {
    return std::move(callback).Run(/*success=*/true);
  }

  // Use `weak_ptr_factory_` because `bat_ads_service_weak_ptr_factory_` is
  // invalidated to cancel pending startups; this callback must always fire.
  bat_ads_associated_remote_->Shutdown(
      base::BindOnce(&AdsServiceImpl::ShutdownAdsCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::ShutdownAdsCallback(ShutdownCallback callback,
                                         bool success) {
  ShutdownAdsService();

  std::move(callback).Run(success);
}

void AdsServiceImpl::ShutdownAdsService() {
  TRACE_EVENT("brave.ads", "AdsServiceImpl::ShutdownAdsService");
  if (is_bat_ads_initialized_) {
    VLOG(2) << "Shutting down Bat Ads Service";
  }

  // Invalidate weak pointers before resetting mojo endpoints so no in-flight
  // callback fires against a partially torn-down service.
  bat_ads_service_weak_ptr_factory_.InvalidateWeakPtrs();

  bat_ads_client_notifier_remote_.reset();
  bat_ads_client_notifier_pending_receiver_.reset();
  bat_ads_associated_remote_.reset();
  bat_ads_client_associated_receiver_.reset();
  bat_ads_service_remote_.reset();

  if (http_client_) {
    http_client_->CancelRequests();
  }

  idle_state_timer_.Stop();

  notification_ad_timers_.clear();

  application_state_monitor_observation_.Reset();

  CloseAllNotificationAds();

  CloseAdaptiveCaptcha();

  if (is_bat_ads_initialized_) {
    VLOG(2) << "Shutdown Bat Ads Service";

    NotifyDidShutdownAdsService();
  }

  is_bat_ads_initialized_ = false;
}

void AdsServiceImpl::Shutdown() {
  // The profile is being destroyed and the service must never start again, so
  // this is never reset to false.
  is_shutting_down_ = true;

  ShutdownAdsService();
}

void AdsServiceImpl::AddBatAdsObserver(
    mojo::PendingRemote<bat_ads::mojom::BatAdsObserver>
        bat_ads_observer_pending_remote) {
  CHECK(bat_ads_observer_pending_remote.is_valid());

  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->AddBatAdsObserver(
        std::move(bat_ads_observer_pending_remote));
  }
}

bool AdsServiceImpl::IsBrowserUpgradeRequiredToServeAds() const {
  return browser_upgrade_required_to_serve_ads_;
}

int64_t AdsServiceImpl::GetMaximumNotificationAdsPerHour() const {
  int64_t ads_per_hour =
      prefs_->GetInt64(prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == -1) {
    ads_per_hour = kDefaultNotificationAdsPerHour.Get();
  }

  return ads_per_hour;
}

void AdsServiceImpl::OnNotificationAdShown(const std::string& placement_id) {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->TriggerNotificationAdEvent(
        placement_id, mojom::NotificationAdEventType::kViewedImpression,
        /*intentional*/ base::DoNothing());
  }
}

void AdsServiceImpl::OnNotificationAdClosed(const std::string& placement_id,
                                            bool by_user) {
  if (StopNotificationAdTimeOutTimer(placement_id)) {
    VLOG(2) << "Canceled timeout for notification ad with placement id "
            << placement_id;
  }

  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->TriggerNotificationAdEvent(
        placement_id,
        by_user ? mojom::NotificationAdEventType::kDismissed
                : mojom::NotificationAdEventType::kTimedOut,
        /*intentional*/ base::DoNothing());
  }
}

void AdsServiceImpl::OnNotificationAdClicked(const std::string& placement_id) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  OpenNewTabWithAd(placement_id);

  bat_ads_associated_remote_->TriggerNotificationAdEvent(
      placement_id, mojom::NotificationAdEventType::kClicked,
      /*intentional*/ base::DoNothing());
}

void AdsServiceImpl::ClearData(ClearDataCallback callback) {
  UMA_HISTOGRAM_BOOLEAN(kClearDataHistogramName, true);
  ShutdownAds(base::BindOnce(
      &AdsServiceImpl::ClearAllPrefsAndAdsServiceDataAndMaybeRestart,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::GetInternals(GetInternalsCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*internals=*/std::nullopt);
  }

  bat_ads_associated_remote_->GetInternals(
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*internals=*/std::nullopt));
}

void AdsServiceImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*diagnostics*/ std::nullopt);
  }

  bat_ads_associated_remote_->GetDiagnostics(std::move(callback));
}

void AdsServiceImpl::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*statement*/ nullptr);
  }

  bat_ads_associated_remote_->GetStatementOfAccounts(
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*statement*/ nullptr));
}

void AdsServiceImpl::ParseAndSaveNewTabPageAds(
    base::DictValue dict,
    ParseAndSaveNewTabPageAdsCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->ParseAndSaveNewTabPageAds(std::move(dict),
                                                        std::move(callback));
}

void AdsServiceImpl::MaybeServeNewTabPageAd(
    MaybeServeMojomNewTabPageAdCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*ad=*/nullptr);
  }

  bat_ads_associated_remote_->MaybeServeNewTabPageAd(std::move(callback));
}

void AdsServiceImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    mojom::NewTabPageAdMetricType mojom_ad_metric_type,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(mojom_ad_event_type));

  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->TriggerNewTabPageAdEvent(
      placement_id, creative_instance_id, mojom_ad_metric_type,
      mojom_ad_event_type, std::move(callback));
}

void AdsServiceImpl::MaybeGetSearchResultAd(
    const std::string& placement_id,
    MaybeGetSearchResultAdCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*mojom_creative_ad*/ {});
  }

  bat_ads_associated_remote_->MaybeGetSearchResultAd(placement_id,
                                                     std::move(callback));
}

void AdsServiceImpl::TriggerSearchResultAdEvent(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    mojom::SearchResultAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(mojom_ad_event_type));

  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->TriggerSearchResultAdEvent(
      std::move(mojom_creative_ad), mojom_ad_event_type, std::move(callback));
}

void AdsServiceImpl::PurgeOrphanedAdEventsForType(
    mojom::AdType mojom_ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  CHECK(mojom::IsKnownEnumValue(mojom_ad_type));

  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->PurgeOrphanedAdEventsForType(mojom_ad_type,
                                                           std::move(callback));
}

void AdsServiceImpl::GetAdHistory(base::Time from_time,
                                  base::Time to_time,
                                  GetAdHistoryForUICallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*ad_history*/ std::nullopt);
  }

  bat_ads_associated_remote_->GetAdHistory(
      from_time, to_time,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*ad_history*/ std::nullopt));
}

void AdsServiceImpl::ToggleLikeAd(mojom::ReactionInfoPtr mojom_reaction,
                                  ToggleReactionCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->ToggleLikeAd(
      std::move(mojom_reaction),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success*/ false));
}

void AdsServiceImpl::ToggleDislikeAd(mojom::ReactionInfoPtr mojom_reaction,
                                     ToggleReactionCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->ToggleDislikeAd(
      std::move(mojom_reaction),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success*/ false));
}

void AdsServiceImpl::ToggleLikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                                       ToggleReactionCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->ToggleLikeSegment(
      std::move(mojom_reaction),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success*/ false));
}

void AdsServiceImpl::ToggleDislikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                                          ToggleReactionCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->ToggleDislikeSegment(
      std::move(mojom_reaction),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success*/ false));
}

void AdsServiceImpl::ToggleSaveAd(mojom::ReactionInfoPtr mojom_reaction,
                                  ToggleReactionCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->ToggleSaveAd(
      std::move(mojom_reaction),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success*/ false));
}

void AdsServiceImpl::ToggleMarkAdAsInappropriate(
    mojom::ReactionInfoPtr mojom_reaction,
    ToggleReactionCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->ToggleMarkAdAsInappropriate(
      std::move(mojom_reaction),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success*/ false));
}

void AdsServiceImpl::NotifyTabTextContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabTextContentDidChange(
        tab_id, redirect_chain, text);
  }
}

void AdsServiceImpl::NotifyTabDidStartPlayingMedia(int32_t tab_id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabDidStartPlayingMedia(tab_id);
  }
}

void AdsServiceImpl::NotifyTabDidStopPlayingMedia(int32_t tab_id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabDidStopPlayingMedia(tab_id);
  }
}

void AdsServiceImpl::NotifyTabDidChange(int32_t tab_id,
                                        const std::vector<GURL>& redirect_chain,
                                        bool is_new_navigation,
                                        bool is_restoring,
                                        bool is_visible) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabDidChange(
        tab_id, redirect_chain, is_new_navigation, is_restoring, is_visible);
  }
}

void AdsServiceImpl::NotifyTabDidLoad(int32_t tab_id, int http_status_code) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabDidLoad(tab_id, http_status_code);
  }
}

void AdsServiceImpl::NotifyDidCloseTab(int32_t tab_id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyDidCloseTab(tab_id);
  }
}

void AdsServiceImpl::NotifyUserGestureEventTriggered(int32_t page_transition) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyUserGestureEventTriggered(
        page_transition);
  }
}

void AdsServiceImpl::NotifyBrowserDidBecomeActive() {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyBrowserDidBecomeActive();
  }
}

void AdsServiceImpl::NotifyBrowserDidResignActive() {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyBrowserDidResignActive();
  }
}

void AdsServiceImpl::NotifyDidSolveAdaptiveCaptcha() {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyDidSolveAdaptiveCaptcha();
  }
}

void AdsServiceImpl::IsNetworkConnectionAvailable(
    IsNetworkConnectionAvailableCallback callback) {
  std::move(callback).Run(!net::NetworkChangeNotifier::IsOffline());
}

void AdsServiceImpl::IsBrowserActive(IsBrowserActiveCallback callback) {
  std::move(callback).Run(
      ApplicationStateMonitor::GetInstance()->IsBrowserActive());
}

void AdsServiceImpl::IsBrowserInFullScreenMode(
    IsBrowserInFullScreenModeCallback callback) {
  std::move(callback).Run(delegate_->IsFullScreenMode());
}

void AdsServiceImpl::CanShowNotificationAds(
    CanShowNotificationAdsCallback callback) {
  std::move(callback).Run(CheckIfCanShowNotificationAds());
}

void AdsServiceImpl::CanShowNotificationAdsWhileBrowserIsBackgrounded(
    CanShowNotificationAdsWhileBrowserIsBackgroundedCallback callback) {
  std::move(callback).Run(
      delegate_->CanShowSystemNotificationsWhileBrowserIsBackgrounded());
}

void AdsServiceImpl::ShowNotificationAd(base::DictValue dict) {
  const NotificationAdInfo ad = NotificationAdFromValue(dict);

  std::u16string title;
  if (base::IsStringUTF8(ad.title)) {
    title = base::UTF8ToUTF16(ad.title);
  }

  std::u16string body;
  if (base::IsStringUTF8(ad.body)) {
    body = base::UTF8ToUTF16(ad.body);
  }

  delegate_->ShowNotificationAd(ad.placement_id, title, body);

  StartNotificationAdTimeOutTimer(ad.placement_id);
}

void AdsServiceImpl::CloseNotificationAd(const std::string& placement_id) {
  delegate_->CloseNotificationAd(placement_id);
}

void AdsServiceImpl::GetSiteHistory(int max_count,
                                    int recent_day_range,
                                    GetSiteHistoryCallback callback) {
  const std::u16string search_text;
  history::QueryOptions options;
  options.SetRecentDayRange(recent_day_range);
  options.max_count = max_count;
  options.duplicate_policy = history::QueryOptions::REMOVE_ALL_DUPLICATES;
  history_service_->QueryHistory(
      search_text, options,
      base::BindOnce(
          [](GetSiteHistoryCallback callback, history::QueryResults results) {
            SiteHistoryList site_history;
            for (const auto& result : results) {
              site_history.push_back(result.url().GetWithEmptyPath());
            }

            std::ranges::sort(site_history);
            auto to_remove = std::ranges::unique(site_history);
            site_history.erase(to_remove.begin(), to_remove.end());
            std::move(callback).Run(site_history);
          },
          std::move(callback)),
      &history_service_task_tracker_);
}

void AdsServiceImpl::UrlRequest(mojom::UrlRequestInfoPtr url_request,
                                UrlRequestCallback callback) {
  if (http_client_) {
    http_client_->SendRequest(std::move(url_request), std::move(callback));
  }
}

void AdsServiceImpl::Save(const std::string& name,
                          const std::string& value,
                          SaveCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&WriteOnFileTaskRunner,
                     ads_service_path_.AppendASCII(name), value),
      std::move(callback));
}

void AdsServiceImpl::Remove(const std::string& name, RemoveCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::DeleteFile, ads_service_path_.AppendASCII(name)),
      std::move(callback));
}

void AdsServiceImpl::Load(const std::string& name, LoadCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner,
                     ads_service_path_.AppendASCII(name)),
      std::move(callback));
}

void AdsServiceImpl::LoadResourceComponent(
    const std::string& id,
    int version,
    LoadResourceComponentCallback callback) {
  if (!resource_component_observation_.IsObserving()) {
    return std::move(callback).Run({});
  }

  std::optional<base::FilePath> file_path =
      resource_component_observation_.GetSource()->MaybeGetPath(id, version);
  if (!file_path) {
    return std::move(callback).Run({});
  }

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](const base::FilePath& path,
             scoped_refptr<base::SequencedTaskRunner> file_task_runner) {
            std::unique_ptr<base::File, base::OnTaskRunnerDeleter> file(
                new base::File(path, base::File::Flags::FLAG_OPEN |
                                         base::File::Flags::FLAG_READ),
                base::OnTaskRunnerDeleter(std::move(file_task_runner)));
            return file;
          },
          std::move(*file_path), file_task_runner_),
      base::BindOnce(
          [](LoadResourceComponentCallback callback,
             std::unique_ptr<base::File, base::OnTaskRunnerDeleter> file) {
            CHECK(file);
            std::move(callback).Run(std::move(*file));
          },
          std::move(callback)));
}

void AdsServiceImpl::ShowScheduledCaptcha(const std::string& payment_id,
                                          const std::string& captcha_id) {
#if BUILDFLAG(IS_ANDROID)
  ShowScheduledCaptchaCallback(payment_id, captcha_id);
#else   // BUILDFLAG(IS_ANDROID)
  if (prefs_->GetBoolean(
          brave_adaptive_captcha::prefs::kScheduledCaptchaPaused)) {
    return VLOG(1) << "Ads paused; support intervention required";
  }

  CHECK(ads_tooltips_delegate_);

  ads_tooltips_delegate_->ShowCaptchaTooltip(
      payment_id, captcha_id, /*include_cancel_button=*/true,
      base::BindOnce(&AdsServiceImpl::ShowScheduledCaptchaCallback,
                     weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&AdsServiceImpl::SnoozeScheduledCaptchaCallback,
                     weak_ptr_factory_.GetWeakPtr()));
#endif  // !BUILDFLAG(IS_ANDROID)
}

void AdsServiceImpl::FindProfilePref(const std::string& path,
                                     FindProfilePrefCallback callback) {
  std::move(callback).Run(prefs_->FindPreference(path) != nullptr);
}

void AdsServiceImpl::GetProfilePref(const std::string& path,
                                    GetProfilePrefCallback callback) {
  std::move(callback).Run(prefs_->GetValue(path).Clone());
}

void AdsServiceImpl::SetProfilePref(const std::string& path,
                                    base::Value value) {
  prefs_->Set(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::ClearProfilePref(const std::string& path) {
  prefs_->ClearPref(path);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::HasProfilePrefPath(const std::string& path,
                                        HasProfilePrefPathCallback callback) {
  std::move(callback).Run(prefs_->HasPrefPath(path));
}

void AdsServiceImpl::FindLocalStatePref(const std::string& path,
                                        FindLocalStatePrefCallback callback) {
  std::move(callback).Run(local_state_->FindPreference(path) != nullptr);
}

void AdsServiceImpl::GetLocalStatePref(const std::string& path,
                                       GetLocalStatePrefCallback callback) {
  std::move(callback).Run(local_state_->GetValue(path).Clone());
}

void AdsServiceImpl::SetLocalStatePref(const std::string& path,
                                       base::Value value) {
  local_state_->Set(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::ClearLocalStatePref(const std::string& path) {
  local_state_->ClearPref(path);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::HasLocalStatePrefPath(
    const std::string& path,
    HasLocalStatePrefPathCallback callback) {
  std::move(callback).Run(local_state_->HasPrefPath(path));
}

void AdsServiceImpl::GetVirtualPrefs(GetVirtualPrefsCallback callback) {
  std::move(callback).Run(virtual_pref_provider_->GetPrefs());
}

void AdsServiceImpl::Log(const std::string& file,
                         int32_t line,
                         int32_t verbose_level,
                         const std::string& message) {
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  rewards_service_observation_.GetSource()->WriteDiagnosticLog(
      file, line, verbose_level, message);
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

  const int vlog_level =
      ::logging::GetVlogLevelHelper(file.c_str(), file.length());
  if (verbose_level <= vlog_level) {
    ::logging::LogMessage(file.c_str(), line, -verbose_level).stream()
        << message;
  }
}

void AdsServiceImpl::OnBrowserUpgradeRequiredToServeAds() {
  browser_upgrade_required_to_serve_ads_ = true;
}

void AdsServiceImpl::OnRemindUser(mojom::ReminderType mojom_reminder_type) {
  CHECK(mojom::IsKnownEnumValue(mojom_reminder_type));

  ShowReminder(mojom_reminder_type);
}

void AdsServiceImpl::OnBrowserDidBecomeActive() {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyBrowserDidEnterForeground();
#if BUILDFLAG(IS_ANDROID)
    bat_ads_client_notifier_remote_->NotifyBrowserDidBecomeActive();
#endif  // BUILDFLAG(IS_ANDROID)
  }
}

void AdsServiceImpl::OnBrowserDidResignActive() {
  if (bat_ads_client_notifier_remote_.is_bound()) {
#if BUILDFLAG(IS_ANDROID)
    bat_ads_client_notifier_remote_->NotifyBrowserDidResignActive();
#endif  // BUILDFLAG(IS_ANDROID)
    bat_ads_client_notifier_remote_->NotifyBrowserDidEnterBackground();
  }
}

void AdsServiceImpl::OnResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyResourceComponentDidChange(
        manifest_version, id);
  }
}

void AdsServiceImpl::OnDidUnregisterResourceComponent(const std::string& id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyDidUnregisterResourceComponent(id);
  }
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
void AdsServiceImpl::OnRewardsWalletCreated() {
  GetRewardsWallet();
}

void AdsServiceImpl::OnExternalWalletConnected() {
  ShowReminder(mojom::ReminderType::kExternalWalletConnected);

  ShutdownAds(base::BindOnce(
      &AdsServiceImpl::ClearDataPrefsAndAdsServiceDataAndMaybeRestart,
      weak_ptr_factory_.GetWeakPtr(), /*intentional*/ base::DoNothing()));
}

void AdsServiceImpl::OnCompleteReset(bool success) {
  if (success) {
    ShutdownAds(base::BindOnce(
        &AdsServiceImpl::ClearAllPrefsAndAdsServiceDataAndMaybeRestart,
        weak_ptr_factory_.GetWeakPtr(), /*intentional*/ base::DoNothing()));
  }
}
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

void AdsServiceImpl::OnContentSettingChanged(
    const ContentSettingsPattern& /*primary_pattern*/,
    const ContentSettingsPattern& /*secondary_pattern*/,
    ContentSettingsTypeSet content_type_set) {
  if (content_type_set.Contains(ContentSettingsType::JAVASCRIPT)) {
    SetContentSettings();
  }
}

}  // namespace brave_ads
