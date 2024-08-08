/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/circular_deque.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/browser/brave_ads/ad_units/notification_ad/notification_ad_platform_bridge.h"
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_ads/browser/ad_units/notification_ad/custom_notification_ad_feature.h"
#include "brave/components/brave_ads/browser/analytics/p2a/p2a.h"
#include "brave/components/brave_ads/browser/analytics/p3a/notification_ad.h"
#include "brave/components/brave_ads/browser/bat_ads_service_factory.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component.h"
#include "brave/components/brave_ads/browser/device_id/device_id.h"
#include "brave/components/brave_ads/browser/reminder/reminder_util.h"
#include "brave/components/brave_ads/browser/user_engagement/ad_events/ad_event_cache_helper.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_ads/core/public/database/database.h"
#include "brave/components/brave_ads/core/public/flags/flags_util.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/user_attention/user_idle_detection/user_idle_detection_feature.h"
#include "brave/components/brave_ads/resources/grit/bat_ads_resources.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom-forward.h"
#include "brave/components/l10n/common/country_code_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "build/build_config.h"  // IWYU pragma: keep
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/fullscreen.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#endif
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_adaptive_captcha/pref_names.h"
#include "brave/components/brave_ads/browser/tooltips/ads_tooltips_delegate.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/notifications/brave_notification_platform_bridge_helper_android.h"
#include "chrome/browser/android/service_tab_launcher.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "content/public/browser/page_navigator.h"
#endif

namespace brave_ads {

namespace {

constexpr int kMaximumNumberOfTimesToRetryNetworkRequests = 1;

constexpr char kNotificationAdUrlPrefix[] = "https://www.brave.com/ads/?";

int ResourceBundleId(const std::string& name) {
  if (name == kCatalogJsonSchemaDataResourceName) {
    return IDR_ADS_CATALOG_SCHEMA;
  }

  NOTREACHED_NORETURN();
}

std::string URLMethodToRequestType(mojom::UrlRequestMethodType method) {
  CHECK(mojom::IsKnownEnumValue(method));

  switch (method) {
    case mojom::UrlRequestMethodType::kGet: {
      return "GET";
    }

    case mojom::UrlRequestMethodType::kPost: {
      return "POST";
    }

    case mojom::UrlRequestMethodType::kPut: {
      return "PUT";
    }
  }
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

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ads_service_impl", R"(
      semantics {
        sender: "Brave Ads Service"
        description:
          "This service is used to communicate with Brave servers "
          "to send and retrieve information for Ads."
        trigger:
          "Triggered by user viewing ads or at various intervals."
        data:
          "Ads catalog and Confirmations."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature by visiting brave://rewards."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

void OnUrlLoaderResponseStartedCallback(
    const GURL& /*final_url*/,
    const network::mojom::URLResponseHead& response_head) {
  if (response_head.headers->response_code() == -1) {
    VLOG(6) << "Response headers are malformed!!";
  }
}

}  // namespace

AdsServiceImpl::AdsServiceImpl(
    Profile* profile,
    PrefService* local_state,
    brave_adaptive_captcha::BraveAdaptiveCaptchaService*
        adaptive_captcha_service,
    std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate,
    std::unique_ptr<DeviceId> device_id,
    std::unique_ptr<BatAdsServiceFactory> bat_ads_service_factory,
    brave_ads::ResourceComponent* resource_component,
    history::HistoryService* history_service,
    brave_rewards::RewardsService* rewards_service)
    : profile_(profile),
      local_state_(local_state),
      resource_component_(resource_component),
      history_service_(history_service),
      adaptive_captcha_service_(adaptive_captcha_service),
      ads_tooltips_delegate_(std::move(ads_tooltips_delegate)),
      device_id_(std::move(device_id)),
      bat_ads_service_factory_(std::move(bat_ads_service_factory)),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      base_path_(profile_->GetPath().AppendASCII("ads_service")),
      display_service_(NotificationDisplayService::GetForProfile(profile_)),
      rewards_service_(rewards_service),
      bat_ads_client_associated_receiver_(this) {
  CHECK(profile_);
  CHECK(adaptive_captcha_service_);
  CHECK(device_id_);
  CHECK(bat_ads_service_factory_);
  CHECK(rewards_service_);
  CHECK(profile->IsRegularProfile());

  if (!history_service_ || !local_state_) {
    CHECK_IS_TEST();
  }

  if (CanStartBatAdsService()) {
    bat_ads_client_notifier_pending_receiver_ =
        bat_ads_client_notifier_remote_.BindNewPipeAndPassReceiver();
  }

  Migrate();

  InitializeNotificationsForCurrentProfile();

  GetDeviceIdAndMaybeStartBatAdsService();

  if (resource_component_) {
    resource_component_->AddObserver(this);
  } else {
    CHECK_IS_TEST();
  }

  rewards_service_->AddObserver(this);
}

AdsServiceImpl::~AdsServiceImpl() {
  if (resource_component_) {
    resource_component_->RemoveObserver(this);
  }

  bat_ads_observer_receiver_.reset();

  rewards_service_->RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

bool AdsServiceImpl::IsBatAdsServiceBound() const {
  return bat_ads_service_remote_.is_bound();
}

void AdsServiceImpl::RegisterResourceComponents() const {
  RegisterResourceComponentsForCurrentCountryCode();

  if (UserHasOptedInToNotificationAds()) {
    // Only utilized for text classification, which requires the user to have
    // joined Brave Rewards and opted into notification ads.
    RegisterResourceComponentsForDefaultLanguageCode();
  }
}

void AdsServiceImpl::Migrate() {
  int64_t ads_per_hour =
      profile_->GetPrefs()->GetInt64(prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == 0) {
    profile_->GetPrefs()->ClearPref(prefs::kMaximumNotificationAdsPerHour);
    profile_->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, false);
  }
}

void AdsServiceImpl::RegisterResourceComponentsForCurrentCountryCode() const {
  if (resource_component_) {
    const std::string country_code = brave_l10n::GetCountryCode(local_state_);
    resource_component_->RegisterComponentForCountryCode(country_code);
  }
}

void AdsServiceImpl::RegisterResourceComponentsForDefaultLanguageCode() const {
  if (resource_component_) {
    const std::string& locale = brave_l10n::GetDefaultLocaleString();
    const std::string language_code = brave_l10n::GetISOLanguageCode(locale);
    resource_component_->RegisterComponentForLanguageCode(language_code);
  }
}

bool AdsServiceImpl::UserHasJoinedBraveRewards() const {
  return profile_->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled);
}

bool AdsServiceImpl::UserHasOptedInToBraveNewsAds() const {
  return profile_->GetPrefs()->GetBoolean(
             brave_news::prefs::kBraveNewsOptedIn) &&
         profile_->GetPrefs()->GetBoolean(
             brave_news::prefs::kNewTabPageShowToday);
}

bool AdsServiceImpl::UserHasOptedInToNewTabPageAds() const {
  return profile_->GetPrefs()->GetBoolean(
             ntp_background_images::prefs::kNewTabPageShowBackgroundImage) &&
         profile_->GetPrefs()->GetBoolean(
             ntp_background_images::prefs::
                 kNewTabPageShowSponsoredImagesBackgroundImage);
}

bool AdsServiceImpl::UserHasOptedInToNotificationAds() const {
  return profile_->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled) &&
         profile_->GetPrefs()->GetBoolean(prefs::kOptedInToNotificationAds);
}

bool AdsServiceImpl::UserHasOptedInToSearchResultAds() const {
  return profile_->GetPrefs()->GetBoolean(prefs::kOptedInToSearchResultAds);
}

void AdsServiceImpl::InitializeNotificationsForCurrentProfile() {
  NotificationHelper::GetInstance()->InitForProfile(profile_);

  RecordNotificationAdPositionMetric(ShouldShowCustomNotificationAds(),
                                     profile_->GetPrefs());
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
  return ShouldAlwaysRunService() || UserHasOptedInToBraveNewsAds() ||
         (UserHasJoinedBraveRewards() && (UserHasOptedInToNotificationAds() ||
                                          UserHasOptedInToNewTabPageAds() ||
                                          UserHasOptedInToSearchResultAds()));
}

void AdsServiceImpl::MaybeStartBatAdsService() {
  if (IsBatAdsServiceBound() || !CanStartBatAdsService()) {
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
      bat_ads_client_associated_receiver_.BindNewEndpointAndPassRemote(),
      bat_ads_associated_remote_.BindNewEndpointAndPassReceiver(),
      std::move(bat_ads_client_notifier_pending_receiver_),
      base::BindOnce(&AdsServiceImpl::BatAdsServiceCreatedCallback,
                     weak_ptr_factory_.GetWeakPtr(), ++service_starts_count_));

  bat_ads_associated_remote_.reset_on_disconnect();
  bat_ads_client_notifier_remote_.reset_on_disconnect();

  bat_ads_observer_receiver_.reset();
  AddBatAdsObserver(bat_ads_observer_receiver_.BindNewPipeAndPassRemote());
}

void AdsServiceImpl::DisconnectHandler() {
  VLOG(1) << "Bat Ads Service was disconnected";
  ShutdownAdsService();
}

bool AdsServiceImpl::ShouldProceedInitialization(
    size_t current_start_number) const {
  return IsBatAdsServiceBound() &&
         service_starts_count_ == current_start_number;
}

void AdsServiceImpl::BatAdsServiceCreatedCallback(
    const size_t current_start_number) {
  if (!ShouldProceedInitialization(current_start_number)) {
    return;
  }

  SetSysInfo();

  SetBuildChannel();

  SetFlags();

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&EnsureBaseDirectoryExistsOnFileTaskRunner, base_path_),
      base::BindOnce(&AdsServiceImpl::InitializeBasePathDirectoryCallback,
                     weak_ptr_factory_.GetWeakPtr(), current_start_number));
}

void AdsServiceImpl::InitializeBasePathDirectoryCallback(
    const size_t current_start_number,
    const bool success) {
  if (!success) {
    VLOG(1) << "Failed to initialize " << base_path_ << " directory";
    return ShutdownAdsService();
  }

  Initialize(current_start_number);
}

void AdsServiceImpl::Initialize(const size_t current_start_number) {
  if (!ShouldProceedInitialization(current_start_number)) {
    return;
  }

  InitializeDatabase();

  InitializeRewardsWallet(current_start_number);
}

void AdsServiceImpl::InitializeDatabase() {
  CHECK(!database_);

  database_ = base::SequenceBound<Database>(
      file_task_runner_, base_path_.AppendASCII("database.sqlite"));
}

void AdsServiceImpl::InitializeRewardsWallet(
    const size_t current_start_number) {
  rewards_service_->GetRewardsWallet(
      base::BindOnce(&AdsServiceImpl::InitializeRewardsWalletCallback,
                     weak_ptr_factory_.GetWeakPtr(), current_start_number));
}

void AdsServiceImpl::InitializeRewardsWalletCallback(
    const size_t current_start_number,
    brave_rewards::mojom::RewardsWalletPtr wallet) {
  if (!ShouldProceedInitialization(current_start_number)) {
    return;
  }

  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  if (!wallet && UserHasJoinedBraveRewards()) {
    VLOG(1) << "Failed to initialize Brave Rewards wallet";
    return ShutdownAdsService();
  }

  InitializeBatAds(std::move(wallet));
}

void AdsServiceImpl::InitializeBatAds(
    brave_rewards::mojom::RewardsWalletPtr rewards_wallet) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  mojom::WalletInfoPtr wallet;

  if (rewards_wallet) {
    wallet = mojom::WalletInfo::New();
    wallet->payment_id = rewards_wallet->payment_id;
    wallet->recovery_seed = base::Base64Encode(rewards_wallet->recovery_seed);
  }

  bat_ads_associated_remote_->Initialize(
      std::move(wallet),
      base::BindOnce(&AdsServiceImpl::InitializeBatAdsCallback,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImpl::InitializeBatAdsCallback(const bool success) {
  if (!success) {
    VLOG(1) << "Failed to initialize Bat Ads";
    return ShutdownAdsService();
  }

  is_bat_ads_initialized_ = true;

  RegisterResourceComponents();

  BackgroundHelper::GetInstance()->AddObserver(this);

  MaybeShowOnboardingNotification();

  MaybeOpenNewTabWithAd();

  CheckIdleStateAfterDelay();

  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyDidInitializeAds();
  }
}

void AdsServiceImpl::ShutdownAndResetState() {
  ShutdownAdsService();

  VLOG(6) << "Resetting ads state";

  profile_->GetPrefs()->ClearPrefsWithPrefixSilently("brave.brave_ads");
  local_state_->ClearPref(brave_l10n::prefs::kCountryCode);

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&DeletePathOnFileTaskRunner, base_path_),
      base::BindOnce(&AdsServiceImpl::ShutdownAndResetStateCallback,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImpl::ShutdownAndResetStateCallback(const bool /*success*/) {
  VLOG(6) << "Reset ads state";

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

  mojom::BuildChannelInfoPtr build_channel = mojom::BuildChannelInfo::New();
  build_channel->name = brave::GetChannelName();
  build_channel->is_release = build_channel->name == "release";

  bat_ads_associated_remote_->SetBuildChannel(std::move(build_channel));
}

void AdsServiceImpl::SetFlags() {
  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  mojom::FlagsPtr flags = BuildFlags();
  CHECK(flags);
#if BUILDFLAG(IS_ANDROID)
  if (profile_->GetPrefs()->GetBoolean(
          brave_rewards::prefs::kUseRewardsStagingServer)) {
    flags->environment_type = mojom::EnvironmentType::kStaging;
  }
#endif  // BUILDFLAG(IS_ANDROID)

  bat_ads_associated_remote_->SetFlags(std::move(flags));
}

bool AdsServiceImpl::ShouldShowOnboardingNotification() {
  const bool should_show_onboarding_notification =
      profile_->GetPrefs()->GetBoolean(
          prefs::kShouldShowOnboardingNotification);
  return should_show_onboarding_notification &&
         UserHasOptedInToNotificationAds() && CheckIfCanShowNotificationAds();
}

void AdsServiceImpl::MaybeShowOnboardingNotification() {
  if (!ShouldShowOnboardingNotification()) {
    return;
  }

  if (NotificationHelper::GetInstance()->ShowOnboardingNotification()) {
    SetProfilePref(prefs::kShouldShowOnboardingNotification,
                   base::Value(false));
  }
}

void AdsServiceImpl::ShowReminder(const mojom::ReminderType type) {
#if !BUILDFLAG(IS_ANDROID)
  if (UserHasOptedInToNotificationAds() && CheckIfCanShowNotificationAds()) {
    // TODO(https://github.com/brave/brave-browser/issues/29587): Decouple Brave
    // Ads reminders from notification ads.
    ShowNotificationAd(BuildReminder(type));
  }
#endif
}

void AdsServiceImpl::CloseAdaptiveCaptcha() {
  adaptive_captcha_service_->ClearScheduledCaptcha();
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
      brave_l10n::prefs::kCountryCode,
      base::BindRepeating(&AdsServiceImpl::OnCountryCodePrefChanged,
                          base::Unretained(this),
                          brave_l10n::prefs::kCountryCode));
}

void AdsServiceImpl::InitializePrefChangeRegistrar() {
  pref_change_registrar_.Init(profile_->GetPrefs());

  InitializeBraveRewardsPrefChangeRegistrar();
  InitializeSubdivisionTargetingPrefChangeRegistrar();
  InitializeBraveNewsAdsPrefChangeRegistrar();
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

void AdsServiceImpl::InitializeBraveNewsAdsPrefChangeRegistrar() {
  pref_change_registrar_.Add(
      brave_news::prefs::kBraveNewsOptedIn,
      base::BindRepeating(&AdsServiceImpl::OnOptedInToAdsPrefChanged,
                          base::Unretained(this),
                          brave_news::prefs::kBraveNewsOptedIn));

  pref_change_registrar_.Add(
      brave_news::prefs::kNewTabPageShowToday,
      base::BindRepeating(&AdsServiceImpl::OnOptedInToAdsPrefChanged,
                          base::Unretained(this),
                          brave_news::prefs::kNewTabPageShowToday));
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
  auto notification_ad_position_callback = base::BindRepeating(
      &AdsServiceImpl::OnNotificationAdPositionChanged, base::Unretained(this));
  pref_change_registrar_.Add(prefs::kNotificationAdLastNormalizedCoordinateY,
                             notification_ad_position_callback);
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
  }

  MaybeStartBatAdsService();

  NotifyPrefChanged(path);
}

void AdsServiceImpl::OnCountryCodePrefChanged(const std::string& path) {
  RegisterResourceComponentsForCurrentCountryCode();

  NotifyPrefChanged(path);
}

void AdsServiceImpl::NotifyPrefChanged(const std::string& path) const {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyPrefDidChange(path);
  }
}

void AdsServiceImpl::GetRewardsWallet() {
  rewards_service_->GetRewardsWallet(
      base::BindOnce(&AdsServiceImpl::NotifyRewardsWalletDidUpdate,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImpl::NotifyRewardsWalletDidUpdate(
    brave_rewards::mojom::RewardsWalletPtr wallet) {
  if (wallet && bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyRewardsWalletDidUpdate(
        wallet->payment_id, base::Base64Encode(wallet->recovery_seed));
  }
}

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
                                      const base::TimeDelta idle_time) {
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

  if (!NotificationHelper::GetInstance()->CanShowNotifications()) {
    return ShouldShowCustomNotificationAds();
  }

  return true;
}

bool AdsServiceImpl::ShouldShowCustomNotificationAds() {
  const bool can_show_native_notifications =
      NotificationHelper::GetInstance()->CanShowNotifications();

  const bool can_fallback_to_custom_notification_ads =
      base::FeatureList::IsEnabled(
          kAllowedToFallbackToCustomNotificationAdFeature) &&
      kCanFallbackToCustomNotificationAds.Get();
  if (!can_fallback_to_custom_notification_ads) {
    ClearProfilePref(prefs::kNotificationAdDidFallbackToCustom);
  }

  const bool should_show =
      base::FeatureList::IsEnabled(kCustomNotificationAdFeature);

  const bool should_fallback =
      !can_show_native_notifications && can_fallback_to_custom_notification_ads;
  if (should_fallback) {
    SetProfilePref(prefs::kNotificationAdDidFallbackToCustom,
                   base::Value(true));
  }

  const bool did_fallback = profile_->GetPrefs()->GetBoolean(
      prefs::kNotificationAdDidFallbackToCustom);

  return should_show || should_fallback || did_fallback;
}

void AdsServiceImpl::StartNotificationAdTimeOutTimer(
    const std::string& placement_id) {
#if BUILDFLAG(IS_ANDROID)
  if (!ShouldShowCustomNotificationAds()) {
    return;
  }
#endif

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

  if (!ShouldShowCustomNotificationAds() &&
      NotificationHelper::GetInstance()->DoesSupportSystemNotifications()) {
    bat_ads_associated_remote_->TriggerNotificationAdEvent(
        placement_id, mojom::NotificationAdEventType::kTimedOut,
        /*intentional*/ base::DoNothing());
  }
}

void AdsServiceImpl::CloseAllNotificationAds() {
  // TODO(https://github.com/brave/brave-browser/issues/25410): Temporary
  // solution until we refactor the shutdown business logic and investigate
  // calling `NotificationAdManager` to cleanup notification ads.

#if BUILDFLAG(IS_ANDROID)
  if (!ShouldShowCustomNotificationAds()) {
    return;
  }
#endif

  const auto& list = profile_->GetPrefs()->GetList(prefs::kNotificationAds);

  const base::circular_deque<NotificationAdInfo> ads =
      NotificationAdsFromValue(list);

  for (const auto& ad : ads) {
    CloseNotificationAd(ad.placement_id);
  }

  profile_->GetPrefs()->SetList(prefs::kNotificationAds, {});
}

void AdsServiceImpl::PrefetchNewTabPageAdCallback(
    std::optional<base::Value::Dict> dict) {
  CHECK(!prefetched_new_tab_page_ad_);
  CHECK(is_prefetching_new_tab_page_ad_);

  is_prefetching_new_tab_page_ad_ = false;

  if (dict) {
    prefetched_new_tab_page_ad_ = NewTabPageAdFromValue(*dict);
  }
}

void AdsServiceImpl::MaybeOpenNewTabWithAd() {
  if (retry_opening_new_tab_for_ad_with_placement_id_.empty()) {
    return;
  }

  OpenNewTabWithAd(retry_opening_new_tab_for_ad_with_placement_id_);

  retry_opening_new_tab_for_ad_with_placement_id_ = "";
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
    std::optional<base::Value::Dict> dict) {
  if (!dict) {
    return VLOG(1) << "Failed to get notification ad";
  }

  const NotificationAdInfo notification_ad = NotificationAdFromValue(*dict);

  OpenNewTabWithUrl(notification_ad.target_url);
}

void AdsServiceImpl::OpenNewTabWithUrl(const GURL& url) {
  if (is_shutting_down_) {
    return;
  }

  if (!url.is_valid()) {
    return VLOG(1) << "Failed to open new tab due to invalid URL: " << url;
  }

#if BUILDFLAG(IS_ANDROID)
  // ServiceTabLauncher can currently only launch new tabs
  const content::OpenURLParams params(url, content::Referrer(),
                                      WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                      ui::PAGE_TRANSITION_LINK, true);
  ServiceTabLauncher::GetInstance()->LaunchTab(
      profile_, params, base::BindOnce([](content::WebContents*) {}));
#else
  Browser* browser = chrome::FindTabbedBrowser(profile_, false);
  if (!browser) {
    browser = Browser::Create(Browser::CreateParams(profile_, true));
  }

  NavigateParams nav_params(browser, url, ui::PAGE_TRANSITION_LINK);
  nav_params.disposition = WindowOpenDisposition::SINGLETON_TAB;
  nav_params.window_action = NavigateParams::SHOW_WINDOW;
  nav_params.path_behavior = NavigateParams::RESPECT;
  Navigate(&nav_params);
#endif
}

void AdsServiceImpl::RetryOpeningNewTabWithAd(const std::string& placement_id) {
  VLOG(2) << "Retry opening new tab for ad with placement id " << placement_id;
  retry_opening_new_tab_for_ad_with_placement_id_ = placement_id;
}

void AdsServiceImpl::URLRequestCallback(
    SimpleURLLoaderList::iterator url_loader_iter,
    UrlRequestCallback callback,
    const std::unique_ptr<std::string> response_body) {
  auto url_loader = std::move(*url_loader_iter);
  url_loaders_.erase(url_loader_iter);

  int response_code = -1;

  base::flat_map<std::string, std::string> headers;

  if (!url_loader->ResponseInfo()) {
    VLOG(6) << "ResponseInfo was never received";
  } else if (!url_loader->ResponseInfo()->headers) {
    VLOG(6) << "Failed to obtain headers from the network stack";
  } else {
    const scoped_refptr<net::HttpResponseHeaders> headers_list =
        url_loader->ResponseInfo()->headers;
    response_code = headers_list->response_code();

    size_t iter = 0;
    std::string key;
    std::string value;

    while (headers_list->EnumerateHeaderLines(&iter, &key, &value)) {
      key = base::ToLowerASCII(key);
      headers[key] = value;
    }
  }

  mojom::UrlResponseInfoPtr url_response = mojom::UrlResponseInfo::New();
  url_response->url = url_loader->GetFinalURL();
  url_response->status_code = response_code;
  url_response->body = response_body ? *response_body : "";
  url_response->headers = headers;

  std::move(callback).Run(std::move(url_response));
}

void AdsServiceImpl::ShowScheduledCaptchaCallback(
    const std::string& payment_id,
    const std::string& captcha_id) {
  adaptive_captcha_service_->ShowScheduledCaptcha(payment_id, captcha_id);
}

void AdsServiceImpl::SnoozeScheduledCaptchaCallback() {
  adaptive_captcha_service_->SnoozeScheduledCaptcha();
}

void AdsServiceImpl::OnNotificationAdPositionChanged() {
  RecordNotificationAdPositionMetric(ShouldShowCustomNotificationAds(),
                                     profile_->GetPrefs());
}

void AdsServiceImpl::ShutdownAdsService() {
  if (is_bat_ads_initialized_) {
    SuspendP2AHistograms();

    VLOG(2) << "Shutting down Bat Ads Service";
  }

  bat_ads_client_notifier_remote_.reset();
  bat_ads_client_notifier_pending_receiver_.reset();
  bat_ads_associated_remote_.reset();
  bat_ads_client_associated_receiver_.reset();
  bat_ads_service_remote_.reset();

  url_loaders_.clear();

  idle_state_timer_.Stop();

  notification_ad_timers_.clear();

  prefetched_new_tab_page_ad_.reset();
  is_prefetching_new_tab_page_ad_ = false;

  if (is_bat_ads_initialized_) {
    BackgroundHelper::GetInstance()->RemoveObserver(this);
  }

  CloseAllNotificationAds();

  CloseAdaptiveCaptcha();

  database_.Reset();

  if (is_bat_ads_initialized_) {
    VLOG(2) << "Shutdown Bat Ads Service";
  }

  is_bat_ads_initialized_ = false;
}

void AdsServiceImpl::Shutdown() {
  is_shutting_down_ = true;

  ShutdownAdsService();
}

void AdsServiceImpl::AddBatAdsObserver(
    mojo::PendingRemote<bat_ads::mojom::BatAdsObserver> observer) {
  CHECK(observer.is_valid());

  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->AddBatAdsObserver(std::move(observer));
  }
}

bool AdsServiceImpl::IsBrowserUpgradeRequiredToServeAds() const {
  return browser_upgrade_required_to_serve_ads_;
}

int64_t AdsServiceImpl::GetMaximumNotificationAdsPerHour() const {
  int64_t ads_per_hour =
      profile_->GetPrefs()->GetInt64(prefs::kMaximumNotificationAdsPerHour);
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
                                            const bool by_user) {
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

  bat_ads_associated_remote_->GetStatementOfAccounts(std::move(callback));
}

void AdsServiceImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdAsDictCallback callback) {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(dimensions,
                                   /*inline_content_ad*/ std::nullopt);
  }

  bat_ads_associated_remote_->MaybeServeInlineContentAd(dimensions,
                                                        std::move(callback));
}

void AdsServiceImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->TriggerInlineContentAdEvent(
      placement_id, creative_instance_id, event_type, std::move(callback));
}

void AdsServiceImpl::PrefetchNewTabPageAd() {
  if (!bat_ads_associated_remote_.is_bound()) {
    return;
  }

  if (!prefetched_new_tab_page_ad_ && !is_prefetching_new_tab_page_ad_) {
    is_prefetching_new_tab_page_ad_ = true;

    bat_ads_associated_remote_->MaybeServeNewTabPageAd(
        base::BindOnce(&AdsServiceImpl::PrefetchNewTabPageAdCallback,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

std::optional<NewTabPageAdInfo>
AdsServiceImpl::MaybeGetPrefetchedNewTabPageAdForDisplay() {
  if (!bat_ads_associated_remote_.is_bound()) {
    return std::nullopt;
  }

  std::optional<NewTabPageAdInfo> ad;
  if (prefetched_new_tab_page_ad_ && prefetched_new_tab_page_ad_->IsValid()) {
    ad = prefetched_new_tab_page_ad_;
    prefetched_new_tab_page_ad_.reset();
  }

  return ad;
}

void AdsServiceImpl::OnFailedToPrefetchNewTabPageAd(
    const std::string& /*placement_id*/,
    const std::string& /*creative_instance_id*/) {
  prefetched_new_tab_page_ad_.reset();

  PurgeOrphanedAdEventsForType(mojom::AdType::kNewTabPageAd,
                               /*intentional*/ base::DoNothing());
}

void AdsServiceImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->TriggerNewTabPageAdEvent(
      placement_id, creative_instance_id, event_type, std::move(callback));
}

void AdsServiceImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (!bat_ads_associated_remote_.is_bound()) {
    return std::move(callback).Run(/*success*/ false);
  }

  bat_ads_associated_remote_->TriggerPromotedContentAdEvent(
      placement_id, creative_instance_id, event_type, std::move(callback));
}

void AdsServiceImpl::TriggerSearchResultAdEvent(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    const mojom::SearchResultAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->TriggerSearchResultAdEvent(
        std::move(mojom_creative_ad), event_type, std::move(callback));
  }
}

void AdsServiceImpl::PurgeOrphanedAdEventsForType(
    const mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  CHECK(mojom::IsKnownEnumValue(ad_type));

  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->PurgeOrphanedAdEventsForType(
        ad_type, std::move(callback));
  }
}

void AdsServiceImpl::GetAdHistory(const base::Time from_time,
                                  const base::Time to_time,
                                  GetAdHistoryCallback callback) {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->GetAdHistory(from_time, to_time,
                                             std::move(callback));
  }
}

void AdsServiceImpl::ToggleLikeAd(base::Value::Dict value,
                                  ToggleUserReactionCallback callback) {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->ToggleLikeAd(std::move(value),
                                             std::move(callback));
  }
}

void AdsServiceImpl::ToggleDislikeAd(base::Value::Dict value,
                                     ToggleUserReactionCallback callback) {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->ToggleDislikeAd(std::move(value),
                                                std::move(callback));
  }
}

void AdsServiceImpl::ToggleLikeCategory(base::Value::Dict value,
                                        ToggleUserReactionCallback callback) {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->ToggleLikeCategory(std::move(value),
                                                   std::move(callback));
  }
}

void AdsServiceImpl::ToggleDislikeCategory(
    base::Value::Dict value,
    ToggleUserReactionCallback callback) {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->ToggleDislikeCategory(std::move(value),
                                                      std::move(callback));
  }
}

void AdsServiceImpl::ToggleSaveAd(base::Value::Dict value,
                                  ToggleUserReactionCallback callback) {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->ToggleSaveAd(std::move(value),
                                             std::move(callback));
  }
}

void AdsServiceImpl::ToggleMarkAdAsInappropriate(
    base::Value::Dict value,
    ToggleUserReactionCallback callback) {
  if (bat_ads_associated_remote_.is_bound()) {
    bat_ads_associated_remote_->ToggleMarkAdAsInappropriate(
        std::move(value), std::move(callback));
  }
}

void AdsServiceImpl::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabTextContentDidChange(
        tab_id, redirect_chain, text);
  }
}

void AdsServiceImpl::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabHtmlContentDidChange(
        tab_id, redirect_chain, html);
  }
}

void AdsServiceImpl::NotifyTabDidStartPlayingMedia(const int32_t tab_id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabDidStartPlayingMedia(tab_id);
  }
}

void AdsServiceImpl::NotifyTabDidStopPlayingMedia(const int32_t tab_id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabDidStopPlayingMedia(tab_id);
  }
}

void AdsServiceImpl::NotifyTabDidChange(const int32_t tab_id,
                                        const std::vector<GURL>& redirect_chain,
                                        const bool is_new_navigation,
                                        const bool is_restoring,
                                        const bool is_error_page,
                                        const bool is_visible) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyTabDidChange(
        tab_id, redirect_chain, is_new_navigation, is_restoring, is_error_page,
        is_visible);
  }
}

void AdsServiceImpl::NotifyDidCloseTab(const int32_t tab_id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyDidCloseTab(tab_id);
  }
}

void AdsServiceImpl::NotifyUserGestureEventTriggered(
    const int32_t page_transition_type) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyUserGestureEventTriggered(
        page_transition_type);
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
  std::move(callback).Run(BackgroundHelper::GetInstance()->IsForeground());
}

void AdsServiceImpl::IsBrowserInFullScreenMode(
    IsBrowserInFullScreenModeCallback callback) {
#if !BUILDFLAG(IS_ANDROID)
  std::move(callback).Run(IsFullScreenMode());
#else
  std::move(callback).Run(true);
#endif
}

void AdsServiceImpl::CanShowNotificationAds(
    CanShowNotificationAdsCallback callback) {
  std::move(callback).Run(CheckIfCanShowNotificationAds());
}

void AdsServiceImpl::CanShowNotificationAdsWhileBrowserIsBackgrounded(
    CanShowNotificationAdsWhileBrowserIsBackgroundedCallback callback) {
  std::move(callback).Run(
      NotificationHelper::GetInstance()
          ->CanShowSystemNotificationsWhileBrowserIsBackgrounded());
}

void AdsServiceImpl::ShowNotificationAd(base::Value::Dict dict) {
  const NotificationAdInfo ad = NotificationAdFromValue(dict);

  if (ShouldShowCustomNotificationAds()) {
    std::unique_ptr<NotificationAdPlatformBridge> platform_bridge =
        std::make_unique<NotificationAdPlatformBridge>(*profile_);

    std::u16string title;
    if (base::IsStringUTF8(ad.title)) {
      title = base::UTF8ToUTF16(ad.title);
    }

    std::u16string body;
    if (base::IsStringUTF8(ad.body)) {
      body = base::UTF8ToUTF16(ad.body);
    }

    const NotificationAd notification_ad(ad.placement_id, title, body, nullptr);
    platform_bridge->ShowNotificationAd(notification_ad);
  } else {
    std::u16string title;
    if (base::IsStringUTF8(ad.title)) {
      title = base::UTF8ToUTF16(ad.title);
    }

    std::u16string body;
    if (base::IsStringUTF8(ad.body)) {
      body = base::UTF8ToUTF16(ad.body);
    }

    message_center::RichNotificationData notification_data;
    notification_data.context_message = u" ";

    const GURL url = GURL(kNotificationAdUrlPrefix + ad.placement_id);

    const std::unique_ptr<message_center::Notification> notification =
        std::make_unique<message_center::Notification>(
            message_center::NOTIFICATION_TYPE_SIMPLE, ad.placement_id, title,
            body, ui::ImageModel(), std::u16string(), url,
            message_center::NotifierId(
                message_center::NotifierType::SYSTEM_COMPONENT,
                "service.ads_service"),
            notification_data, nullptr);

#if !BUILDFLAG(IS_MAC) || defined(OFFICIAL_BUILD)
    // `set_never_timeout` uses an XPC service which requires signing so for now
    // we don't set this for macos dev builds
    notification->set_never_timeout(true);
#endif

    display_service_->Display(NotificationHandler::Type::BRAVE_ADS,
                              *notification, /*metadata=*/nullptr);
  }

  StartNotificationAdTimeOutTimer(ad.placement_id);
}

void AdsServiceImpl::CloseNotificationAd(const std::string& placement_id) {
  if (ShouldShowCustomNotificationAds()) {
    std::unique_ptr<NotificationAdPlatformBridge> platform_bridge =
        std::make_unique<NotificationAdPlatformBridge>(*profile_);

    platform_bridge->CloseNotificationAd(placement_id);
  } else {
#if BUILDFLAG(IS_ANDROID)
    const std::string brave_ads_url_prefix = kNotificationAdUrlPrefix;
    const GURL url =
        GURL(brave_ads_url_prefix.substr(0, brave_ads_url_prefix.size() - 1));
    BraveNotificationPlatformBridgeHelperAndroid::MaybeRegenerateNotification(
        placement_id, url);
#endif
    display_service_->Close(NotificationHandler::Type::BRAVE_ADS, placement_id);
  }
}

void AdsServiceImpl::CacheAdEventForInstanceId(
    const std::string& id,
    const std::string& ad_type,
    const std::string& confirmation_type,
    const base::Time time) {
  AdEventCacheHelper::GetInstance()->CacheAdEventForInstanceId(
      id, ad_type, confirmation_type, time);
}

void AdsServiceImpl::GetCachedAdEvents(const std::string& ad_type,
                                       const std::string& confirmation_type,
                                       GetCachedAdEventsCallback callback) {
  std::move(callback).Run(AdEventCacheHelper::GetInstance()->GetCachedAdEvents(
      ad_type, confirmation_type));
}

void AdsServiceImpl::ResetAdEventCacheForInstanceId(const std::string& id) {
  return AdEventCacheHelper::GetInstance()->ResetAdEventCacheForInstanceId(id);
}

void AdsServiceImpl::GetSiteHistory(const int max_count,
                                    const int recent_day_range,
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

            base::ranges::sort(site_history);
            site_history.erase(base::ranges::unique(site_history),
                               site_history.cend());
            std::move(callback).Run(site_history);
          },
          std::move(callback)),
      &history_service_task_tracker_);
}

void AdsServiceImpl::UrlRequest(mojom::UrlRequestInfoPtr url_request,
                                UrlRequestCallback callback) {
  CHECK(url_request);
  CHECK(url_request->url.is_valid());

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url_request->url;
  resource_request->method = URLMethodToRequestType(url_request->method);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  for (const auto& header : url_request->headers) {
    resource_request->headers.AddHeaderFromString(header);
  }

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());

  if (!url_request->content.empty()) {
    url_loader->AttachStringForUpload(url_request->content,
                                      url_request->content_type);
  }

  url_loader->SetOnResponseStartedCallback(
      base::BindOnce(&OnUrlLoaderResponseStartedCallback));

  url_loader->SetRetryOptions(
      kMaximumNumberOfTimesToRetryNetworkRequests,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  url_loader->SetAllowHttpErrorResults(true);

  auto url_loader_iter =
      url_loaders_.insert(url_loaders_.cend(), std::move(url_loader));
  url_loader_iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      profile_->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess()
          .get(),
      base::BindOnce(&AdsServiceImpl::URLRequestCallback,
                     base::Unretained(this), url_loader_iter,
                     std::move(callback)));
}

void AdsServiceImpl::Save(const std::string& name,
                          const std::string& value,
                          SaveCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::ImportantFileWriter::WriteFileAtomically,
                     base_path_.AppendASCII(name), value, std::string_view()),
      std::move(callback));
}

void AdsServiceImpl::Load(const std::string& name, LoadCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(
          [](LoadCallback callback, const std::optional<std::string>& value) {
            std::move(callback).Run(value);
          },
          std::move(callback)));
}

void AdsServiceImpl::LoadResourceComponent(
    const std::string& id,
    const int version,
    LoadResourceComponentCallback callback) {
  if (!resource_component_) {
    return std::move(callback).Run({});
  }

  std::optional<base::FilePath> file_path =
      resource_component_->MaybeGetPath(id, version);
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

void AdsServiceImpl::LoadDataResource(const std::string& name,
                                      LoadDataResourceCallback callback) {
  const int resource_bundle_id = ResourceBundleId(name);

  std::string data_resource;

  const auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(resource_bundle_id)) {
    data_resource = resource_bundle.LoadDataResourceString(resource_bundle_id);
  } else {
    data_resource = static_cast<std::string>(
        resource_bundle.GetRawDataResource(resource_bundle_id));
  }

  std::move(callback).Run(data_resource);
}

void AdsServiceImpl::ShowScheduledCaptcha(const std::string& payment_id,
                                          const std::string& captcha_id) {
#if BUILDFLAG(IS_ANDROID)
  ShowScheduledCaptchaCallback(payment_id, captcha_id);
#else   // BUILDFLAG(IS_ANDROID)
  if (profile_->GetPrefs()->GetBoolean(
          brave_adaptive_captcha::prefs::kScheduledCaptchaPaused)) {
    return VLOG(1) << "Ads paused; support intervention required";
  }

  const int snooze_count = profile_->GetPrefs()->GetInteger(
      brave_adaptive_captcha::prefs::kScheduledCaptchaSnoozeCount);

  CHECK(ads_tooltips_delegate_);

  ads_tooltips_delegate_->ShowCaptchaTooltip(
      payment_id, captcha_id, snooze_count == 0,
      base::BindOnce(&AdsServiceImpl::ShowScheduledCaptchaCallback,
                     weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&AdsServiceImpl::SnoozeScheduledCaptchaCallback,
                     weak_ptr_factory_.GetWeakPtr()));
#endif  // !BUILDFLAG(IS_ANDROID)
}

void AdsServiceImpl::RunDBTransaction(mojom::DBTransactionInfoPtr transaction,
                                      RunDBTransactionCallback callback) {
  CHECK(transaction);
  CHECK(database_);

  database_.AsyncCall(&Database::RunTransaction)
      .WithArgs(std::move(transaction))
      .Then(std::move(callback));
}

void AdsServiceImpl::RecordP2AEvents(const std::vector<std::string>& events) {
  for (const auto& event : events) {
    RecordAndEmitP2AHistogramName(profile_->GetPrefs(),
                                  /*name*/ event);
  }
}

void AdsServiceImpl::GetProfilePref(const std::string& path,
                                    GetProfilePrefCallback callback) {
  std::move(callback).Run(profile_->GetPrefs()->GetValue(path).Clone());
}

void AdsServiceImpl::SetProfilePref(const std::string& path,
                                    base::Value value) {
  profile_->GetPrefs()->Set(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::ClearProfilePref(const std::string& path) {
  profile_->GetPrefs()->ClearPref(path);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::HasProfilePrefPath(const std::string& path,
                                        HasProfilePrefPathCallback callback) {
  std::move(callback).Run(profile_->GetPrefs()->HasPrefPath(path));
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

void AdsServiceImpl::Log(const std::string& file,
                         const int32_t line,
                         const int32_t verbose_level,
                         const std::string& message) {
  rewards_service_->WriteDiagnosticLog(file, line, verbose_level, message);

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

void AdsServiceImpl::OnRemindUser(const brave_ads::mojom::ReminderType type) {
  ShowReminder(type);
}

void AdsServiceImpl::OnBrowserDidEnterForeground() {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyBrowserDidEnterForeground();
#if BUILDFLAG(IS_ANDROID)
    bat_ads_client_notifier_remote_->NotifyBrowserDidBecomeActive();
#endif  // BUILDFLAG(IS_ANDROID)
  }
}

void AdsServiceImpl::OnBrowserDidEnterBackground() {
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

  PrefetchNewTabPageAd();
}

void AdsServiceImpl::OnDidUnregisterResourceComponent(const std::string& id) {
  if (bat_ads_client_notifier_remote_.is_bound()) {
    bat_ads_client_notifier_remote_->NotifyDidUnregisterResourceComponent(id);
  }
}

void AdsServiceImpl::OnRewardsWalletCreated() {
  GetRewardsWallet();
}

void AdsServiceImpl::OnExternalWalletConnected() {
  SetProfilePref(prefs::kShouldMigrateVerifiedRewardsUser, base::Value(true));

  ShowReminder(mojom::ReminderType::kExternalWalletConnected);
}

void AdsServiceImpl::OnCompleteReset(const bool success) {
  if (success) {
    ShutdownAndResetState();
  }
}

}  // namespace brave_ads
