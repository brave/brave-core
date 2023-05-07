/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/circular_deque.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/timer/timer.h"
#include "brave/browser/brave_ads/notification_helper/notification_helper.h"
#include "brave/browser/brave_ads/notifications/notification_ad_platform_bridge.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_ads/browser/ads_p2a.h"
#include "brave/components/brave_ads/browser/ads_storage_cleanup.h"
#include "brave/components/brave_ads/browser/bat_ads_service_factory.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component.h"
#include "brave/components/brave_ads/browser/device_id.h"
#include "brave/components/brave_ads/browser/frequency_capping_helper.h"
#include "brave/components/brave_ads/browser/reminder_util.h"
#include "brave/components/brave_ads/common/constants.h"
#include "brave/components/brave_ads/common/custom_notification_ad_feature.h"
#include "brave/components/brave_ads/common/notification_ad_feature.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_constants.h"
#include "brave/components/brave_ads/core/ads_util.h"
#include "brave/components/brave_ads/core/database.h"
#include "brave/components/brave_ads/core/flags_util.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_value_util.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "brave/components/brave_ads/core/notification_ad_value_util.h"
#include "brave/components/brave_ads/resources/grit/bat_ads_resources.h"
#include "brave/components/brave_federated/data_stores/async_data_store.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/mojom/ledger.mojom.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service.h"
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
#include "brave/components/brave_ads/browser/ads_tooltips_delegate.h"
#include "chrome/browser/first_run/first_run.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
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

constexpr base::TimeDelta kRestartBatAdsServiceDelay = base::Seconds(10);

constexpr int kMaximumNumberOfTimesToRetryNetworkRequests = 1;
constexpr int kHttpUpgradeRequiredStatusResponseCode = 426;

constexpr char kNotificationAdUrlPrefix[] = "https://www.brave.com/ads/?";

int GetDataResourceId(const std::string& name) {
  if (name == data::resource::kCatalogJsonSchemaFilename) {
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

std::string LoadOnFileTaskRunner(const base::FilePath& path) {
  std::string data;
  const bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    return "";
  }

  return data;
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

bool MigrateConfirmationStateOnFileTaskRunner(const base::FilePath& path) {
  const base::FilePath rewards_service_base_path =
      path.AppendASCII("rewards_service");

  const base::FilePath legacy_confirmations_state_path =
      rewards_service_base_path.AppendASCII("confirmations.json");

  if (base::PathExists(legacy_confirmations_state_path)) {
    const base::FilePath ads_service_base_path =
        path.AppendASCII("ads_service");

    if (!base::DirectoryExists(ads_service_base_path)) {
      if (!base::CreateDirectory(ads_service_base_path)) {
        VLOG(1) << "Failed to create " << ads_service_base_path.value();
        return false;
      }

      VLOG(6) << "Created " << ads_service_base_path.value();
    }

    const base::FilePath confirmations_state_path =
        ads_service_base_path.AppendASCII("confirmations.json");

    VLOG(6) << "Migrating " << legacy_confirmations_state_path.value() << " to "
            << confirmations_state_path.value();

    if (!base::Move(legacy_confirmations_state_path,
                    confirmations_state_path)) {
      return false;
    }

    VLOG(6) << "Successfully migrated confirmation state";
  }

  if (base::PathExists(rewards_service_base_path)) {
    VLOG(6) << "Deleting " << rewards_service_base_path.value();

    if (!base::DeleteFile(rewards_service_base_path)) {
      VLOG(1) << "Failed to delete " << rewards_service_base_path.value();
    }
  }

  return true;
}

mojom::DBCommandResponseInfoPtr RunDBTransactionOnFileTaskRunner(
    mojom::DBTransactionInfoPtr transaction,
    Database* database) {
  CHECK(transaction);
  CHECK(database);

  mojom::DBCommandResponseInfoPtr command_response =
      mojom::DBCommandResponseInfo::New();

  if (!database) {
    command_response->status =
        mojom::DBCommandResponseInfo::StatusType::RESPONSE_ERROR;
  } else {
    database->RunTransaction(std::move(transaction), command_response.get());
  }

  return command_response;
}

void RegisterResourceComponentsForLocale(const std::string& locale) {
  g_brave_browser_process->resource_component()->RegisterComponentsForLocale(
      locale);
}

void RegisterResourceComponentsForDefaultLocale() {
  RegisterResourceComponentsForLocale(brave_l10n::GetDefaultLocaleString());
}

void OnURLResponseStarted(
    const GURL& /*final_url*/,
    const network::mojom::URLResponseHead& response_head) {
  if (response_head.headers->response_code() == -1) {
    VLOG(6) << "Response headers are malformed!!";
  }
}

void OnResetState(const bool success) {
  if (!success) {
    VLOG(1) << "Failed to reset ads state";
    return;
  }

  VLOG(6) << "Successfully reset ads state";
}

void OnAddTrainingSample(const bool success) {
  if (!success) {
    VLOG(6) << "Failed to add training sample";
    return;
  }

  VLOG(6) << "Successfully added training sample";
}

}  // namespace

AdsServiceImpl::AdsServiceImpl(
    Profile* profile,
    brave_adaptive_captcha::BraveAdaptiveCaptchaService*
        adaptive_captcha_service,
    std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate,
    std::unique_ptr<DeviceId> device_id,
    std::unique_ptr<BatAdsServiceFactory> bat_ads_service_factory,
    history::HistoryService* history_service,
    brave_rewards::RewardsService* rewards_service,
    brave_federated::AsyncDataStore* notification_ad_timing_data_store)
    : profile_(profile),
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
      notification_ad_timing_data_store_(notification_ad_timing_data_store),
      bat_ads_client_(this) {
  CHECK(profile_);
  CHECK(adaptive_captcha_service_);
  CHECK(device_id_);
  CHECK(history_service_);
  CHECK(rewards_service_);
  CHECK(brave::IsRegularProfile(profile_));

  InitializeNotificationsForCurrentProfile();

  MigratePrefs();

  DisableAdsIfUnsupportedRegion();

  MigrateConfirmationState();

  rewards_service_->AddObserver(this);
}

AdsServiceImpl::~AdsServiceImpl() {
  rewards_service_->RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceImpl::InitializeNotificationsForCurrentProfile() const {
  NotificationHelper::GetInstance()->InitForProfile(profile_);
}

void AdsServiceImpl::MigrateConfirmationState() {
  const base::FilePath path = profile_->GetPath();

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&MigrateConfirmationStateOnFileTaskRunner, path),
      base::BindOnce(&AdsServiceImpl::OnMigrateConfirmationState, AsWeakPtr()));
}

void AdsServiceImpl::OnMigrateConfirmationState(const bool success) {
  if (!success) {
    VLOG(1) << "Failed to migrate confirmation state";
    return;
  }

  GetDeviceId();
}

void AdsServiceImpl::GetDeviceId() {
  device_id_->GetDeviceId(
      base::BindOnce(&AdsServiceImpl::OnGetDeviceId, AsWeakPtr()));
}

void AdsServiceImpl::OnGetDeviceId(std::string device_id) {
  sys_info_.device_id = std::move(device_id);

  InitializePrefChangeRegistrar();

  MaybeStartBatAdsService();
}

bool AdsServiceImpl::UserHasOptedIn() const {
  return base::FeatureList::IsEnabled(
             brave_news::features::kBraveNewsFeature) &&
         GetPrefService()->GetBoolean(brave_news::prefs::kBraveNewsOptedIn) &&
         GetPrefService()->GetBoolean(brave_news::prefs::kNewTabPageShowToday);
}

bool AdsServiceImpl::CanStartBatAdsService() const {
  return IsEnabled() || UserHasOptedIn();
}

void AdsServiceImpl::MaybeStartBatAdsService() {
  CancelRestartBatAdsService();

  if (bat_ads_service_.is_bound() || !CanStartBatAdsService()) {
    return;
  }

  if (!IsSupportedRegion()) {
    VLOG(6) << brave_l10n::GetDefaultISOCountryCodeString()
            << " region does not support ads";
    return;
  }

  StartBatAdsService();
}

void AdsServiceImpl::StartBatAdsService() {
  CHECK(!bat_ads_service_.is_bound());

  bat_ads_service_ = bat_ads_service_factory_->Launch();

  bat_ads_service_.set_disconnect_handler(base::BindOnce(
      &AdsServiceImpl::RestartBatAdsServiceAfterDelay, AsWeakPtr()));

  CHECK(bat_ads_service_.is_bound());

  bat_ads_service_->Create(
      bat_ads_client_.BindNewEndpointAndPassRemote(),
      bat_ads_.BindNewEndpointAndPassReceiver(),
      bat_ads_client_notifier_.BindNewPipeAndPassReceiver(),
      base::BindOnce(&AdsServiceImpl::OnBatAdsServiceCreated, AsWeakPtr()));
}

void AdsServiceImpl::RestartBatAdsServiceAfterDelay() {
  VLOG(6) << "Restart bat-ads service";

  Shutdown();

  VLOG(6) << "Restarting bat-ads service in " << kRestartBatAdsServiceDelay;
  restart_bat_ads_service_timer_.Start(
      FROM_HERE, kRestartBatAdsServiceDelay,
      base::BindOnce(&AdsServiceImpl::MaybeStartBatAdsService, AsWeakPtr()));
}

void AdsServiceImpl::CancelRestartBatAdsService() {
  if (restart_bat_ads_service_timer_.IsRunning()) {
    VLOG(6) << "Canceled bat-ads service restart";
    restart_bat_ads_service_timer_.Stop();
  }
}

void AdsServiceImpl::OnBatAdsServiceCreated() {
  SetSysInfo();

  SetBuildChannel();

  SetFlags();

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&EnsureBaseDirectoryExistsOnFileTaskRunner, base_path_),
      base::BindOnce(&AdsServiceImpl::OnInitializeBasePathDirectory,
                     AsWeakPtr()));
}

void AdsServiceImpl::OnInitializeBasePathDirectory(const bool success) {
  if (!success) {
    VLOG(1) << "Failed to initialize " << base_path_ << " directory";
    return Shutdown();
  }

  Initialize();
}

void AdsServiceImpl::Initialize() {
  InitializeDatabase();

  BackgroundHelper::GetInstance()->AddObserver(this);

  g_brave_browser_process->resource_component()->AddObserver(this);

  RegisterResourceComponentsForDefaultLocale();

  InitializeRewardsWallet();
}

void AdsServiceImpl::InitializeDatabase() {
  CHECK(!database_);

  database_ =
      std::make_unique<Database>(base_path_.AppendASCII("database.sqlite"));
}

bool AdsServiceImpl::ShouldRewardUser() const {
  return IsEnabled();
}

void AdsServiceImpl::InitializeRewardsWallet() {
  rewards_service_->GetRewardsWallet(
      base::BindOnce(&AdsServiceImpl::OnInitializeRewardsWallet, AsWeakPtr()));
}

void AdsServiceImpl::OnInitializeRewardsWallet(
    brave_rewards::mojom::RewardsWalletPtr wallet) {
  if (!bat_ads_.is_bound()) {
    return;
  }

  if (wallet) {
    bat_ads_->OnRewardsWalletDidChange(
        wallet->payment_id, base::Base64Encode(wallet->recovery_seed));
  } else if (ShouldRewardUser()) {
    VLOG(1) << "Failed to initialize Rewards wallet";
    return Shutdown();
  }

  InitializeBatAds();
}

void AdsServiceImpl::InitializeBatAds() {
  if (bat_ads_.is_bound()) {
    bat_ads_->Initialize(
        base::BindOnce(&AdsServiceImpl::OnInitializeBatAds, AsWeakPtr()));
  }
}

void AdsServiceImpl::OnInitializeBatAds(const bool success) {
  if (!success) {
    VLOG(1) << "Failed to initialize bat-ads";
    return Shutdown();
  }

  is_bat_ads_initialized_ = true;

  CleanUpOnFirstRun();

  MaybeShowOnboardingNotification();

  MaybeOpenNewTabWithAd();

  CheckIdleStateAfterDelay();
}

void AdsServiceImpl::ShutdownAndResetState() {
  Shutdown();

  VLOG(6) << "Resetting ads state";

  GetPrefService()->ClearPrefsWithPrefixSilently("brave.brave_ads");

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&DeletePathOnFileTaskRunner, base_path_),
      base::BindOnce(&OnResetState));
}

void AdsServiceImpl::SetSysInfo() {
  if (bat_ads_.is_bound()) {
    bat_ads_->SetSysInfo(sys_info_.Clone());
  }
}

void AdsServiceImpl::SetBuildChannel() {
  if (!bat_ads_.is_bound()) {
    return;
  }

  mojom::BuildChannelInfoPtr build_channel = mojom::BuildChannelInfo::New();
  build_channel->name = brave::GetChannelName();
  build_channel->is_release = build_channel->name == "release";

  bat_ads_->SetBuildChannel(std::move(build_channel));
}

void AdsServiceImpl::SetFlags() {
  if (!bat_ads_.is_bound()) {
    return;
  }

  mojom::FlagsPtr flags = BuildFlags();
  CHECK(flags);
#if BUILDFLAG(IS_ANDROID)
  if (GetPrefService()->GetBoolean(
          brave_rewards::prefs::kUseRewardsStagingServer)) {
    flags->environment_type = mojom::EnvironmentType::kStaging;
  }
#endif  // BUILDFLAG(IS_ANDROID)

  bat_ads_->SetFlags(std::move(flags));
}

void AdsServiceImpl::CleanUpOnFirstRun() {
  if (did_cleanup_on_first_run_) {
    return;
  }

  did_cleanup_on_first_run_ = true;

  RemoveDeprecatedFiles();
}

void AdsServiceImpl::RemoveDeprecatedFiles() const {
  file_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&RemoveDeprecatedAdsDataFiles, base_path_));
}

bool AdsServiceImpl::ShouldShowOnboardingNotification() {
  const bool should_show_my_first_notification_ad =
      GetPrefService()->GetBoolean(prefs::kShouldShowOnboardingNotification);
  return IsEnabled() && CheckIfCanShowNotificationAds() &&
         should_show_my_first_notification_ad;
}

void AdsServiceImpl::MaybeShowOnboardingNotification() {
  if (!ShouldShowOnboardingNotification()) {
    return;
  }

  if (!NotificationHelper::GetInstance()->ShowOnboardingNotification()) {
    return;
  }

  SetBooleanPref(prefs::kShouldShowOnboardingNotification, false);
}

void AdsServiceImpl::CloseAdaptiveCaptcha() {
  adaptive_captcha_service_->ClearScheduledCaptcha();
#if !BUILDFLAG(IS_ANDROID)
  ads_tooltips_delegate_->CloseCaptchaTooltip();
#endif  // !BUILDFLAG(IS_ANDROID)
}

void AdsServiceImpl::InitializePrefChangeRegistrar() {
  pref_change_registrar_.Init(profile_->GetPrefs());

  pref_change_registrar_.Add(
      prefs::kEnabled,
      base::BindRepeating(&AdsServiceImpl::OnEnabledPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      prefs::kIdleTimeThreshold,
      base::BindRepeating(&AdsServiceImpl::OnIdleTimeThresholdPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_news::prefs::kBraveNewsOptedIn,
      base::BindRepeating(&AdsServiceImpl::OnBraveNewsOptedInPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_news::prefs::kNewTabPageShowToday,
      base::BindRepeating(&AdsServiceImpl::OnNewTabPageShowTodayPrefChanged,
                          base::Unretained(this)));
}

void AdsServiceImpl::OnEnabledPrefChanged() {
  if (!CanStartBatAdsService()) {
    return Shutdown();
  }

  MaybeStartBatAdsService();
}

void AdsServiceImpl::OnIdleTimeThresholdPrefChanged() {
  CheckIdleStateAfterDelay();
}

void AdsServiceImpl::OnBraveNewsOptedInPrefChanged() {
  if (!CanStartBatAdsService()) {
    return Shutdown();
  }

  MaybeStartBatAdsService();
}

void AdsServiceImpl::OnNewTabPageShowTodayPrefChanged() {
  if (!CanStartBatAdsService()) {
    return Shutdown();
  }

  MaybeStartBatAdsService();
}

void AdsServiceImpl::NotifyPrefChanged(const std::string& path) const {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyPrefDidChange(path);
  }
}

void AdsServiceImpl::GetRewardsWallet() {
  rewards_service_->GetRewardsWallet(
      base::BindOnce(&AdsServiceImpl::OnGetRewardsWallet, AsWeakPtr()));
}

void AdsServiceImpl::OnGetRewardsWallet(
    brave_rewards::mojom::RewardsWalletPtr wallet) {
  if (wallet && bat_ads_.is_bound()) {
    bat_ads_->OnRewardsWalletDidChange(
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
  const int idle_threshold =
      GetPrefService()->GetInteger(prefs::kIdleTimeThreshold);
  const ui::IdleState idle_state = ui::CalculateIdleState(idle_threshold);
  ProcessIdleState(idle_state, last_idle_time_);

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
      if (bat_ads_client_notifier_.is_bound()) {
        bat_ads_client_notifier_->NotifyUserDidBecomeActive(idle_time,
                                                            screen_was_locked);
      }

      break;
    }

    case ui::IdleState::IDLE_STATE_IDLE:
    case ui::IdleState::IDLE_STATE_LOCKED: {
      if (bat_ads_client_notifier_.is_bound()) {
        bat_ads_client_notifier_->NotifyUserDidBecomeIdle();
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
  if (!IsNotificationAdFeatureEnabled()) {
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

  bool can_fallback_to_custom_notification_ads =
      kCanFallbackToCustomNotificationAds.Get();
  if (!can_fallback_to_custom_notification_ads) {
    ClearPref(prefs::kNotificationAdDidFallbackToCustom);
  } else {
    const bool allowed_to_fallback_to_custom_notification_ads =
        IsAllowedToFallbackToCustomNotificationAdFeatureEnabled();
    if (!allowed_to_fallback_to_custom_notification_ads) {
      can_fallback_to_custom_notification_ads = false;
    }
  }

  const bool should_show = IsCustomNotificationAdFeatureEnabled();

  const bool should_fallback =
      !can_show_native_notifications && can_fallback_to_custom_notification_ads;
  if (should_fallback) {
    SetBooleanPref(prefs::kNotificationAdDidFallbackToCustom, true);
  }

  const bool did_fallback =
      GetPrefService()->GetBoolean(prefs::kNotificationAdDidFallbackToCustom);

  return should_show || should_fallback || did_fallback;
}

void AdsServiceImpl::StartNotificationAdTimeOutTimer(
    const std::string& placement_id) {
#if BUILDFLAG(IS_ANDROID)
  if (!ShouldShowCustomNotificationAds()) {
    return;
  }
#endif

  const int timeout_in_seconds = kNotificationAdTimeout.Get();
  if (timeout_in_seconds == 0) {
    // Never time out
    return;
  }

  const base::TimeDelta timeout = base::Seconds(timeout_in_seconds);

  notification_ad_timers_[placement_id] =
      std::make_unique<base::OneShotTimer>();
  notification_ad_timers_[placement_id]->Start(
      FROM_HERE, timeout,
      base::BindOnce(&AdsServiceImpl::NotificationAdTimedOut, AsWeakPtr(),
                     placement_id));

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

  if (!bat_ads_.is_bound()) {
    return;
  }

  if (!ShouldShowCustomNotificationAds() &&
      NotificationHelper::GetInstance()->DoesSupportSystemNotifications()) {
    bat_ads_->TriggerNotificationAdEvent(
        placement_id, mojom::NotificationAdEventType::kTimedOut);
  }
}

void AdsServiceImpl::CloseAllNotificationAds() {
  // TODO(https://github.com/brave/brave-browser/issues/25410): Temporary
  // solution until we refactor the shutdown business logic and investigate
  // calling |NotificationAdManager| to cleanup notification ads.

#if BUILDFLAG(IS_ANDROID)
  if (!ShouldShowCustomNotificationAds()) {
    return;
  }
#endif

  const auto& list = GetPrefService()->GetList(prefs::kNotificationAds);

  const base::circular_deque<NotificationAdInfo> ads =
      NotificationAdsFromValue(list);

  for (const auto& ad : ads) {
    CloseNotificationAd(ad.placement_id);
  }

  GetPrefService()->SetList(prefs::kNotificationAds, {});
}

void AdsServiceImpl::OnPrefetchNewTabPageAd(
    absl::optional<base::Value::Dict> dict) {
  CHECK(!prefetched_new_tab_page_ad_);
  CHECK(is_prefetching_new_tab_page_ad_);

  is_prefetching_new_tab_page_ad_ = false;

  if (!dict) {
    VLOG(1) << "Failed to prefetch new tab page ad";
    return;
  }
  prefetched_new_tab_page_ad_ = NewTabPageAdFromValue(*dict);
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

  if (!is_bat_ads_initialized_) {
    return RetryOpeningNewTabWithAd(placement_id);
  }

  bat_ads_->MaybeGetNotificationAd(
      placement_id,
      base::BindOnce(&AdsServiceImpl::OnOpenNewTabWithAd, AsWeakPtr()));
}

void AdsServiceImpl::OnOpenNewTabWithAd(
    absl::optional<base::Value::Dict> dict) {
  if (!dict) {
    VLOG(1) << "Failed to get notification ad";
    return;
  }

  const NotificationAdInfo notification = NotificationAdFromValue(*dict);

  OpenNewTabWithUrl(notification.target_url);
}

void AdsServiceImpl::OpenNewTabWithUrl(const GURL& url) {
  if (g_browser_process->IsShuttingDown()) {
    return;
  }

  if (!url.is_valid()) {
    VLOG(1) << "Failed to open new tab due to invalid URL: " << url;
    return;
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
  nav_params.path_behavior = NavigateParams::IGNORE_AND_NAVIGATE;
  Navigate(&nav_params);
#endif
}

void AdsServiceImpl::RetryOpeningNewTabWithAd(const std::string& placement_id) {
  VLOG(2) << "Retry opening new tab for ad with placement id " << placement_id;
  retry_opening_new_tab_for_ad_with_placement_id_ = placement_id;
}

void AdsServiceImpl::OnURLRequest(
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

  if (response_code == kHttpUpgradeRequiredStatusResponseCode &&
      !needs_browser_upgrade_to_serve_ads_) {
    needs_browser_upgrade_to_serve_ads_ = true;
    for (AdsServiceObserver& observer : observers_) {
      observer.OnNeedsBrowserUpgradeToServeAds();
    }
  }

  mojom::UrlResponseInfoPtr url_response = mojom::UrlResponseInfo::New();
  url_response->url = url_loader->GetFinalURL();
  url_response->status_code = response_code;
  url_response->body = response_body ? *response_body : "";
  url_response->headers = headers;

  std::move(callback).Run(std::move(url_response));
}

PrefService* AdsServiceImpl::GetPrefService() {
  return profile_->GetPrefs();
}

const PrefService* AdsServiceImpl::GetPrefService() const {
  return profile_->GetPrefs();
}

bool AdsServiceImpl::IsUpgradingFromPreBraveAdsBuild() {
  // Brave ads was hidden in 0.62.x however due to a bug |prefs::kEnabled| was
  // set to true causing "https://github.com/brave/brave-browser/issues/5434"

  // |prefs::kIdleTimeThreshold| was not serialized in 0.62.x

  // |prefs::kVersion| was introduced in 0.63.x

  // We can detect if we are upgrading from a pre Brave ads build by checking
  // |prefs::kEnabled| is set to true, |prefs::kIdleTimeThreshold| does not
  // exist, |prefs::kVersion| does not exist and it is not the first time the
  // browser has run for this user
#if !BUILDFLAG(IS_ANDROID)
  return GetPrefService()->GetBoolean(prefs::kEnabled) &&
         !GetPrefService()->HasPrefPath(prefs::kIdleTimeThreshold) &&
         !GetPrefService()->HasPrefPath(prefs::kVersion) &&
         !first_run::IsChromeFirstRun();
#else
  return false;
#endif
}

void AdsServiceImpl::MigratePrefs() {
  is_upgrading_from_pre_brave_ads_build_ = IsUpgradingFromPreBraveAdsBuild();
  if (is_upgrading_from_pre_brave_ads_build_) {
    VLOG(2) << "Migrating ads preferences from pre Brave Ads build";

    // Force migration of preferences from version 1 if
    // |is_upgrading_from_pre_brave_ads_build_| is set to true to fix
    // "https://github.com/brave/brave-browser/issues/5434"
    SetIntegerPref(prefs::kVersion, 1);
  } else {
    VLOG(2) << "Migrating ads preferences";
  }

  const int source_version = GetPrefService()->GetInteger(prefs::kVersion);
  const int dest_version = kCurrentPrefVersion;

  if (!MigratePrefs(source_version, dest_version, true)) {
    // Migration dry-run failed, so do not migrate preferences
    VLOG(1) << "Failed to migrate ads preferences from version "
            << source_version << " to " << dest_version;

    return;
  }

  MigratePrefs(source_version, dest_version);
}

bool AdsServiceImpl::MigratePrefs(const int source_version,
                                  const int dest_version,
                                  const bool is_dry_run) {
  CHECK(source_version >= 1) << "Invalid migration path";
  CHECK(source_version <= dest_version) << "Invalid migration path";

  if (source_version == dest_version) {
    SetIntegerPref(prefs::kVersion, dest_version);

    if (!is_dry_run) {
      VLOG(2) << "Ads preferences are up to date on version " << dest_version;
    }

    return true;
  }

  // Migration paths should be added to the below map, i.e.
  //
  //   {{1, 2}, &AdsServiceImpl::MigratePrefsVersion1To2},
  //   {{2, 3}, &AdsServiceImpl::MigratePrefsVersion2To3},
  //   {{3, 4}, &AdsServiceImpl::MigratePrefsVersion3To4}

  static const base::NoDestructor<
      base::flat_map<std::pair<int, int>, void (AdsServiceImpl::*)()>>
      kMappings({// {{from version, to version}, function}
                 {{1, 2}, &AdsServiceImpl::MigratePrefsVersion1To2},
                 {{2, 3}, &AdsServiceImpl::MigratePrefsVersion2To3},
                 {{3, 4}, &AdsServiceImpl::MigratePrefsVersion3To4},
                 {{4, 5}, &AdsServiceImpl::MigratePrefsVersion4To5},
                 {{5, 6}, &AdsServiceImpl::MigratePrefsVersion5To6},
                 {{6, 7}, &AdsServiceImpl::MigratePrefsVersion6To7},
                 {{7, 8}, &AdsServiceImpl::MigratePrefsVersion7To8},
                 {{8, 9}, &AdsServiceImpl::MigratePrefsVersion8To9},
                 {{9, 10}, &AdsServiceImpl::MigratePrefsVersion9To10},
                 {{10, 11}, &AdsServiceImpl::MigratePrefsVersion10To11},
                 {{11, 12}, &AdsServiceImpl::MigratePrefsVersion11To12}});

  // Cycle through migration paths, i.e. if upgrading from version 2 to 5 we
  // should migrate version 2 to 3, then 3 to 4 and finally version 4 to 5

  int from_version = source_version;
  int to_version = from_version + 1;

  do {
    const auto mapping =
        kMappings->find(std::make_pair(from_version, to_version));
    if (mapping == kMappings->cend()) {
      // Migration path does not exist. It is highly recommended to perform a
      // dry-run before migrating preferences
      return false;
    }

    if (!is_dry_run) {
      VLOG(2) << "Migrating ads preferences from mapping version "
              << from_version << " to " << to_version;

      (this->*(mapping->second))();
    }

    from_version++;
    if (to_version < dest_version) {
      to_version++;
    }
  } while (from_version != to_version);

  if (!is_dry_run) {
    SetIntegerPref(prefs::kVersion, dest_version);

    VLOG(2) << "Successfully migrated Ads preferences from version "
            << source_version << " to " << dest_version;
  }

  return true;
}

void AdsServiceImpl::DisableAdsIfUpgradingFromPreBraveAdsBuild() {
  if (!is_upgrading_from_pre_brave_ads_build_) {
    return;
  }

  SetEnabled(false);
}

void AdsServiceImpl::DisableAdsIfUnsupportedRegion() {
  if (!IsSupportedRegion() && IsEnabled()) {
    SetEnabled(false);
  }
}

void AdsServiceImpl::MigratePrefsVersion1To2() {
  // Intentionally empty as migration is no longer required
}

void AdsServiceImpl::MigratePrefsVersion2To3() {
  // Disable ads if upgrading from a pre brave ads build due to a bug where ads
  // were always enabled
  DisableAdsIfUpgradingFromPreBraveAdsBuild();
}

void AdsServiceImpl::MigratePrefsVersion3To4() {
  // Intentionally empty as migration is no longer required
}

void AdsServiceImpl::MigratePrefsVersion4To5() {}

void AdsServiceImpl::MigratePrefsVersion5To6() {
  // Intentionally empty as migration is no longer required
}

void AdsServiceImpl::MigratePrefsVersion6To7() {
  // Intentionally empty as migration is no longer required
}

void AdsServiceImpl::MigratePrefsVersion7To8() {
  const bool rewards_enabled =
      GetPrefService()->GetBoolean(brave_rewards::prefs::kEnabled);
  if (!rewards_enabled) {
    SetEnabled(false);
  }
}

void AdsServiceImpl::MigratePrefsVersion8To9() {
  // Intentionally empty as migration is no longer required
}

void AdsServiceImpl::MigratePrefsVersion9To10() {
  if (!GetPrefService()->HasPrefPath(prefs::kMaximumNotificationAdsPerHour)) {
    return;
  }

  const int64_t ads_per_hour =
      GetPrefService()->GetInt64(prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == -1 || ads_per_hour == 2) {
    // The user did not change the ads per hour setting from the legacy default
    // value of 2 so we should clear the preference to transition to
    // |kDefaultBraveRewardsNotificationAdsPerHour|
    GetPrefService()->ClearPref(prefs::kMaximumNotificationAdsPerHour);
  }
}

void AdsServiceImpl::MigratePrefsVersion10To11() {
  if (!GetPrefService()->HasPrefPath(prefs::kMaximumNotificationAdsPerHour)) {
    return;
  }

  const int64_t ads_per_hour =
      GetPrefService()->GetInt64(prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == 0 || ads_per_hour == -1) {
    // Clear the ads per hour preference to transition to
    // |kDefaultBraveRewardsNotificationAdsPerHour|
    GetPrefService()->ClearPref(prefs::kMaximumNotificationAdsPerHour);
  }
}

void AdsServiceImpl::MigratePrefsVersion11To12() {
  std::string value;

  if (base::ReadFileToString(base_path_.AppendASCII("confirmations.json"),
                             &value)) {
    const auto hash = static_cast<uint64_t>(base::PersistentHash(value));
    SetUint64Pref(prefs::kConfirmationsHash, hash);
  }

  if (base::ReadFileToString(base_path_.AppendASCII("client.json"), &value)) {
    const auto hash = static_cast<uint64_t>(base::PersistentHash(value));
    SetUint64Pref(prefs::kClientHash, hash);
  }
}

void AdsServiceImpl::Shutdown() {
  SuspendP2AHistograms();

  CancelRestartBatAdsService();

  VLOG(2) << "Shutting down bat-ads service";

  is_bat_ads_initialized_ = false;

  bat_ads_client_notifier_.reset();
  bat_ads_.reset();
  bat_ads_client_.reset();
  bat_ads_service_.reset();

  BackgroundHelper::GetInstance()->RemoveObserver(this);

  g_brave_browser_process->resource_component()->RemoveObserver(this);

  url_loaders_.clear();

  idle_state_timer_.Stop();

  notification_ad_timers_.clear();

  prefetched_new_tab_page_ad_.reset();
  is_prefetching_new_tab_page_ad_ = false;

  CloseAllNotificationAds();

  CloseAdaptiveCaptcha();

  if (database_) {
    const bool success =
        file_task_runner_->DeleteSoon(FROM_HERE, database_.release());
    CHECK(success) << "Failed to release database";
  }

  VLOG(2) << "Shutdown bat-ads service";
}

bool AdsServiceImpl::IsEnabled() const {
  return GetPrefService()->GetBoolean(prefs::kEnabled);
}

void AdsServiceImpl::SetEnabled(const bool is_enabled) {
  SetBooleanPref(prefs::kEnabled, is_enabled);
}

int64_t AdsServiceImpl::GetMaximumNotificationAdsPerHour() const {
  int64_t ads_per_hour =
      GetPrefService()->GetInt64(prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == -1) {
    ads_per_hour = kDefaultNotificationAdsPerHour.Get();
  }

  return ads_per_hour;
}

void AdsServiceImpl::SetMaximumNotificationAdsPerHour(
    const int64_t ads_per_hour) {
  SetInt64Pref(prefs::kMaximumNotificationAdsPerHour, ads_per_hour);
}

bool AdsServiceImpl::ShouldAllowSubdivisionTargeting() const {
  return GetPrefService()->GetBoolean(prefs::kShouldAllowSubdivisionTargeting);
}

std::string AdsServiceImpl::GetSubdivisionTargetingCode() const {
  return GetPrefService()->GetString(prefs::kSubdivisionTargetingCode);
}

void AdsServiceImpl::SetSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  SetStringPref(prefs::kSubdivisionTargetingCode, subdivision_targeting_code);
}

std::string AdsServiceImpl::GetAutoDetectedSubdivisionTargetingCode() const {
  return GetPrefService()->GetString(
      prefs::kAutoDetectedSubdivisionTargetingCode);
}

void AdsServiceImpl::SetAutoDetectedSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  SetStringPref(prefs::kAutoDetectedSubdivisionTargetingCode,
                subdivision_targeting_code);
}

bool AdsServiceImpl::NeedsBrowserUpgradeToServeAds() const {
  return needs_browser_upgrade_to_serve_ads_;
}

void AdsServiceImpl::ShowScheduledCaptcha(const std::string& payment_id,
                                          const std::string& captcha_id) {
  adaptive_captcha_service_->ShowScheduledCaptcha(payment_id, captcha_id);
}

void AdsServiceImpl::SnoozeScheduledCaptcha() {
  adaptive_captcha_service_->SnoozeScheduledCaptcha();
}

void AdsServiceImpl::OnNotificationAdShown(const std::string& placement_id) {
  if (bat_ads_.is_bound()) {
    bat_ads_->TriggerNotificationAdEvent(
        placement_id, mojom::NotificationAdEventType::kViewed);
  }
}

void AdsServiceImpl::OnNotificationAdClosed(const std::string& placement_id,
                                            const bool by_user) {
  if (StopNotificationAdTimeOutTimer(placement_id)) {
    VLOG(2) << "Canceled timeout for notification ad with placement id "
            << placement_id;
  }

  if (!bat_ads_.is_bound()) {
    return;
  }

  const mojom::NotificationAdEventType event_type =
      by_user ? mojom::NotificationAdEventType::kDismissed
              : mojom::NotificationAdEventType::kTimedOut;

  bat_ads_->TriggerNotificationAdEvent(placement_id, event_type);
}

void AdsServiceImpl::OnNotificationAdClicked(const std::string& placement_id) {
  if (!bat_ads_.is_bound()) {
    return;
  }

  OpenNewTabWithAd(placement_id);

  bat_ads_->TriggerNotificationAdEvent(
      placement_id, mojom::NotificationAdEventType::kClicked);
}

void AdsServiceImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  if (!bat_ads_.is_bound()) {
    return std::move(callback).Run(/*diagnostics*/ absl::nullopt);
  }

  bat_ads_->GetDiagnostics(std::move(callback));
}

void AdsServiceImpl::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  if (!bat_ads_.is_bound()) {
    return std::move(callback).Run(/*statement*/ nullptr);
  }

  bat_ads_->GetStatementOfAccounts(std::move(callback));
}

void AdsServiceImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdAsDictCallback callback) {
  if (!bat_ads_.is_bound()) {
    return std::move(callback).Run(dimensions,
                                   /*inline_content_ad*/ absl::nullopt);
  }

  bat_ads_->MaybeServeInlineContentAd(dimensions, std::move(callback));
}

void AdsServiceImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (bat_ads_.is_bound()) {
    bat_ads_->TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                          event_type);
  }
}

void AdsServiceImpl::PrefetchNewTabPageAd() {
  if (!bat_ads_.is_bound()) {
    return;
  }

  if (!prefetched_new_tab_page_ad_ && !is_prefetching_new_tab_page_ad_) {
    is_prefetching_new_tab_page_ad_ = true;

    bat_ads_->MaybeServeNewTabPageAd(
        base::BindOnce(&AdsServiceImpl::OnPrefetchNewTabPageAd, AsWeakPtr()));
  }
}

absl::optional<NewTabPageAdInfo>
AdsServiceImpl::GetPrefetchedNewTabPageAdForDisplay() {
  if (!bat_ads_.is_bound()) {
    return absl::nullopt;
  }

  absl::optional<NewTabPageAdInfo> ad;
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

  PurgeOrphanedAdEventsForType(mojom::AdType::kNewTabPageAd, base::DoNothing());
}

void AdsServiceImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (bat_ads_.is_bound()) {
    bat_ads_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                       event_type);
  }
}

void AdsServiceImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (bat_ads_.is_bound()) {
    bat_ads_->TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                            event_type);
  }
}

void AdsServiceImpl::TriggerSearchResultAdEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (bat_ads_.is_bound()) {
    bat_ads_->TriggerSearchResultAdEvent(std::move(ad_mojom), event_type);
  }
}

void AdsServiceImpl::PurgeOrphanedAdEventsForType(
    const mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  CHECK(mojom::IsKnownEnumValue(ad_type));

  if (bat_ads_.is_bound()) {
    bat_ads_->PurgeOrphanedAdEventsForType(ad_type, std::move(callback));
  }
}

void AdsServiceImpl::GetHistory(const base::Time from_time,
                                const base::Time to_time,
                                GetHistoryCallback callback) {
  if (bat_ads_.is_bound()) {
    bat_ads_->GetHistory(from_time, to_time, std::move(callback));
  }
}

void AdsServiceImpl::ToggleLikeAd(base::Value::Dict value,
                                  ToggleLikeAdCallback callback) {
  if (bat_ads_.is_bound()) {
    bat_ads_->ToggleLikeAd(std::move(value), std::move(callback));
  }
}

void AdsServiceImpl::ToggleDislikeAd(base::Value::Dict value,
                                     ToggleDislikeAdCallback callback) {
  if (bat_ads_.is_bound()) {
    bat_ads_->ToggleDislikeAd(std::move(value), std::move(callback));
  }
}

void AdsServiceImpl::ToggleLikeCategory(
    const std::string& category,
    const mojom::UserReactionType user_reaction_type,
    ToggleLikeCategoryCallback callback) {
  if (bat_ads_.is_bound()) {
    bat_ads_->ToggleLikeCategory(category, user_reaction_type,
                                 std::move(callback));
  }
}
void AdsServiceImpl::ToggleDislikeCategory(
    const std::string& category,
    const mojom::UserReactionType user_reaction_type,
    ToggleDislikeCategoryCallback callback) {
  if (bat_ads_.is_bound()) {
    bat_ads_->ToggleDislikeCategory(category, user_reaction_type,
                                    std::move(callback));
  }
}

void AdsServiceImpl::ToggleSaveAd(base::Value::Dict value,
                                  ToggleSaveAdCallback callback) {
  if (bat_ads_.is_bound()) {
    bat_ads_->ToggleSaveAd(std::move(value), std::move(callback));
  }
}

void AdsServiceImpl::ToggleMarkAdAsInappropriate(
    base::Value::Dict value,
    ToggleMarkAdAsInappropriateCallback callback) {
  if (bat_ads_.is_bound()) {
    bat_ads_->ToggleMarkAdAsInappropriate(std::move(value),
                                          std::move(callback));
  }
}

void AdsServiceImpl::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyTabTextContentDidChange(
        tab_id, redirect_chain, text);
  }
}

void AdsServiceImpl::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyTabHtmlContentDidChange(
        tab_id, redirect_chain, html);
  }
}

void AdsServiceImpl::NotifyTabDidStartPlayingMedia(const int32_t tab_id) {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyTabDidStartPlayingMedia(tab_id);
  }
}

void AdsServiceImpl::NotifyTabDidStopPlayingMedia(const int32_t tab_id) {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyTabDidStopPlayingMedia(tab_id);
  }
}

void AdsServiceImpl::NotifyTabDidChange(const int32_t tab_id,
                                        const std::vector<GURL>& redirect_chain,
                                        const bool is_visible,
                                        const bool is_incognito) {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyTabDidChange(tab_id, redirect_chain,
                                                 is_visible, is_incognito);
  }
}

void AdsServiceImpl::NotifyDidCloseTab(const int32_t tab_id) {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyDidCloseTab(tab_id);
  }
}

void AdsServiceImpl::NotifyUserGestureEventTriggered(
    const int32_t page_transition_type) {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyUserGestureEventTriggered(
        page_transition_type);
  }
}

void AdsServiceImpl::NotifyBrowserDidBecomeActive() {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyBrowserDidBecomeActive();
  }
}

void AdsServiceImpl::NotifyBrowserDidResignActive() {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyBrowserDidResignActive();
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
    // set_never_timeout uses an XPC service which requires signing so for now
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

void AdsServiceImpl::ShowReminder(const mojom::ReminderType type) {
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  if (IsEnabled() && CheckIfCanShowNotificationAds()) {
    absl::optional<base::Value::Dict> reminder = GetReminder(type);
    if (!reminder) {
      NOTREACHED_NORETURN();
    }

    // TODO(https://github.com/brave/brave-browser/issues/29587): Decouple Brave
    // Ads reminders from notification ads.
    ShowNotificationAd(std::move(*reminder));
  }
#endif
}

void AdsServiceImpl::UpdateAdRewards() {
  for (AdsServiceObserver& observer : observers_) {
    observer.OnAdRewardsDidChange();
  }
}

void AdsServiceImpl::RecordAdEventForId(const std::string& id,
                                        const std::string& ad_type,
                                        const std::string& confirmation_type,
                                        const base::Time time) {
  FrequencyCappingHelper::GetInstance()->RecordAdEventForId(
      id, ad_type, confirmation_type, time);
}

void AdsServiceImpl::GetAdEventHistory(const std::string& ad_type,
                                       const std::string& confirmation_type,
                                       GetAdEventHistoryCallback callback) {
  std::move(callback).Run(
      FrequencyCappingHelper::GetInstance()->GetAdEventHistory(
          ad_type, confirmation_type));
}

void AdsServiceImpl::ResetAdEventHistoryForId(const std::string& id) {
  return FrequencyCappingHelper::GetInstance()->ResetAdEventHistoryForId(id);
}

void AdsServiceImpl::GetBrowsingHistory(const int max_count,
                                        const int days_ago,
                                        GetBrowsingHistoryCallback callback) {
  const std::u16string search_text;
  history::QueryOptions options;
  options.SetRecentDayRange(days_ago);
  options.max_count = max_count;
  options.duplicate_policy = history::QueryOptions::REMOVE_ALL_DUPLICATES;
  history_service_->QueryHistory(
      search_text, options,
      base::BindOnce(
          [](GetBrowsingHistoryCallback callback,
             history::QueryResults results) {
            std::vector<GURL> history;
            for (const auto& result : results) {
              history.push_back(result.url().GetWithEmptyPath());
            }

            base::ranges::sort(history);
            history.erase(base::ranges::unique(history), history.cend());
            std::move(callback).Run(history);
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
      base::BindOnce(&OnURLResponseStarted));

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
      base::BindOnce(&AdsServiceImpl::OnURLRequest, base::Unretained(this),
                     url_loader_iter, std::move(callback)));
}

void AdsServiceImpl::Save(const std::string& name,
                          const std::string& value,
                          SaveCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::ImportantFileWriter::WriteFileAtomically,
                     base_path_.AppendASCII(name), value, base::StringPiece()),
      std::move(callback));
}

void AdsServiceImpl::Load(const std::string& name, LoadCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(
          [](LoadCallback callback, const std::string& value) {
            const bool success = !value.empty();
            std::move(callback).Run(success, value);
          },
          std::move(callback)));
}

void AdsServiceImpl::LoadFileResource(const std::string& id,
                                      const int version,
                                      LoadFileResourceCallback callback) {
  const absl::optional<base::FilePath> file_path_optional =
      g_brave_browser_process->resource_component()->GetPath(id, version);
  if (!file_path_optional) {
    return std::move(callback).Run({});
  }
  base::FilePath file_path = file_path_optional.value();

  VLOG(2) << "Loading file resource from " << file_path << " for component id "
          << id;

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
          std::move(file_path), file_task_runner_),
      base::BindOnce(
          [](LoadFileResourceCallback callback,
             std::unique_ptr<base::File, base::OnTaskRunnerDeleter> file) {
            CHECK(file);
            std::move(callback).Run(std::move(*file));
          },
          std::move(callback)));
}

void AdsServiceImpl::LoadDataResource(const std::string& name,
                                      LoadDataResourceCallback callback) {
  const int id = GetDataResourceId(name);

  std::string data_resource;

  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    data_resource = resource_bundle.LoadDataResourceString(id);
  } else {
    data_resource =
        static_cast<std::string>(resource_bundle.GetRawDataResource(id));
  }

  std::move(callback).Run(data_resource);
}

void AdsServiceImpl::GetScheduledCaptcha(const std::string& payment_id,
                                         GetScheduledCaptchaCallback callback) {
  adaptive_captcha_service_->GetScheduledCaptcha(payment_id,
                                                 std::move(callback));
}

void AdsServiceImpl::ShowScheduledCaptchaNotification(
    const std::string& payment_id,
    const std::string& captcha_id) {
#if BUILDFLAG(IS_ANDROID)
  ShowScheduledCaptcha(payment_id, captcha_id);
#else   // BUILDFLAG(IS_ANDROID)
  const PrefService* const pref_service = profile_->GetPrefs();

  if (pref_service->GetBoolean(
          brave_adaptive_captcha::prefs::kScheduledCaptchaPaused)) {
    VLOG(1) << "Ads paused; support intervention required";
    return;
  }

  const int snooze_count = pref_service->GetInteger(
      brave_adaptive_captcha::prefs::kScheduledCaptchaSnoozeCount);

  CHECK(ads_tooltips_delegate_);

  ads_tooltips_delegate_->ShowCaptchaTooltip(
      payment_id, captcha_id, snooze_count == 0,
      base::BindOnce(&AdsServiceImpl::ShowScheduledCaptcha, AsWeakPtr()),
      base::BindOnce(&AdsServiceImpl::SnoozeScheduledCaptcha, AsWeakPtr()));
#endif  // !BUILDFLAG(IS_ANDROID)
}

void AdsServiceImpl::ClearScheduledCaptcha() {
  adaptive_captcha_service_->ClearScheduledCaptcha();
}

void AdsServiceImpl::RunDBTransaction(mojom::DBTransactionInfoPtr transaction,
                                      RunDBTransactionCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&RunDBTransactionOnFileTaskRunner, std::move(transaction),
                     database_.get()),
      std::move(callback));
}

void AdsServiceImpl::RecordP2AEvent(const std::string& /*name*/,
                                    base::Value::List list) {
  for (const auto& item : list) {
    CHECK(item.is_string());
    RecordInWeeklyStorageAndEmitP2AHistogramAnswer(profile_->GetPrefs(),
                                                   item.GetString());
  }
}

void AdsServiceImpl::AddTrainingSample(
    std::vector<brave_federated::mojom::CovariateInfoPtr> training_sample) {
  if (!notification_ad_timing_data_store_) {
    return;
  }

  notification_ad_timing_data_store_->AddTrainingInstance(
      std::move(training_sample), base::BindOnce(&OnAddTrainingSample));
}

void AdsServiceImpl::GetBooleanPref(const std::string& path,
                                    GetBooleanPrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetBoolean(path));
}

void AdsServiceImpl::SetBooleanPref(const std::string& path, const bool value) {
  GetPrefService()->SetBoolean(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::GetIntegerPref(const std::string& path,
                                    GetIntegerPrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetInteger(path));
}

void AdsServiceImpl::SetIntegerPref(const std::string& path, const int value) {
  GetPrefService()->SetInteger(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::GetDoublePref(const std::string& path,
                                   GetDoublePrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetDouble(path));
}

void AdsServiceImpl::SetDoublePref(const std::string& path,
                                   const double value) {
  GetPrefService()->SetDouble(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::GetStringPref(const std::string& path,
                                   GetStringPrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetString(path));
}

void AdsServiceImpl::SetStringPref(const std::string& path,
                                   const std::string& value) {
  GetPrefService()->SetString(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::GetInt64Pref(const std::string& path,
                                  GetInt64PrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetInt64(path));
}

void AdsServiceImpl::SetInt64Pref(const std::string& path,
                                  const int64_t value) {
  GetPrefService()->SetInt64(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::GetUint64Pref(const std::string& path,
                                   GetUint64PrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetUint64(path));
}

void AdsServiceImpl::SetUint64Pref(const std::string& path,
                                   const uint64_t value) {
  GetPrefService()->SetUint64(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::GetTimePref(const std::string& path,
                                 GetTimePrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetTime(path));
}

void AdsServiceImpl::SetTimePref(const std::string& path,
                                 const base::Time value) {
  GetPrefService()->SetTime(path, value);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::GetDictPref(const std::string& path,
                                 GetDictPrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetDict(path).Clone());
}

void AdsServiceImpl::SetDictPref(const std::string& path,
                                 base::Value::Dict value) {
  GetPrefService()->SetDict(path, std::move(value));
  NotifyPrefChanged(path);
}

void AdsServiceImpl::GetListPref(const std::string& path,
                                 GetListPrefCallback callback) {
  std::move(callback).Run(GetPrefService()->GetList(path).Clone());
}

void AdsServiceImpl::SetListPref(const std::string& path,
                                 base::Value::List value) {
  GetPrefService()->SetList(path, std::move(value));
  NotifyPrefChanged(path);
}

void AdsServiceImpl::ClearPref(const std::string& path) {
  GetPrefService()->ClearPref(path);
  NotifyPrefChanged(path);
}

void AdsServiceImpl::HasPrefPath(const std::string& path,
                                 HasPrefPathCallback callback) {
  std::move(callback).Run(GetPrefService()->HasPrefPath(path));
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

void AdsServiceImpl::OnBrowserDidEnterForeground() {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyBrowserDidEnterForeground();
  }
}

void AdsServiceImpl::OnBrowserDidEnterBackground() {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyBrowserDidEnterBackground();
  }
}

void AdsServiceImpl::OnDidUpdateResourceComponent(const std::string& id) {
  if (bat_ads_client_notifier_.is_bound()) {
    bat_ads_client_notifier_->NotifyDidUpdateResourceComponent(id);
  }
}

void AdsServiceImpl::OnRewardsWalletUpdated() {
  GetRewardsWallet();
}

void AdsServiceImpl::OnExternalWalletConnected() {
  SetBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser, true);

  ShowReminder(mojom::ReminderType::kExternalWalletConnected);
}

void AdsServiceImpl::OnCompleteReset(const bool success) {
  if (success) {
    ShutdownAndResetState();
  }
}

}  // namespace brave_ads
