/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/check.h"
#include "base/containers/circular_deque.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/cxx17_backports.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"  // IWYU pragma: keep
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/metrics/field_trial_params.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "bat/ads/ad_constants.h"
#include "bat/ads/ads.h"
#include "bat/ads/database.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/new_tab_page_ad_value_util.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/notification_ad_value_util.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/resources/grit/bat_ads_resources.h"
#include "brave/browser/brave_ads/notification_helper/notification_helper.h"
#include "brave/browser/brave_ads/notifications/notification_ad_platform_bridge.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_ads/browser/ads_p2a.h"
#include "brave/components/brave_ads/browser/ads_storage_cleanup.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component.h"
#include "brave/components/brave_ads/browser/device_id.h"
#include "brave/components/brave_ads/browser/frequency_capping_helper.h"
#include "brave/components/brave_ads/browser/service_sandbox_type.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_federated/data_stores/async_data_store.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/rpill/common/rpill.h"
#include "brave/components/services/bat_ads/public/cpp/ads_client_mojo_bridge.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom.h"
#include "build/build_config.h"  // IWYU pragma: keep
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
#include "chrome/browser/first_run/first_run.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"
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

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_adaptive_captcha/pref_names.h"
#include "brave/components/brave_ads/browser/ads_tooltips_delegate.h"
#endif

namespace brave_ads {

namespace {

constexpr unsigned int kRetryOnNetworkChangeCount = 1;

constexpr base::TimeDelta kBatAdsServiceRestartDelay = base::Seconds(1);

constexpr base::TimeDelta kBatAdsServiceRepeatedRestartDelay =
    base::Seconds(20);

constexpr base::TimeDelta kBatAdsServiceRepeatedRestartCheckInterval =
    base::Seconds(60);

constexpr int kHttpUpgradeRequiredStatusCode = 426;

constexpr char kNotificationAdUrlPrefix[] = "https://www.brave.com/ads/?";

const base::Feature kServing{"AdServing", base::FEATURE_ENABLED_BY_DEFAULT};

int GetDataResourceId(const std::string& name) {
  if (name == ads::data::resource::kCatalogJsonSchemaFilename) {
    return IDR_ADS_CATALOG_SCHEMA;
  }

  NOTREACHED();

  return -1;
}

std::string URLMethodToRequestType(ads::mojom::UrlRequestMethodType method) {
  DCHECK(ads::mojom::IsKnownEnumValue(method));

  switch (method) {
    case ads::mojom::UrlRequestMethodType::kGet: {
      return "GET";
    }

    case ads::mojom::UrlRequestMethodType::kPost: {
      return "POST";
    }

    case ads::mojom::UrlRequestMethodType::kPut: {
      return "PUT";
    }
  }
}

std::string LoadOnFileTaskRunner(const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

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
        VLOG(0) << "Failed to create " << ads_service_base_path.value();
        return false;
      }

      VLOG(1) << "Created " << ads_service_base_path.value();
    }

    base::FilePath confirmations_state_path =
        ads_service_base_path.AppendASCII("confirmations.json");

    VLOG(1) << "Migrating " << legacy_confirmations_state_path.value() << " to "
            << confirmations_state_path.value();

    if (!base::Move(legacy_confirmations_state_path,
                    confirmations_state_path)) {
      return false;
    }

    VLOG(1) << "Successfully migrated confirmation state";
  }

  if (base::PathExists(rewards_service_base_path)) {
    VLOG(1) << "Deleting " << rewards_service_base_path.value();

    if (!base::DeleteFile(rewards_service_base_path)) {
      VLOG(0) << "Failed to delete " << rewards_service_base_path.value();
    }
  }

  return true;
}

ads::mojom::DBCommandResponseInfoPtr RunDBTransactionOnFileTaskRunner(
    ads::mojom::DBTransactionInfoPtr transaction,
    ads::Database* database) {
  ads::mojom::DBCommandResponseInfoPtr command_response =
      ads::mojom::DBCommandResponseInfo::New();

  if (!database) {
    command_response->status =
        ads::mojom::DBCommandResponseInfo::StatusType::RESPONSE_ERROR;
  } else {
    database->RunTransaction(std::move(transaction), command_response.get());
  }

  return command_response;
}

std::string GetLocale() {
  return brave_l10n::LocaleHelper::GetInstance()->GetLocale();
}

void RegisterResourceComponentsForLocale(const std::string& locale) {
  g_brave_browser_process->resource_component()->RegisterComponentsForLocale(
      locale);
}

void RegisterResourceComponentsForCurrentLocale() {
  RegisterResourceComponentsForLocale(GetLocale());
}

void OnURLResponseStarted(
    const GURL& /*final_url*/,
    const network::mojom::URLResponseHead& response_head) {
  if (response_head.headers->response_code() == -1) {
    VLOG(6) << "Response headers are malformed!!";
    return;
  }
}

std::vector<std::string> ExtraCommandLineSwitches() {
  std::vector<std::string> command_line_switches;

  const std::string& rewards_command_line_switch =
      brave_rewards::RewardsFlags::GetCommandLineSwitchASCII();
  if (!rewards_command_line_switch.empty()) {
    command_line_switches.push_back(rewards_command_line_switch);
  }

  return command_line_switches;
}

}  // namespace

AdsServiceImpl::AdsServiceImpl(
    Profile* profile,
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
    brave_adaptive_captcha::BraveAdaptiveCaptchaService*
        adaptive_captcha_service,
    std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate,
#endif
    std::unique_ptr<DeviceId> device_id,
    history::HistoryService* history_service,
    brave_rewards::RewardsService* rewards_service,
    brave_federated::AsyncDataStore* notification_ad_timing_data_store)
    : profile_(profile),
      history_service_(history_service),
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
      adaptive_captcha_service_(adaptive_captcha_service),
      ads_tooltips_delegate_(std::move(ads_tooltips_delegate)),
#endif
      device_id_(std::move(device_id)),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      base_path_(profile_->GetPath().AppendASCII("ads_service")),
      display_service_(NotificationDisplayService::GetForProfile(profile_)),
      rewards_service_(rewards_service),
      notification_ad_timing_data_store_(notification_ad_timing_data_store),
      bat_ads_client_(new bat_ads::AdsClientMojoBridge(this)) {
  DCHECK(profile_);
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  DCHECK(adaptive_captcha_service_);
#endif
  DCHECK(device_id_);
  DCHECK(history_service_);
  DCHECK(brave::IsRegularProfile(profile_));

  MigratePrefs();

  InitNotificationsForProfile();

  MigrateConfirmationState();
}

AdsServiceImpl::~AdsServiceImpl() = default;

///////////////////////////////////////////////////////////////////////////////

bool AdsServiceImpl::IsBraveNewsEnabled() const {
  return base::FeatureList::IsEnabled(
             brave_today::features::kBraveNewsFeature) &&
         GetBooleanPref(brave_news::prefs::kBraveTodayOptedIn) &&
         GetBooleanPref(brave_news::prefs::kNewTabPageShowToday);
}

bool AdsServiceImpl::ShouldStartBatAds() const {
  return IsEnabled() || IsBraveNewsEnabled();
}

bool AdsServiceImpl::IsBatAdsServiceBound() const {
  return bat_ads_service_.is_bound();
}

bool AdsServiceImpl::IsBatAdsBound() const {
  return bat_ads_.is_bound() && !g_browser_process->IsShuttingDown();
}

void AdsServiceImpl::InitNotificationsForProfile() {
  NotificationHelper::GetInstance()->InitForProfile(profile_);
}

void AdsServiceImpl::MigrateConfirmationState() {
  const base::FilePath path = profile_->GetPath();
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&MigrateConfirmationStateOnFileTaskRunner, path),
      base::BindOnce(&AdsServiceImpl::OnMigrateConfirmationState, AsWeakPtr()));
}

void AdsServiceImpl::OnMigrateConfirmationState(const bool success) {
  if (!success) {
    VLOG(0) << "Failed to migrate confirmation state";
    return;
  }

  InitializePrefChangeRegistrar();

  MaybeStartOrStop(/* should_restart */ false);
}

void AdsServiceImpl::InitializePrefChangeRegistrar() {
  pref_change_registrar_.Init(profile_->GetPrefs());

  pref_change_registrar_.Add(
      ads::prefs::kEnabled,
      base::BindRepeating(&AdsServiceImpl::OnEnabledPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      ads::prefs::kIdleTimeThreshold,
      base::BindRepeating(&AdsServiceImpl::OnIdleTimeThresholdPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_rewards::prefs::kWalletBrave,
      base::BindRepeating(&AdsServiceImpl::OnWalletBravePrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_news::prefs::kBraveTodayOptedIn,
      base::BindRepeating(&AdsServiceImpl::OnBraveTodayOptedInPrefChanged,
                          base::Unretained(this)));

  pref_change_registrar_.Add(
      brave_news::prefs::kNewTabPageShowToday,
      base::BindRepeating(&AdsServiceImpl::OnNewTabPageShowTodayPrefChanged,
                          base::Unretained(this)));
}

void AdsServiceImpl::SetSysInfo() {
  bat_ads_service_->SetSysInfo(sys_info_.Clone(), base::NullCallback());
}

void AdsServiceImpl::SetBuildChannel() {
  ads::mojom::BuildChannelInfoPtr build_channel =
      ads::mojom::BuildChannelInfo::New();
  build_channel->name = brave::GetChannelName();
  build_channel->is_release = build_channel->name == "release";

  bat_ads_service_->SetBuildChannel(std::move(build_channel),
                                    base::NullCallback());
}

void AdsServiceImpl::MaybeStartOrStop(const bool should_restart) {
  if (!IsSupportedLocale()) {
    VLOG(1) << GetLocale() << " locale does not support ads";
    Shutdown();
    return;
  }

  if (!ShouldStartBatAds()) {
    VLOG(1) << "Shutting down ads service";
    Shutdown();
    return;
  }

  if (should_restart) {
    VLOG(1) << "Restarting ads service";
    Shutdown();
  }

  if (IsBatAdsBound()) {
    VLOG(1) << "Already started bat-ads service";
    return;
  }

  StartBatAdsService();

  ++total_number_of_starts_;
  if (should_restart) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&AdsServiceImpl::Start, AsWeakPtr(),
                       total_number_of_starts_),
        GetBatAdsServiceRestartDelay());
  } else {
    Start(total_number_of_starts_);
  }
}

void AdsServiceImpl::StartBatAdsService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!IsBatAdsBound());

  if (!IsBatAdsServiceBound()) {
    content::ServiceProcessHost::Launch(
        bat_ads_service_.BindNewPipeAndPassReceiver(),
        content::ServiceProcessHost::Options()
            .WithDisplayName(IDS_SERVICE_BAT_ADS)
            .WithExtraCommandLineSwitches(ExtraCommandLineSwitches())
            .Pass());

    bat_ads_service_.set_disconnect_handler(
        base::BindOnce(&AdsServiceImpl::MaybeStartOrStop, AsWeakPtr(),
                       /* should_restart */ true));
  }
}

base::TimeDelta AdsServiceImpl::GetBatAdsServiceRestartDelay() {
  base::TimeDelta delay = kBatAdsServiceRestartDelay;

  // Increase the restart delay if we have restarts that are too close.
  if (!last_bat_ads_service_restart_time_.is_null() &&
      last_bat_ads_service_restart_time_ +
              kBatAdsServiceRepeatedRestartCheckInterval >
          base::Time::Now()) {
    delay = kBatAdsServiceRepeatedRestartDelay;
  }

  last_bat_ads_service_restart_time_ = base::Time::Now();

  return delay;
}

void AdsServiceImpl::Start(const uint32_t number_of_start) {
  GetDeviceId(number_of_start);
}

void AdsServiceImpl::GetDeviceId(const uint32_t number_of_start) {
  device_id_->GetDeviceId(base::BindOnce(&AdsServiceImpl::OnGetDeviceId,
                                         AsWeakPtr(), number_of_start));
}

void AdsServiceImpl::OnGetDeviceId(const uint32_t number_of_start,
                                   std::string device_id) {
  sys_info_.device_id = device_id;

  DetectUncertainFuture(number_of_start);
}

void AdsServiceImpl::DetectUncertainFuture(const uint32_t number_of_start) {
  brave_rpill::DetectUncertainFuture(base::BindOnce(
      &AdsServiceImpl::OnDetectUncertainFuture, AsWeakPtr(), number_of_start));
}

void AdsServiceImpl::OnDetectUncertainFuture(const uint32_t number_of_start,
                                             const bool is_uncertain_future) {
  sys_info_.is_uncertain_future = is_uncertain_future;

  EnsureBaseDirectoryExists(number_of_start);
}

void AdsServiceImpl::EnsureBaseDirectoryExists(const uint32_t number_of_start) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&EnsureBaseDirectoryExistsOnFileTaskRunner, base_path_),
      base::BindOnce(&AdsServiceImpl::OnEnsureBaseDirectoryExists, AsWeakPtr(),
                     number_of_start));
}

void AdsServiceImpl::OnEnsureBaseDirectoryExists(const uint32_t number_of_start,
                                                 const bool success) {
  if (!success) {
    VLOG(0) << "Failed to create base directory";
    return;
  }

  CreateBatAdsService(number_of_start);

  RegisterResourceComponentsForCurrentLocale();

  GetRewardsWallet();
}

void AdsServiceImpl::CreateBatAdsService(const uint32_t number_of_start) {
  if (!ShouldStartBatAds() || !IsBatAdsServiceBound() ||
      number_of_start != total_number_of_starts_) {
    VLOG(1) << "Already created bat-ads service";
    return;
  }

  BackgroundHelper::GetInstance()->AddObserver(this);

  g_brave_browser_process->resource_component()->AddObserver(this);

  if (database_) {
    NOTREACHED() << "Ads service shutdown was not initiated prior to start";

    const uint32_t total_number_of_starts = total_number_of_starts_;
    base::debug::Alias(&total_number_of_starts);
    base::debug::Alias(&number_of_start);
    base::debug::DumpWithoutCrashing();

    // TODO(https://github.com/brave/brave-browser/issues/17643):
    // This is a temporary hack to make sure that all race conditions on
    // ads service start/shutdown are fixed. Need to craft more reliable
    // solution for a longer term.
    const bool success =
        file_task_runner_->DeleteSoon(FROM_HERE, database_.release());
    VLOG_IF(1, !success) << "Failed to release database";
  }

  database_ = std::make_unique<ads::Database>(
      base_path_.AppendASCII("database.sqlite"));

  bat_ads_service_->Create(
      bat_ads_client_.BindNewEndpointAndPassRemote(),
      bat_ads_.BindNewEndpointAndPassReceiver(),
      base::BindOnce(&AdsServiceImpl::OnCreateBatAdsService, AsWeakPtr()));
}

void AdsServiceImpl::OnCreateBatAdsService() {
  if (!IsBatAdsBound()) {
    return;
  }

  SetSysInfo();

  SetBuildChannel();

  bat_ads_->Initialize(
      base::BindOnce(&AdsServiceImpl::OnInitializeBatAds, AsWeakPtr()));
}

void AdsServiceImpl::OnInitializeBatAds(const bool success) {
  if (!success) {
    VLOG(0) << "Failed to initialize bat-ads";
    is_bat_ads_initialized_ = false;
    return;
  }

  is_bat_ads_initialized_ = true;

  CleanUpOnFirstRun();

  MaybeShowOnboardingNotification();

  MaybeOpenNewTabWithAd();

  StartCheckIdleStateTimer();
}

void AdsServiceImpl::CleanUpOnFirstRun() {
  if (did_cleanup_on_first_run_) {
    return;
  }

  did_cleanup_on_first_run_ = true;

  RemoveDeprecatedFiles();

  // Purge orphaned new tab page ad events which may have remained from the
  // previous browser startup. If prefetch_new_tab_page_ad_on_first_run_ value
  // is true, then it means that there was an attempt to prefetch a new tab page
  // ad prior to CleanUpOnFirstRun() call. In this case, repeating the prefetch
  // of a new tab page ad is needed after the purge.
  if (prefetch_new_tab_page_ad_on_first_run_) {
    prefetch_new_tab_page_ad_on_first_run_ = false;
    PurgeOrphanedAdEventsForType(
        ads::mojom::AdType::kNewTabPageAd,
        base::BindOnce(&AdsServiceImpl::OnPurgeOrphanedNewTabPageAdEvents,
                       AsWeakPtr()));
  } else {
    PurgeOrphanedAdEventsForType(ads::mojom::AdType::kNewTabPageAd,
                                 base::DoNothing());
  }
}

void AdsServiceImpl::RemoveDeprecatedFiles() const {
  file_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&RemoveDeprecatedAdsDataFiles, base_path_));
}

void AdsServiceImpl::ResetState() {
  VLOG(1) << "Resetting ads state";

  profile_->GetPrefs()->ClearPrefsWithPrefixSilently("brave.brave_ads");

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&DeletePathOnFileTaskRunner, base_path_),
      base::BindOnce(&AdsServiceImpl::OnResetState, AsWeakPtr()));
}

void AdsServiceImpl::OnResetState(const bool success) {
  if (!success) {
    VLOG(0) << "Failed to reset ads state";
    return;
  }

  VLOG(1) << "Successfully reset ads state";
}

void AdsServiceImpl::GetRewardsWallet() {
  if (!IsBatAdsBound()) {
    return;
  }

  rewards_service_->GetRewardsWallet(
      base::BindOnce(&AdsServiceImpl::OnGetRewardsWallet, AsWeakPtr()));
}

void AdsServiceImpl::OnGetRewardsWallet(
    ledger::mojom::RewardsWalletPtr wallet) {
  if (!IsBatAdsBound() || !wallet) {
    return;
  }

  bat_ads_->OnWalletUpdated(wallet->payment_id,
                            base::Base64Encode(wallet->recovery_seed));
}

void AdsServiceImpl::OnEnabledPrefChanged() {
  rewards_service_->OnAdsEnabled(IsEnabled());

  if (!IsEnabled()) {
    SuspendP2AHistograms();
    VLOG(1) << "P2A histograms suspended";

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
    adaptive_captcha_service_->ClearScheduledCaptcha();
#if !BUILDFLAG(IS_ANDROID)
    ads_tooltips_delegate_->CloseCaptchaTooltip();
#endif
#endif
  }

  brave_rewards::p3a::UpdateAdsStateOnPreferenceChange(profile_->GetPrefs(),
                                                       ads::prefs::kEnabled);

  MaybeStartOrStop(/* should_restart */ false);
}

void AdsServiceImpl::OnIdleTimeThresholdPrefChanged() {
  StartCheckIdleStateTimer();
}

void AdsServiceImpl::OnWalletBravePrefChanged() {
  GetRewardsWallet();
}

void AdsServiceImpl::OnBraveTodayOptedInPrefChanged() {
  MaybeStartOrStop(/* should_restart */ false);
}

void AdsServiceImpl::OnNewTabPageShowTodayPrefChanged() {
  MaybeStartOrStop(/* should_restart */ false);
}

void AdsServiceImpl::NotifyPrefChanged(const std::string& path) {
  if (IsBatAdsBound()) {
    bat_ads_->OnPrefDidChange(path);
  }
}

void AdsServiceImpl::StartCheckIdleStateTimer() {
#if !BUILDFLAG(IS_ANDROID)
  idle_state_timer_.Stop();

  idle_state_timer_.Start(FROM_HERE, base::Seconds(1), this,
                          &AdsServiceImpl::CheckIdleState);
#endif
}

void AdsServiceImpl::CheckIdleState() {
  const int idle_threshold = GetIntegerPref(ads::prefs::kIdleTimeThreshold);
  const ui::IdleState idle_state = ui::CalculateIdleState(idle_threshold);
  ProcessIdleState(idle_state, last_idle_time_);

  last_idle_time_ = base::Seconds(ui::CalculateIdleTime());
}

void AdsServiceImpl::ProcessIdleState(const ui::IdleState idle_state,
                                      const base::TimeDelta idle_time) {
  if (!IsBatAdsBound() || idle_state == last_idle_state_) {
    return;
  }

  switch (idle_state) {
    case ui::IdleState::IDLE_STATE_ACTIVE: {
      const bool was_locked =
          last_idle_state_ == ui::IdleState::IDLE_STATE_LOCKED;
      bat_ads_->OnUnIdle(idle_time, was_locked);
      break;
    }

    case ui::IdleState::IDLE_STATE_IDLE:
    case ui::IdleState::IDLE_STATE_LOCKED: {
      bat_ads_->OnIdle();
      break;
    }

    case ui::IdleState::IDLE_STATE_UNKNOWN: {
      break;
    }
  }

  last_idle_state_ = idle_state;
}

bool AdsServiceImpl::ShouldShowCustomNotificationAds() {
  const bool can_show_native_notifications =
      NotificationHelper::GetInstance()->CanShowNotifications();

  bool can_fallback_to_custom_notification_ads =
      features::CanFallbackToCustomNotificationAds();
  if (!can_fallback_to_custom_notification_ads) {
    ClearPref(prefs::kNotificationAdDidFallbackToCustom);
  } else {
    const bool allowed_to_fallback_to_custom_notification_ads =
        features::IsAllowedToFallbackToCustomNotificationAdsEnabled();
    if (!allowed_to_fallback_to_custom_notification_ads) {
      can_fallback_to_custom_notification_ads = false;
    }
  }

  const bool should_show = features::IsCustomNotificationAdsEnabled();

  const bool should_fallback =
      !can_show_native_notifications && can_fallback_to_custom_notification_ads;
  if (should_fallback) {
    SetBooleanPref(prefs::kNotificationAdDidFallbackToCustom, true);
  }

  const bool did_fallback =
      GetBooleanPref(prefs::kNotificationAdDidFallbackToCustom);

  return should_show || should_fallback || did_fallback;
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
    VLOG(1) << "Cancelled timeout for notification ad with placement id "
            << placement_id;
  }

  if (!IsBatAdsBound() || !is_bat_ads_initialized_) {
    RetryOpeningNewTabWithAd(placement_id);
    return;
  }

  bat_ads_->MaybeGetNotificationAd(
      placement_id,
      base::BindOnce(&AdsServiceImpl::OnGetNotificationAd, AsWeakPtr()));
}

void AdsServiceImpl::OnGetNotificationAd(
    absl::optional<base::Value::Dict> dict) {
  if (!dict) {
    VLOG(0) << "Failed to get notification ad";
    return;
  }

  const ads::NotificationAdInfo notification =
      ads::NotificationAdFromValue(*dict);

  OpenNewTabWithUrl(notification.target_url);
}

void AdsServiceImpl::RetryOpeningNewTabWithAd(const std::string& placement_id) {
  VLOG(1) << "Retry opening new tab for ad with placement id " << placement_id;
  retry_opening_new_tab_for_ad_with_placement_id_ = placement_id;
}

void AdsServiceImpl::OpenNewTabWithUrl(const GURL& url) {
  if (g_browser_process->IsShuttingDown()) {
    return;
  }

  if (!url.is_valid()) {
    VLOG(0) << "Failed to open new tab due to invalid URL: " << url;
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

void AdsServiceImpl::StartNotificationAdTimeOutTimer(
    const std::string& placement_id) {
#if BUILDFLAG(IS_ANDROID)
  if (!ShouldShowCustomNotificationAds()) {
    return;
  }
#endif

  const int timeout_in_seconds = features::NotificationAdTimeout();
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

  VLOG(1) << "Timeout notification ad with placement id " << placement_id
          << " in " << timeout;
}

bool AdsServiceImpl::StopNotificationAdTimeOutTimer(
    const std::string& placement_id) {
  const auto iter = notification_ad_timers_.find(placement_id);
  if (iter == notification_ad_timers_.end()) {
    return false;
  }

  notification_ad_timers_.erase(iter);

  return true;
}

void AdsServiceImpl::NotificationAdTimedOut(const std::string& placement_id) {
  if (!IsBatAdsBound()) {
    return;
  }

  VLOG(1) << "Timed-out notification ad with placement id " << placement_id;

  CloseNotificationAd(placement_id);

  if (!ShouldShowCustomNotificationAds() &&
      NotificationHelper::GetInstance()->DoesSupportSystemNotifications()) {
    bat_ads_->TriggerNotificationAdEvent(
        placement_id, ads::mojom::NotificationAdEventType::kTimedOut);
  }
}

void AdsServiceImpl::Shutdown() {
  VLOG(1) << "Shutting down ads service";

  is_bat_ads_initialized_ = false;

  bat_ads_.reset();
  bat_ads_client_.reset();
  bat_ads_service_.reset();

  BackgroundHelper::GetInstance()->RemoveObserver(this);

  g_brave_browser_process->resource_component()->RemoveObserver(this);

  CloseAllNotificationAds();

  url_loaders_.clear();

  idle_state_timer_.Stop();

  const bool success =
      file_task_runner_->DeleteSoon(FROM_HERE, database_.release());
  VLOG_IF(1, !success) << "Failed to release database";

  VLOG(1) << "Shutdown ads service";
}

bool AdsServiceImpl::IsSupportedLocale() const {
  const std::string locale = GetLocale();
  return ads::IsSupportedLocale(locale);
}

bool AdsServiceImpl::IsEnabled() const {
  return GetBooleanPref(ads::prefs::kEnabled);
}

void AdsServiceImpl::SetEnabled(const bool is_enabled) {
  SetBooleanPref(ads::prefs::kEnabled, is_enabled);
}

int64_t AdsServiceImpl::GetMaximumNotificationAdsPerHour() const {
  int64_t ads_per_hour =
      GetInt64Pref(ads::prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == -1) {
    ads_per_hour = base::GetFieldTrialParamByFeatureAsInt(
        kServing, "default_ad_notifications_per_hour",
        ads::kDefaultNotificationAdsPerHour);
  }

  return base::clamp(ads_per_hour,
                     static_cast<int64_t>(ads::kMinimumNotificationAdsPerHour),
                     static_cast<int64_t>(ads::kMaximumNotificationAdsPerHour));
}

void AdsServiceImpl::SetMaximumNotificationAdsPerHour(
    const int64_t ads_per_hour) {
  DCHECK(ads_per_hour >= ads::kMinimumNotificationAdsPerHour &&
         ads_per_hour <= ads::kMaximumNotificationAdsPerHour);
  SetInt64Pref(ads::prefs::kMaximumNotificationAdsPerHour, ads_per_hour);
}

bool AdsServiceImpl::ShouldAllowSubdivisionTargeting() const {
  return GetBooleanPref(ads::prefs::kShouldAllowSubdivisionTargeting);
}

std::string AdsServiceImpl::GetSubdivisionTargetingCode() const {
  return GetStringPref(ads::prefs::kSubdivisionTargetingCode);
}

void AdsServiceImpl::SetSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  SetStringPref(ads::prefs::kSubdivisionTargetingCode,
                subdivision_targeting_code);
}

std::string AdsServiceImpl::GetAutoDetectedSubdivisionTargetingCode() const {
  return GetStringPref(ads::prefs::kAutoDetectedSubdivisionTargetingCode);
}

void AdsServiceImpl::SetAutoDetectedSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  SetStringPref(ads::prefs::kAutoDetectedSubdivisionTargetingCode,
                subdivision_targeting_code);
}

bool AdsServiceImpl::NeedsBrowserUpgradeToServeAds() const {
  return needs_browser_upgrade_to_serve_ads_;
}

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
void AdsServiceImpl::ShowScheduledCaptcha(const std::string& payment_id,
                                          const std::string& captcha_id) {
  adaptive_captcha_service_->ShowScheduledCaptcha(payment_id, captcha_id);
}

void AdsServiceImpl::SnoozeScheduledCaptcha() {
  adaptive_captcha_service_->SnoozeScheduledCaptcha();
}
#endif

void AdsServiceImpl::OnNotificationAdShown(const std::string& placement_id) {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->TriggerNotificationAdEvent(
      placement_id, ads::mojom::NotificationAdEventType::kViewed);
}

void AdsServiceImpl::OnNotificationAdClosed(const std::string& placement_id,
                                            const bool by_user) {
  if (StopNotificationAdTimeOutTimer(placement_id)) {
    VLOG(1) << "Cancelled timeout for notification ad with placement id "
            << placement_id;
  }

  if (!IsBatAdsBound()) {
    return;
  }

  const ads::mojom::NotificationAdEventType event_type =
      by_user ? ads::mojom::NotificationAdEventType::kDismissed
              : ads::mojom::NotificationAdEventType::kTimedOut;

  bat_ads_->TriggerNotificationAdEvent(placement_id, event_type);
}

void AdsServiceImpl::OnNotificationAdClicked(const std::string& placement_id) {
  if (!IsBatAdsBound()) {
    return;
  }

  OpenNewTabWithAd(placement_id);

  bat_ads_->TriggerNotificationAdEvent(
      placement_id, ads::mojom::NotificationAdEventType::kClicked);
}

void AdsServiceImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  if (!IsBatAdsBound()) {
    std::move(callback).Run(/* diagnostics */ absl::nullopt);
    return;
  }

  bat_ads_->GetDiagnostics(std::move(callback));
}

void AdsServiceImpl::OnLocaleDidChange(const std::string& locale) {
  if (!IsBatAdsBound()) {
    return;
  }

  RegisterResourceComponentsForLocale(locale);

  bat_ads_->OnLocaleDidChange(locale);
}

void AdsServiceImpl::OnTabHtmlContentDidChange(
    const SessionID& tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnTabHtmlContentDidChange(tab_id.id(), redirect_chain, html);
}

void AdsServiceImpl::OnTabTextContentDidChange(
    const SessionID& tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnTabTextContentDidChange(tab_id.id(), redirect_chain, text);
}

void AdsServiceImpl::OnUserGesture(const int32_t page_transition_type) {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnUserGesture(page_transition_type);
}

void AdsServiceImpl::OnMediaStart(const SessionID& tab_id) {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnMediaPlaying(tab_id.id());
}

void AdsServiceImpl::OnMediaStop(const SessionID& tab_id) {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnMediaStopped(tab_id.id());
}

void AdsServiceImpl::OnTabUpdated(const SessionID& tab_id,
                                  const std::vector<GURL>& redirect_chain,
                                  const bool is_active,
                                  const bool is_browser_active) {
  if (!IsBatAdsBound()) {
    return;
  }

  const bool is_incognito = !brave::IsRegularProfile(profile_);

  bat_ads_->OnTabUpdated(tab_id.id(), redirect_chain, is_active,
                         is_browser_active, is_incognito);
}

void AdsServiceImpl::OnTabClosed(const SessionID& tab_id) {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnTabClosed(tab_id.id());
}

void AdsServiceImpl::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  if (!IsBatAdsBound()) {
    std::move(callback).Run(/* statement */ nullptr);
    return;
  }

  bat_ads_->GetStatementOfAccounts(std::move(callback));
}

void AdsServiceImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (!IsBatAdsBound()) {
    std::move(callback).Run(dimensions, /*inline_content_ad*/ absl::nullopt);
    return;
  }

  bat_ads_->MaybeServeInlineContentAd(dimensions, std::move(callback));
}

void AdsServiceImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::InlineContentAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                        event_type);
}

absl::optional<ads::NewTabPageAdInfo>
AdsServiceImpl::GetPrefetchedNewTabPageAd() {
  if (!IsBatAdsBound()) {
    return absl::nullopt;
  }

  absl::optional<ads::NewTabPageAdInfo> ad;
  if (prefetched_new_tab_page_ad_) {
    ad = prefetched_new_tab_page_ad_;
    prefetched_new_tab_page_ad_.reset();
  }

  return ad;
}

void AdsServiceImpl::OnFailedToPrefetchNewTabPageAd(
    const std::string& /*placement_id*/,
    const std::string& /*creative_instance_id*/) {
  need_purge_orphaned_new_tab_page_ad_events_ = true;
}

void AdsServiceImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::NewTabPageAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                     event_type);
}

void AdsServiceImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::PromotedContentAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                          event_type);
}

void AdsServiceImpl::TriggerSearchResultAdEvent(
    ads::mojom::SearchResultAdInfoPtr ad_mojom,
    const ads::mojom::SearchResultAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->TriggerSearchResultAdEvent(std::move(ad_mojom), event_type);
}

void AdsServiceImpl::PurgeOrphanedAdEventsForType(
    const ads::mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  DCHECK(ads::mojom::IsKnownEnumValue(ad_type));

  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->PurgeOrphanedAdEventsForType(ad_type, std::move(callback));
}

void AdsServiceImpl::GetHistory(const base::Time from_time,
                                const base::Time to_time,
                                GetHistoryCallback callback) {
  if (IsBatAdsBound()) {
    bat_ads_->GetHistory(from_time, to_time, std::move(callback));
  }
}

void AdsServiceImpl::ToggleAdThumbUp(base::Value::Dict value,
                                     ToggleAdThumbUpCallback callback) {
  if (IsBatAdsBound()) {
    bat_ads_->ToggleAdThumbUp(std::move(value), std::move(callback));
  }
}

void AdsServiceImpl::ToggleAdThumbDown(base::Value::Dict value,
                                       ToggleAdThumbDownCallback callback) {
  if (IsBatAdsBound()) {
    bat_ads_->ToggleAdThumbDown(std::move(value), std::move(callback));
  }
}

void AdsServiceImpl::ToggleAdOptIn(const std::string& category,
                                   const int action,
                                   ToggleAdOptInCallback callback) {
  if (IsBatAdsBound()) {
    bat_ads_->ToggleAdOptIn(category, action, std::move(callback));
  }
}

void AdsServiceImpl::ToggleAdOptOut(const std::string& category,
                                    const int action,
                                    ToggleAdOptOutCallback callback) {
  if (IsBatAdsBound()) {
    bat_ads_->ToggleAdOptOut(category, action, std::move(callback));
  }
}

void AdsServiceImpl::ToggleSavedAd(base::Value::Dict value,
                                   ToggleSavedAdCallback callback) {
  if (IsBatAdsBound()) {
    bat_ads_->ToggleSavedAd(std::move(value), std::move(callback));
  }
}

void AdsServiceImpl::ToggleFlaggedAd(base::Value::Dict value,
                                     ToggleFlaggedAdCallback callback) {
  if (IsBatAdsBound()) {
    bat_ads_->ToggleFlaggedAd(std::move(value), std::move(callback));
  }
}

void AdsServiceImpl::WipeState(const bool should_shutdown) {
  if (should_shutdown) {
    Shutdown();
  }

  ResetState();
}

bool AdsServiceImpl::IsNetworkConnectionAvailable() const {
  return !net::NetworkChangeNotifier::IsOffline();
}

bool AdsServiceImpl::IsBrowserActive() const {
  return BackgroundHelper::GetInstance()->IsForeground();
}

bool AdsServiceImpl::IsBrowserInFullScreenMode() const {
#if !BUILDFLAG(IS_ANDROID)
  return IsFullScreenMode();
#else
  return true;
#endif
}

bool AdsServiceImpl::CanShowNotificationAds() {
  if (!features::IsNotificationAdsEnabled()) {
    LOG(INFO) << "Notification not made: Ad notifications feature is disabled";
    return false;
  }

  if (!NotificationHelper::GetInstance()->CanShowNotifications()) {
    return ShouldShowCustomNotificationAds();
  }

  return true;
}

bool AdsServiceImpl::CanShowNotificationAdsWhileBrowserIsBackgrounded() const {
  return NotificationHelper::GetInstance()
      ->CanShowSystemNotificationsWhileBrowserIsBackgrounded();
}

void AdsServiceImpl::ShowNotificationAd(const ads::NotificationAdInfo& ad) {
  if (ShouldShowCustomNotificationAds()) {
    std::unique_ptr<NotificationAdPlatformBridge> platform_bridge =
        std::make_unique<NotificationAdPlatformBridge>(profile_);

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

    std::unique_ptr<message_center::Notification> notification =
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
        std::make_unique<NotificationAdPlatformBridge>(profile_);

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

void AdsServiceImpl::CloseAllNotificationAds() {
  // TODO(https://github.com/brave/brave-browser/issues/25410): Temporary
  // solution until we refactor the shutdown business logic and investigate
  // calling |ads::NotificationAdManager| to cleanup notification ads.

#if BUILDFLAG(IS_ANDROID)
  if (!ShouldShowCustomNotificationAds()) {
    return;
  }
#endif

  // TODO(https://github.com/brave/brave-browser/issues/22489): Temporary
  // solution until we move all ads prefs to |components/brave_ads/pref_names|.
  constexpr char kNotificationAdsPrefName[] =
      "brave.brave_ads.notification_ads";

  const base::Value::List& list =
      profile_->GetPrefs()->GetValueList(kNotificationAdsPrefName);

  const base::circular_deque<ads::NotificationAdInfo> ads =
      ads::NotificationAdsFromValue(list);

  for (const auto& ad : ads) {
    CloseNotificationAd(ad.placement_id);
  }

  profile_->GetPrefs()->SetList(kNotificationAdsPrefName, {});
}

void AdsServiceImpl::UpdateAdRewards() {
  for (AdsServiceObserver& observer : observers_) {
    observer.OnAdRewardsDidChange();
  }
}

void AdsServiceImpl::RecordAdEventForId(const std::string& id,
                                        const std::string& ad_type,
                                        const std::string& confirmation_type,
                                        const base::Time time) const {
  FrequencyCappingHelper::GetInstance()->RecordAdEventForId(
      id, ad_type, confirmation_type, time);
}

std::vector<base::Time> AdsServiceImpl::GetAdEventHistory(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  return FrequencyCappingHelper::GetInstance()->GetAdEventHistory(
      ad_type, confirmation_type);
}

void AdsServiceImpl::ResetAdEventHistoryForId(const std::string& id) const {
  return FrequencyCappingHelper::GetInstance()->ResetAdEventHistoryForId(id);
}

void AdsServiceImpl::GetBrowsingHistory(
    const int max_count,
    const int days_ago,
    ads::GetBrowsingHistoryCallback callback) {
  std::u16string search_text;
  history::QueryOptions options;
  options.SetRecentDayRange(days_ago);
  options.max_count = max_count;
  options.duplicate_policy = history::QueryOptions::REMOVE_ALL_DUPLICATES;
  history_service_->QueryHistory(
      search_text, options,
      base::BindOnce(&AdsServiceImpl::OnBrowsingHistorySearchComplete,
                     AsWeakPtr(), std::move(callback)),
      &history_service_task_tracker_);
}

void AdsServiceImpl::UrlRequest(ads::mojom::UrlRequestInfoPtr url_request,
                                ads::UrlRequestCallback callback) {
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
      kRetryOnNetworkChangeCount,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  url_loader->SetAllowHttpErrorResults(true);

  auto url_loader_iter =
      url_loaders_.insert(url_loaders_.end(), std::move(url_loader));
  url_loader_iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      profile_->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess()
          .get(),
      base::BindOnce(&AdsServiceImpl::OnURLRequest, base::Unretained(this),
                     url_loader_iter, std::move(callback)));
}

void AdsServiceImpl::Save(const std::string& name,
                          const std::string& value,
                          ads::SaveCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&base::ImportantFileWriter::WriteFileAtomically,
                     base_path_.AppendASCII(name), value, base::StringPiece()),
      std::move(callback));
}

void AdsServiceImpl::Load(const std::string& name, ads::LoadCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(&AdsServiceImpl::OnLoad, AsWeakPtr(),
                     std::move(callback)));
}

void AdsServiceImpl::LoadFileResource(const std::string& id,
                                      const int version,
                                      ads::LoadFileCallback callback) {
  const absl::optional<base::FilePath> file_path_optional =
      g_brave_browser_process->resource_component()->GetPath(id, version);
  if (!file_path_optional) {
    std::move(callback).Run({});
    return;
  }
  base::FilePath file_path = file_path_optional.value();

  VLOG(1) << "Loading file resource from " << file_path << " for component id "
          << id;

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(
          [](base::FilePath path,
             scoped_refptr<base::SequencedTaskRunner> file_task_runner) {
            std::unique_ptr<base::File, base::OnTaskRunnerDeleter> file(
                new base::File(path, base::File::Flags::FLAG_OPEN |
                                         base::File::Flags::FLAG_READ),
                base::OnTaskRunnerDeleter(file_task_runner));
            return file;
          },
          std::move(file_path), file_task_runner_),
      base::BindOnce(&AdsServiceImpl::OnLoadFileResource, AsWeakPtr(),
                     std::move(callback)));
}

std::string AdsServiceImpl::LoadDataResource(const std::string& name) {
  const int id = GetDataResourceId(name);

  std::string data_resource;

  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    data_resource = resource_bundle.LoadDataResourceString(id);
  } else {
    data_resource =
        static_cast<std::string>(resource_bundle.GetRawDataResource(id));
  }

  return data_resource;
}

void AdsServiceImpl::GetScheduledCaptcha(
    const std::string& payment_id,
    ads::GetScheduledCaptchaCallback callback) {
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  adaptive_captcha_service_->GetScheduledCaptcha(payment_id,
                                                 std::move(callback));
#endif
}

void AdsServiceImpl::ShowScheduledCaptchaNotification(
    const std::string& payment_id,
    const std::string& captcha_id,
    const bool should_show_tooltip_notification) {
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  PrefService* pref_service = profile_->GetPrefs();
  if (should_show_tooltip_notification) {
    if (pref_service->GetBoolean(
            brave_adaptive_captcha::prefs::kScheduledCaptchaPaused)) {
      VLOG(0) << "Ads paused; support intervention required";
      return;
    }

// TODO(sergz): made a guard to prevent a potential crash, but need to
// check as we could have the guard in the higher level for Android
#if !BUILDFLAG(IS_ANDROID)
    const int snooze_count = pref_service->GetInteger(
        brave_adaptive_captcha::prefs::kScheduledCaptchaSnoozeCount);

    DCHECK(ads_tooltips_delegate_);

    ads_tooltips_delegate_->ShowCaptchaTooltip(
        payment_id, captcha_id, snooze_count == 0,
        base::BindOnce(&AdsServiceImpl::ShowScheduledCaptcha, AsWeakPtr()),
        base::BindOnce(&AdsServiceImpl::SnoozeScheduledCaptcha, AsWeakPtr()));
#endif
  } else {
    ShowScheduledCaptcha(payment_id, captcha_id);
  }
#endif
}

void AdsServiceImpl::ClearScheduledCaptcha() {
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  adaptive_captcha_service_->ClearScheduledCaptcha();
#endif
}

void AdsServiceImpl::RunDBTransaction(
    ads::mojom::DBTransactionInfoPtr transaction,
    ads::RunDBTransactionCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&RunDBTransactionOnFileTaskRunner, std::move(transaction),
                     database_.get()),
      std::move(callback));
}

void AdsServiceImpl::RecordP2AEvent(const std::string& /*name*/,
                                    base::Value::List value) {
  for (const auto& item : value) {
    DCHECK(item.is_string());
    RecordInWeeklyStorageAndEmitP2AHistogramAnswer(profile_->GetPrefs(),
                                                   item.GetString());
  }
}

void AdsServiceImpl::LogTrainingInstance(
    std::vector<brave_federated::mojom::CovariateInfoPtr> training_instance) {
  if (!notification_ad_timing_data_store_) {
    return;
  }

  notification_ad_timing_data_store_->AddTrainingInstance(
      std::move(training_instance),
      base::BindOnce(&AdsServiceImpl::OnLogTrainingInstance, AsWeakPtr()));
}

bool AdsServiceImpl::GetBooleanPref(const std::string& path) const {
  return profile_->GetPrefs()->GetBoolean(path);
}

void AdsServiceImpl::SetBooleanPref(const std::string& path, const bool value) {
  profile_->GetPrefs()->SetBoolean(path, value);
  NotifyPrefChanged(path);
}

int AdsServiceImpl::GetIntegerPref(const std::string& path) const {
  return profile_->GetPrefs()->GetInteger(path);
}

void AdsServiceImpl::SetIntegerPref(const std::string& path, const int value) {
  profile_->GetPrefs()->SetInteger(path, value);
  NotifyPrefChanged(path);
}

double AdsServiceImpl::GetDoublePref(const std::string& path) const {
  return profile_->GetPrefs()->GetDouble(path);
}

void AdsServiceImpl::SetDoublePref(const std::string& path,
                                   const double value) {
  profile_->GetPrefs()->SetDouble(path, value);
  NotifyPrefChanged(path);
}

std::string AdsServiceImpl::GetStringPref(const std::string& path) const {
  return profile_->GetPrefs()->GetString(path);
}

void AdsServiceImpl::SetStringPref(const std::string& path,
                                   const std::string& value) {
  profile_->GetPrefs()->SetString(path, value);
  NotifyPrefChanged(path);
}

int64_t AdsServiceImpl::GetInt64Pref(const std::string& path) const {
  const std::string integer_as_string = profile_->GetPrefs()->GetString(path);
  DCHECK(!integer_as_string.empty());

  int64_t integer;
  base::StringToInt64(integer_as_string, &integer);
  return integer;
}

void AdsServiceImpl::SetInt64Pref(const std::string& path,
                                  const int64_t value) {
  profile_->GetPrefs()->SetInt64(path, value);
  NotifyPrefChanged(path);
}

uint64_t AdsServiceImpl::GetUint64Pref(const std::string& path) const {
  const std::string integer_as_string = profile_->GetPrefs()->GetString(path);
  DCHECK(!integer_as_string.empty());

  uint64_t integer;
  base::StringToUint64(integer_as_string, &integer);
  return integer;
}

void AdsServiceImpl::SetUint64Pref(const std::string& path,
                                   const uint64_t value) {
  profile_->GetPrefs()->SetUint64(path, value);
  NotifyPrefChanged(path);
}

base::Time AdsServiceImpl::GetTimePref(const std::string& path) const {
  return profile_->GetPrefs()->GetTime(path);
}

void AdsServiceImpl::SetTimePref(const std::string& path,
                                 const base::Time value) {
  profile_->GetPrefs()->SetTime(path, value);
  NotifyPrefChanged(path);
}

absl::optional<base::Value::Dict> AdsServiceImpl::GetDictPref(
    const std::string& path) const {
  return profile_->GetPrefs()->GetValueDict(path).Clone();
}

void AdsServiceImpl::SetDictPref(const std::string& path,
                                 base::Value::Dict value) {
  profile_->GetPrefs()->SetDict(path, std::move(value));
  NotifyPrefChanged(path);
}

absl::optional<base::Value::List> AdsServiceImpl::GetListPref(
    const std::string& path) const {
  return profile_->GetPrefs()->GetValueList(path).Clone();
}

void AdsServiceImpl::SetListPref(const std::string& path,
                                 base::Value::List value) {
  profile_->GetPrefs()->SetList(path, std::move(value));
  NotifyPrefChanged(path);
}

void AdsServiceImpl::ClearPref(const std::string& path) {
  profile_->GetPrefs()->ClearPref(path);
  NotifyPrefChanged(path);
}

bool AdsServiceImpl::HasPrefPath(const std::string& path) const {
  return profile_->GetPrefs()->HasPrefPath(path);
}

void AdsServiceImpl::Log(const char* file,
                         const int line,
                         const int verbose_level,
                         const std::string& message) {
  WriteDiagnosticLog(file, line, verbose_level, message);

  const int vlog_level = ::logging::GetVlogLevelHelper(file, strlen(file));
  if (verbose_level <= vlog_level) {
    ::logging::LogMessage(file, line, -verbose_level).stream() << message;
  }
}

void AdsServiceImpl::OnBrowserDidEnterForeground() {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnBrowserDidEnterForeground();
}

void AdsServiceImpl::OnBrowserDidEnterBackground() {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnBrowserDidEnterBackground();
}

void AdsServiceImpl::OnDidUpdateResourceComponent(const std::string& id) {
  if (!IsBatAdsBound()) {
    return;
  }

  bat_ads_->OnDidUpdateResourceComponent(id);
}

void AdsServiceImpl::PrefetchNewTabPageAd() {
  if (!did_cleanup_on_first_run_) {
    // Postpone prefetching of a new tab page ad at a later time during
    // CleanUpOnFirstRun() call.
    prefetch_new_tab_page_ad_on_first_run_ = true;
    return;
  }

  if (!IsBatAdsBound()) {
    return;
  }

  // The previous prefetched new tab page ad is available. No need to do
  // prefetch again.
  if (prefetched_new_tab_page_ad_) {
    return;
  }

  if (need_purge_orphaned_new_tab_page_ad_events_) {
    need_purge_orphaned_new_tab_page_ad_events_ = false;
    PurgeOrphanedAdEventsForType(
        ads::mojom::AdType::kNewTabPageAd,
        base::BindOnce(&AdsServiceImpl::OnPurgeOrphanedNewTabPageAdEvents,
                       AsWeakPtr()));
  } else {
    bat_ads_->MaybeServeNewTabPageAd(
        base::BindOnce(&AdsServiceImpl::OnPrefetchNewTabPageAd, AsWeakPtr()));
  }
}

void AdsServiceImpl::OnPrefetchNewTabPageAd(
    absl::optional<base::Value::Dict> dict) {
  if (!dict) {
    VLOG(0) << "Failed to prefetch new tab page ad";
    return;
  }

  DCHECK(!prefetched_new_tab_page_ad_);
  prefetched_new_tab_page_ad_ = ads::NewTabPageAdFromValue(*dict);
}

void AdsServiceImpl::OnURLRequest(
    SimpleURLLoaderList::iterator url_loader_iter,
    ads::UrlRequestCallback callback,
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
    scoped_refptr<net::HttpResponseHeaders> headers_list =
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

  if (response_code == kHttpUpgradeRequiredStatusCode &&
      !needs_browser_upgrade_to_serve_ads_) {
    needs_browser_upgrade_to_serve_ads_ = true;
    for (AdsServiceObserver& observer : observers_) {
      observer.OnNeedsBrowserUpgradeToServeAds();
    }
  }

  ads::mojom::UrlResponseInfo url_response;
  url_response.url = url_loader->GetFinalURL();
  url_response.status_code = response_code;
  url_response.body = response_body ? *response_body : "";
  url_response.headers = headers;

  std::move(callback).Run(url_response);
}

void AdsServiceImpl::OnPurgeOrphanedNewTabPageAdEvents(const bool success) {
  if (!success) {
    VLOG(0) << "Failed to purge orphaned ad events for new tab page ads";
    return;
  }

  PrefetchNewTabPageAd();
}

void AdsServiceImpl::OnLoad(ads::LoadCallback callback,
                            const std::string& value) {
  const bool success = !value.empty();
  std::move(callback).Run(success, value);
}

void AdsServiceImpl::OnLoadFileResource(
    ads::LoadFileCallback callback,
    std::unique_ptr<base::File, base::OnTaskRunnerDeleter> file) {
  DCHECK(file);
  std::move(callback).Run(std::move(*file));
}

void AdsServiceImpl::MigratePrefs() {
  is_upgrading_from_pre_brave_ads_build_ = IsUpgradingFromPreBraveAdsBuild();
  if (is_upgrading_from_pre_brave_ads_build_) {
    VLOG(1) << "Migrating ads preferences from pre Brave Ads build";

    // Force migration of preferences from version 1 if
    // |is_upgrading_from_pre_brave_ads_build_| is set to true to fix
    // "https://github.com/brave/brave-browser/issues/5434"
    SetIntegerPref(prefs::kVersion, 1);
  } else {
    VLOG(1) << "Migrating ads preferences";
  }

  auto source_version = GetIntegerPref(prefs::kVersion);
  auto dest_version = prefs::kCurrentVersionNumber;

  if (!MigratePrefs(source_version, dest_version, true)) {
    // Migration dry-run failed, so do not migrate preferences
    VLOG(0) << "Failed to migrate ads preferences from version "
            << source_version << " to " << dest_version;

    return;
  }

  MigratePrefs(source_version, dest_version);
}

bool AdsServiceImpl::MigratePrefs(const int source_version,
                                  const int dest_version,
                                  const bool is_dry_run) {
  DCHECK(source_version >= 1) << "Invalid migration path";
  DCHECK(source_version <= dest_version) << "Invalid migration path";

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

  static base::NoDestructor<
      base::flat_map<std::pair<int, int>, void (AdsServiceImpl::*)()>>
      mappings({// {{from version, to version}, function}
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
    auto mapping = mappings->find(std::make_pair(from_version, to_version));
    if (mapping == mappings->end()) {
      // Migration path does not exist. It is highly recommended to perform a
      // dry-run before migrating preferences
      return false;
    }

    if (!is_dry_run) {
      VLOG(1) << "Migrating ads preferences from mapping version "
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

    VLOG(1) << "Successfully migrated Ads preferences from version "
            << source_version << " to " << dest_version;
  }

  return true;
}

void AdsServiceImpl::MigratePrefsVersion1To2() {
  // Intentionally empty as we no longer need to migrate ads per day due to
  // deprecation of prefs::kAdsPerDay
}

void AdsServiceImpl::MigratePrefsVersion2To3() {
  const auto locale = GetLocale();
  const auto country_code = brave_l10n::GetCountryCode(locale);

  // Disable ads if upgrading from a pre brave ads build due to a bug where ads
  // were always enabled
  DisableAdsIfUpgradingFromPreBraveAdsBuild();

  // Disable ads for unsupported legacy country_codes due to a bug where ads
  // were enabled even if the users country code was not supported
  const std::vector<std::string> legacy_country_codes = {
      "US",  // United States of America
      "CA",  // Canada
      "GB",  // United Kingdom (Great Britain and Northern Ireland)
      "DE",  // Germany
      "FR"   // France
  };

  DisableAdsForUnsupportedCountryCodes(country_code, legacy_country_codes);
}

void AdsServiceImpl::MigratePrefsVersion3To4() {
  const auto locale = GetLocale();
  const auto country_code = brave_l10n::GetCountryCode(locale);

  // Disable ads for unsupported legacy country codes due to a bug where ads
  // were enabled even if the users country code was not supported
  const std::vector<std::string> legacy_country_codes = {
      "US",  // United States of America
      "CA",  // Canada
      "GB",  // United Kingdom (Great Britain and Northern Ireland)
      "DE",  // Germany
      "FR",  // France
      "AU",  // Australia
      "NZ",  // New Zealand
      "IE"   // Ireland
  };

  DisableAdsForUnsupportedCountryCodes(country_code, legacy_country_codes);
}

void AdsServiceImpl::MigratePrefsVersion4To5() {
  const auto locale = GetLocale();
  const auto country_code = brave_l10n::GetCountryCode(locale);

  // Disable ads for unsupported legacy country codes due to a bug where ads
  // were enabled even if the users country code was not supported
  const std::vector<std::string> legacy_country_codes = {
      "US",  // United States of America
      "CA",  // Canada
      "GB",  // United Kingdom (Great Britain and Northern Ireland)
      "DE",  // Germany
      "FR",  // France
      "AU",  // Australia
      "NZ",  // New Zealand
      "IE",  // Ireland
      "AR",  // Argentina
      "AT",  // Austria
      "BR",  // Brazil
      "CH",  // Switzerland
      "CL",  // Chile
      "CO",  // Colombia
      "DK",  // Denmark
      "EC",  // Ecuador
      "IL",  // Israel
      "IN",  // India
      "IT",  // Italy
      "JP",  // Japan
      "KR",  // Korea
      "MX",  // Mexico
      "NL",  // Netherlands
      "PE",  // Peru
      "PH",  // Philippines
      "PL",  // Poland
      "SE",  // Sweden
      "SG",  // Singapore
      "VE",  // Venezuela
      "ZA"   // South Africa
  };

  DisableAdsForUnsupportedCountryCodes(country_code, legacy_country_codes);
}

void AdsServiceImpl::MigratePrefsVersion5To6() {
  // Intentionally empty as we no longer need to migrate ads per day due to
  // deprecation of prefs::kAdsPerDay
}

void AdsServiceImpl::MigratePrefsVersion6To7() {
  // Disable ads for newly supported country codes due to a bug where ads were
  // enabled even if the users country code was not supported

  const auto locale = GetLocale();
  const auto country_code = brave_l10n::GetCountryCode(locale);

  const std::vector<std::string> legacy_country_codes = {
      "US",  // United States of America
      "CA",  // Canada
      "GB",  // United Kingdom (Great Britain and Northern Ireland)
      "DE",  // Germany
      "FR",  // France
      "AU",  // Australia
      "NZ",  // New Zealand
      "IE",  // Ireland
      "AR",  // Argentina
      "AT",  // Austria
      "BR",  // Brazil
      "CH",  // Switzerland
      "CL",  // Chile
      "CO",  // Colombia
      "DK",  // Denmark
      "EC",  // Ecuador
      "IL",  // Israel
      "IN",  // India
      "IT",  // Italy
      "JP",  // Japan
      "KR",  // Korea
      "MX",  // Mexico
      "NL",  // Netherlands
      "PE",  // Peru
      "PH",  // Philippines
      "PL",  // Poland
      "SE",  // Sweden
      "SG",  // Singapore
      "VE",  // Venezuela
      "ZA",  // South Africa
      "KY"   // Cayman Islands
  };

  const bool is_a_legacy_country_code =
      base::Contains(legacy_country_codes, country_code);

  if (is_a_legacy_country_code) {
    // Do not disable Brave Ads for legacy country codes introduced before
    // version 1.3.x
    return;
  }

  const int last_schema_version =
      GetIntegerPref(prefs::kSupportedCountryCodesLastSchemaVersion);

  if (last_schema_version >= 4) {
    // Do not disable Brave Ads if |kSupportedCountryCodesLastSchemaVersion|
    // is newer than or equal to schema version 4. This can occur if a user is
    // upgrading from an older version of 1.3.x or above
    return;
  }

  SetEnabled(/* is_enabled */ false);
}

void AdsServiceImpl::MigratePrefsVersion7To8() {
  const bool rewards_enabled = GetBooleanPref(brave_rewards::prefs::kEnabled);
  if (!rewards_enabled) {
    SetEnabled(/* is_enabled */ false);
  }
}

void AdsServiceImpl::MigratePrefsVersion8To9() {
  // Intentionally empty as we no longer need to migrate ads per day due to
  // deprecation of prefs::kAdsPerDay
}

void AdsServiceImpl::MigratePrefsVersion9To10() {
  if (!HasPrefPath(ads::prefs::kMaximumNotificationAdsPerHour)) {
    return;
  }

  const int64_t ads_per_hour =
      GetInt64Pref(ads::prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == -1 || ads_per_hour == 2) {
    // The user did not change the ads per hour setting from the legacy default
    // value of 2 so we should clear the preference to transition to
    // |kDefaultNotificationAdsPerHour|
    profile_->GetPrefs()->ClearPref(ads::prefs::kMaximumNotificationAdsPerHour);
  }
}

void AdsServiceImpl::MigratePrefsVersion10To11() {
  if (!HasPrefPath(ads::prefs::kMaximumNotificationAdsPerHour)) {
    return;
  }

  const int64_t ads_per_hour =
      GetInt64Pref(ads::prefs::kMaximumNotificationAdsPerHour);
  if (ads_per_hour == 0 || ads_per_hour == -1) {
    // Clear the ads per hour preference to transition to
    // |kDefaultNotificationAdsPerHour|
    profile_->GetPrefs()->ClearPref(ads::prefs::kMaximumNotificationAdsPerHour);
  }
}

void AdsServiceImpl::MigratePrefsVersion11To12() {
  std::string value;

  if (base::ReadFileToString(base_path_.AppendASCII("confirmations.json"),
                             &value)) {
    const auto hash = static_cast<uint64_t>(base::PersistentHash(value));
    SetUint64Pref(ads::prefs::kConfirmationsHash, hash);
  }

  if (base::ReadFileToString(base_path_.AppendASCII("client.json"), &value)) {
    const auto hash = static_cast<uint64_t>(base::PersistentHash(value));
    SetUint64Pref(ads::prefs::kClientHash, hash);
  }
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
  return GetBooleanPref(ads::prefs::kEnabled) &&
         !HasPrefPath(ads::prefs::kIdleTimeThreshold) &&
         !HasPrefPath(prefs::kVersion) && !first_run::IsChromeFirstRun();
#else
  return false;
#endif
}

void AdsServiceImpl::DisableAdsIfUpgradingFromPreBraveAdsBuild() {
  if (!is_upgrading_from_pre_brave_ads_build_) {
    return;
  }

  SetEnabled(/* is_enabled */ false);
}

void AdsServiceImpl::DisableAdsForUnsupportedCountryCodes(
    const std::string& country_code,
    const std::vector<std::string>& supported_country_codes) {
  if (base::Contains(supported_country_codes, country_code)) {
    return;
  }

  SetEnabled(/* is_enabled */ false);
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

bool AdsServiceImpl::ShouldShowOnboardingNotification() {
  const bool should_show_my_first_notification_ad =
      GetBooleanPref(prefs::kShouldShowOnboardingNotification);
  return IsEnabled() && CanShowNotificationAds() &&
         should_show_my_first_notification_ad;
}

void AdsServiceImpl::OnBrowsingHistorySearchComplete(
    ads::GetBrowsingHistoryCallback callback,
    history::QueryResults results) {
  std::vector<GURL> history;
  for (const auto& result : results) {
    history.push_back(result.url().GetWithEmptyPath());
  }

  std::sort(history.begin(), history.end());
  history.erase(std::unique(history.begin(), history.end()), history.end());

  std::move(callback).Run(history);
}

void AdsServiceImpl::OnLogTrainingInstance(bool success) {
  if (!success) {
    VLOG(1) << "Failed to log training covariates";
    return;
  }

  VLOG(1) << "Successfully logged training covariates";
}

void AdsServiceImpl::WriteDiagnosticLog(const std::string& file,
                                        const int line,
                                        const int verbose_level,
                                        const std::string& message) {
  rewards_service_->WriteDiagnosticLog(file, line, verbose_level, message);
}

}  // namespace brave_ads
