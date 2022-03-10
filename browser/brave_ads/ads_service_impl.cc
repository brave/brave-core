/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_impl.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/cxx17_backports.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/guid.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/metrics/field_trial_params.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/resources/grit/bat_ads_resources.h"
#include "bat/ads/statement_info.h"
#include "brave/browser/brave_ads/notification_helper/notification_helper.h"
#include "brave/browser/brave_ads/notifications/ad_notification_platform_bridge.h"
#include "brave/browser/brave_ads/service_sandbox_type.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/brave_channel_info.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_p2a.h"
#include "brave/components/brave_ads/browser/ads_storage_cleanup.h"
#include "brave/components/brave_ads/browser/frequency_capping_helper.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/common/switches.h"
#include "brave/components/brave_federated/data_store_service.h"
#include "brave/components/brave_federated/data_stores/ad_notification_timing_data_store.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/rpill/common/rpill.h"
#include "brave/components/services/bat_ads/public/cpp/ads_client_mojo_bridge.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_paths.h"
#if !defined(OS_ANDROID)
#include "chrome/browser/fullscreen.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#endif
#include "chrome/browser/first_run/first_run.h"
#include "chrome/common/chrome_constants.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"

#if defined(OS_ANDROID)
#include "brave/browser/notifications/brave_notification_platform_bridge_helper_android.h"
#include "chrome/browser/android/service_tab_launcher.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "content/public/browser/page_navigator.h"
#endif

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_ads/browser/ads_tooltips_delegate.h"
#endif

using brave_rewards::RewardsNotificationService;

namespace brave_ads {

namespace {

constexpr char kTrue[] = "true";

const unsigned int kRetriesCountOnNetworkChange = 1;

constexpr char kAdNotificationUrlPrefix[] = "https://www.brave.com/ads/?";

const base::Feature kAdServing{"AdServing", base::FEATURE_ENABLED_BY_DEFAULT};

int GetSchemaResourceId(const std::string& name) {
  if (name == ads::g_catalog_schema_resource_id) {
    return IDR_ADS_CATALOG_SCHEMA;
  }

  NOTREACHED();

  return 0;
}

std::string URLMethodToRequestType(ads::mojom::UrlRequestMethod method) {
  switch (method) {
    case ads::mojom::UrlRequestMethod::kGet: {
      return "GET";
    }

    case ads::mojom::UrlRequestMethod::kPost: {
      return "POST";
    }

    case ads::mojom::UrlRequestMethod::kPut: {
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

bool ResetOnFileTaskRunner(const base::FilePath& path) {
  bool recursive;

  base::File::Info file_info;
  if (base::GetFileInfo(path, &file_info)) {
    recursive = file_info.is_directory;
  } else {
    recursive = false;
  }

  if (recursive)
    return base::DeletePathRecursively(path);
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

}  // namespace

AdsServiceImpl::AdsServiceImpl(
    Profile* profile,
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
    brave_adaptive_captcha::BraveAdaptiveCaptchaService*
        adaptive_captcha_service,
    std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate,
#endif
    history::HistoryService* history_service,
    brave_federated::AsyncDataStore<
        brave_federated::AdNotificationTimingDataStore,
        brave_federated::AdNotificationTimingTaskLog>*
        ad_notification_timing_data_store)
    : profile_(profile),
      history_service_(history_service),
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
      adaptive_captcha_service_(adaptive_captcha_service),
      ads_tooltips_delegate_(std::move(ads_tooltips_delegate)),
#endif
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      base_path_(profile_->GetPath().AppendASCII("ads_service")),
      last_idle_state_(ui::IdleState::IDLE_STATE_ACTIVE),
      last_idle_time_(0),
      display_service_(NotificationDisplayService::GetForProfile(profile_)),
      rewards_service_(
          brave_rewards::RewardsServiceFactory::GetForProfile(profile_)),
      ad_notification_timing_data_store_(ad_notification_timing_data_store),
      bat_ads_client_receiver_(new bat_ads::AdsClientMojoBridge(this)) {
  DCHECK(profile_);
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  DCHECK(adaptive_captcha_service_);
#endif
  DCHECK(history_service_);
  DCHECK(brave::IsRegularProfile(profile_));

  MigratePrefs();

  MaybeInitialize();
}

AdsServiceImpl::~AdsServiceImpl() = default;

void AdsServiceImpl::OnResourceComponentUpdated(const std::string& id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnResourceComponentUpdated(id);
}

bool AdsServiceImpl::IsSupportedLocale() const {
  const std::string locale = GetLocale();
  return ads::IsSupportedLocale(locale);
}

void AdsServiceImpl::SetEnabled(const bool is_enabled) {
  SetBooleanPref(ads::prefs::kEnabled, is_enabled);
}

void AdsServiceImpl::SetAllowConversionTracking(const bool should_allow) {
  SetBooleanPref(ads::prefs::kShouldAllowConversionTracking, should_allow);
}

void AdsServiceImpl::SetAdsPerHour(const int64_t ads_per_hour) {
  DCHECK(ads_per_hour >= ads::kMinimumAdNotificationsPerHour &&
         ads_per_hour <= ads::kMaximumAdNotificationsPerHour);
  SetInt64Pref(ads::prefs::kAdsPerHour, ads_per_hour);
}

void AdsServiceImpl::SetAdsSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  SetStringPref(ads::prefs::kAdsSubdivisionTargetingCode,
                subdivision_targeting_code);
}

void AdsServiceImpl::SetAutoDetectedAdsSubdivisionTargetingCode(
    const std::string& subdivision_targeting_code) {
  SetStringPref(ads::prefs::kAutoDetectedAdsSubdivisionTargetingCode,
                subdivision_targeting_code);
}

void AdsServiceImpl::ChangeLocale(const std::string& locale) {
  if (!connected()) {
    return;
  }

  RegisterResourceComponentsForLocale(locale);

  bat_ads_->ChangeLocale(locale);
}

void AdsServiceImpl::OnPrefChanged(const std::string& path) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnPrefChanged(path);
}

void AdsServiceImpl::OnHtmlLoaded(const SessionID& tab_id,
                                  const std::vector<GURL>& redirect_chain,
                                  const std::string& html) {
  if (!connected()) {
    return;
  }

  std::vector<std::string> redirect_chain_as_strings;
  for (const auto& url : redirect_chain) {
    redirect_chain_as_strings.push_back(url.spec());
  }

  bat_ads_->OnHtmlLoaded(tab_id.id(), redirect_chain_as_strings, html);
}

void AdsServiceImpl::OnTextLoaded(const SessionID& tab_id,
                                  const std::vector<GURL>& redirect_chain,
                                  const std::string& text) {
  if (!connected()) {
    return;
  }

  std::vector<std::string> redirect_chain_as_strings;
  for (const auto& url : redirect_chain) {
    redirect_chain_as_strings.push_back(url.spec());
  }

  bat_ads_->OnTextLoaded(tab_id.id(), redirect_chain_as_strings, text);
}

void AdsServiceImpl::OnUserGesture(const int32_t page_transition_type) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnUserGesture(page_transition_type);
}

void AdsServiceImpl::OnMediaStart(const SessionID& tab_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnMediaPlaying(tab_id.id());
}

void AdsServiceImpl::OnMediaStop(const SessionID& tab_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnMediaStopped(tab_id.id());
}

void AdsServiceImpl::OnTabUpdated(const SessionID& tab_id,
                                  const GURL& url,
                                  const bool is_active,
                                  const bool is_browser_active) {
  if (!connected()) {
    return;
  }

  const bool is_incognito = !brave::IsRegularProfile(profile_);

  bat_ads_->OnTabUpdated(tab_id.id(), url.spec(), is_active, is_browser_active,
                         is_incognito);
}

void AdsServiceImpl::OnTabClosed(const SessionID& tab_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnTabClosed(tab_id.id());
}

void AdsServiceImpl::OnWalletUpdated() {
  if (!connected()) {
    return;
  }

  rewards_service_->GetBraveWallet(
      base::BindOnce(&AdsServiceImpl::OnGetBraveWallet, AsWeakPtr()));
}

void AdsServiceImpl::OnGetBraveWallet(ledger::type::BraveWalletPtr wallet) {
  if (!connected() || !wallet) {
    return;
  }

  bat_ads_->OnWalletUpdated(wallet->payment_id,
                            base::Base64Encode(wallet->recovery_seed));
}

void AdsServiceImpl::GetAdsHistory(const double from_timestamp,
                                   const double to_timestamp,
                                   OnGetAdsHistoryCallback callback) {
  if (!connected()) {
    return;
  }

  bat_ads_->GetAdsHistory(from_timestamp, to_timestamp,
                          base::BindOnce(&AdsServiceImpl::OnGetAdsHistory,
                                         AsWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::GetAccountStatement(GetAccountStatementCallback callback) {
  if (!connected()) {
    std::move(callback).Run(/* success */ false, 0, 0, 0.0, 0.0);
    return;
  }

  bat_ads_->GetAccountStatement(
      base::BindOnce(&AdsServiceImpl::OnGetAccountStatement, AsWeakPtr(),
                     std::move(callback)));
}

void AdsServiceImpl::GetAdDiagnostics(GetAdDiagnosticsCallback callback) {
  if (!connected()) {
    std::move(callback).Run(/* success */ false, "");
    return;
  }

  bat_ads_->GetAdDiagnostics(base::BindOnce(&AdsServiceImpl::OnGetAdDiagnostics,
                                            AsWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::ToggleAdThumbUp(const std::string& json,
                                     OnToggleAdThumbUpCallback callback) {
  if (!connected()) {
    return;
  }

  bat_ads_->ToggleAdThumbUp(
      json, base::BindOnce(&AdsServiceImpl::OnToggleAdThumbUp, AsWeakPtr(),
                           std::move(callback)));
}

void AdsServiceImpl::ToggleAdThumbDown(const std::string& json,
                                       OnToggleAdThumbDownCallback callback) {
  if (!connected()) {
    return;
  }

  bat_ads_->ToggleAdThumbDown(
      json, base::BindOnce(&AdsServiceImpl::OnToggleAdThumbDown, AsWeakPtr(),
                           std::move(callback)));
}

void AdsServiceImpl::ToggleAdOptIn(const std::string& category,
                                   const int action,
                                   OnToggleAdOptInCallback callback) {
  if (!connected()) {
    return;
  }

  bat_ads_->ToggleAdOptIn(category, action,
                          base::BindOnce(&AdsServiceImpl::OnToggleAdOptIn,
                                         AsWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::ToggleAdOptOut(const std::string& category,
                                    const int action,
                                    OnToggleAdOptOutCallback callback) {
  if (!connected()) {
    return;
  }

  bat_ads_->ToggleAdOptOut(category, action,
                           base::BindOnce(&AdsServiceImpl::OnToggleAdOptOut,
                                          AsWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::ToggleSavedAd(const std::string& json,
                                   OnToggleSavedAdCallback callback) {
  if (!connected()) {
    return;
  }

  bat_ads_->ToggleSavedAd(
      json, base::BindOnce(&AdsServiceImpl::OnToggleSavedAd, AsWeakPtr(),
                           std::move(callback)));
}

void AdsServiceImpl::ToggleFlaggedAd(const std::string& json,
                                     OnToggleFlaggedAdCallback callback) {
  if (!connected()) {
    return;
  }

  bat_ads_->ToggleFlaggedAd(
      json, base::BindOnce(&AdsServiceImpl::OnToggleFlaggedAd, AsWeakPtr(),
                           std::move(callback)));
}

bool AdsServiceImpl::IsEnabled() const {
  return GetBooleanPref(ads::prefs::kEnabled);
}

bool AdsServiceImpl::IsBraveNewsEnabled() const {
  return base::FeatureList::IsEnabled(
             brave_today::features::kBraveNewsFeature) &&
         GetBooleanPref(brave_news::prefs::kBraveTodayOptedIn) &&
         GetBooleanPref(brave_news::prefs::kNewTabPageShowToday);
}

bool AdsServiceImpl::ShouldStart() const {
  return IsEnabled() || IsBraveNewsEnabled();
}

int64_t AdsServiceImpl::GetAdsPerHour() const {
  int64_t ads_per_hour = GetInt64Pref(ads::prefs::kAdsPerHour);
  if (ads_per_hour == -1) {
    ads_per_hour = base::GetFieldTrialParamByFeatureAsInt(
        kAdServing, "default_ad_notifications_per_hour",
        ads::kDefaultAdNotificationsPerHour);
  }

  return base::clamp(ads_per_hour,
                     static_cast<int64_t>(ads::kMinimumAdNotificationsPerHour),
                     static_cast<int64_t>(ads::kMaximumAdNotificationsPerHour));
}

bool AdsServiceImpl::ShouldAllowAdsSubdivisionTargeting() const {
  return GetBooleanPref(ads::prefs::kShouldAllowAdsSubdivisionTargeting);
}

std::string AdsServiceImpl::GetAdsSubdivisionTargetingCode() const {
  return GetStringPref(ads::prefs::kAdsSubdivisionTargetingCode);
}

std::string AdsServiceImpl::GetAutoDetectedAdsSubdivisionTargetingCode() const {
  return GetStringPref(ads::prefs::kAutoDetectedAdsSubdivisionTargetingCode);
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceImpl::Shutdown() {
  is_initialized_ = false;

  BackgroundHelper::GetInstance()->RemoveObserver(this);

  g_brave_browser_process->resource_component()->RemoveObserver(this);

  url_loaders_.clear();

  idle_poll_timer_.Stop();

  bat_ads_.reset();
  bat_ads_client_receiver_.reset();
  bat_ads_service_.reset();

  const bool success =
      file_task_runner_->DeleteSoon(FROM_HERE, database_.release());
  VLOG_IF(1, !success) << "Failed to release database";
}

///////////////////////////////////////////////////////////////////////////////

bool MigrateConfirmationsStateOnFileTaskRunner(const base::FilePath& path) {
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
  }

  if (base::PathExists(rewards_service_base_path)) {
    VLOG(1) << "Deleting " << rewards_service_base_path.value();

    if (!base::DeleteFile(rewards_service_base_path)) {
      VLOG(0) << "Failed to delete " << rewards_service_base_path.value();
    }
  }

  return true;
}

void AdsServiceImpl::MaybeInitialize() {
  const base::FilePath path = profile_->GetPath();

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&MigrateConfirmationsStateOnFileTaskRunner, path),
      base::BindOnce(&AdsServiceImpl::OnMigrateConfirmationsState,
                     AsWeakPtr()));
}

void AdsServiceImpl::OnMigrateConfirmationsState(const bool success) {
  if (!success) {
    VLOG(0) << "Failed to migrate confirmations state";
    return;
  }

  VLOG(1) << "Successfully migrated confirmations state";

  Initialize();
}

void AdsServiceImpl::Initialize() {
  profile_pref_change_registrar_.Init(profile_->GetPrefs());

  profile_pref_change_registrar_.Add(
      ads::prefs::kEnabled, base::BindRepeating(&AdsServiceImpl::OnPrefsChanged,
                                                base::Unretained(this)));

  profile_pref_change_registrar_.Add(
      ads::prefs::kIdleTimeThreshold,
      base::BindRepeating(&AdsServiceImpl::OnPrefsChanged,
                          base::Unretained(this)));

  profile_pref_change_registrar_.Add(
      brave_rewards::prefs::kWalletBrave,
      base::BindRepeating(&AdsServiceImpl::OnPrefsChanged,
                          base::Unretained(this)));

  profile_pref_change_registrar_.Add(
      brave_news::prefs::kBraveTodayOptedIn,
      base::BindRepeating(&AdsServiceImpl::OnPrefsChanged,
                          base::Unretained(this)));

  profile_pref_change_registrar_.Add(
      brave_news::prefs::kNewTabPageShowToday,
      base::BindRepeating(&AdsServiceImpl::OnPrefsChanged,
                          base::Unretained(this)));

  MaybeStart(false);
}

void AdsServiceImpl::OnCreate() {
  if (!connected()) {
    return;
  }

  auto callback = base::BindOnce(&AdsServiceImpl::OnInitialize, AsWeakPtr());
  bat_ads_->Initialize(base::BindOnce(std::move(callback)));
}

void AdsServiceImpl::OnInitialize(const bool success) {
  if (!success) {
    VLOG(0) << "Failed to initialize ads";

    is_initialized_ = false;
    return;
  }

  is_initialized_ = true;

  MaybeOpenNewTabWithAd();

  StartCheckIdleStateTimer();

  if (!deprecated_data_files_removed_) {
    deprecated_data_files_removed_ = true;
    file_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&RemoveDeprecatedAdsDataFiles, base_path_));
  }
}

void AdsServiceImpl::ShutdownBatAds() {
  if (!connected()) {
    return;
  }

  VLOG(1) << "Shutting down ads";

  bat_ads_->Shutdown(
      base::BindOnce(&AdsServiceImpl::OnShutdownBatAds, AsWeakPtr()));
}

void AdsServiceImpl::OnShutdownBatAds(const bool success) {
  DCHECK(is_initialized_);

  if (!success) {
    VLOG(0) << "Failed to shutdown ads";
    return;
  }

  Shutdown();

  VLOG(1) << "Successfully shutdown ads";
}

bool AdsServiceImpl::StartService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!connected());

  if (!bat_ads_service_.is_bound()) {
    content::ServiceProcessHost::Launch(
        bat_ads_service_.BindNewPipeAndPassReceiver(),
        content::ServiceProcessHost::Options()
            .WithDisplayName(IDS_SERVICE_BAT_ADS)
            .Pass());

    bat_ads_service_.set_disconnect_handler(
        base::BindOnce(&AdsServiceImpl::MaybeStart, AsWeakPtr(), true));
  }

  SetEnvironment();
  SetBuildChannel();
  UpdateIsDebugFlag();

  return true;
}

void AdsServiceImpl::MaybeStart(const bool should_restart) {
  if (!IsSupportedLocale()) {
    VLOG(1) << GetLocale() << " locale does not support ads";
    Shutdown();
    return;
  }

  if (!ShouldStart()) {
    Stop();
    return;
  }

  if (should_restart) {
    VLOG(1) << "Restarting ads service";
    Shutdown();
  }

  if (connected()) {
    return;
  }

  if (!StartService()) {
    VLOG(0) << "Failed to start ads service";
    return;
  }

  ++total_number_of_starts_;
  if (should_restart) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&AdsServiceImpl::Start, AsWeakPtr(),
                       total_number_of_starts_),
        base::Seconds(1));
  } else {
    Start(total_number_of_starts_);
  }
}

void AdsServiceImpl::Start(const uint32_t number_of_start) {
  DetectUncertainFuture(number_of_start);
}

void AdsServiceImpl::Stop() {
  ShutdownBatAds();
}

void AdsServiceImpl::ResetState() {
  VLOG(1) << "Resetting ads state";

  profile_->GetPrefs()->ClearPrefsWithPrefixSilently("brave.brave_ads");

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ResetOnFileTaskRunner, base_path_),
      base::BindOnce(&AdsServiceImpl::OnResetAllState, AsWeakPtr()));
}

void AdsServiceImpl::ResetAllState(const bool should_shutdown) {
  if (!should_shutdown || !connected()) {
    ResetState();
    return;
  }

  VLOG(1) << "Shutting down and resetting ads state";

  bat_ads_->Shutdown(
      base::BindOnce(&AdsServiceImpl::OnShutdownAndResetBatAds, AsWeakPtr()));
}

void AdsServiceImpl::OnShutdownAndResetBatAds(const bool success) {
  DCHECK(is_initialized_);

  if (!success) {
    VLOG(0) << "Failed to shutdown and reset ads state";
    return;
  }

  Shutdown();

  VLOG(1) << "Successfully shutdown ads";

  ResetState();
}

void AdsServiceImpl::OnResetAllState(const bool success) {
  if (!success) {
    VLOG(0) << "Failed to reset ads state";
    return;
  }

  VLOG(1) << "Successfully reset ads state";
}

void AdsServiceImpl::DetectUncertainFuture(const uint32_t number_of_start) {
  auto callback = base::BindOnce(&AdsServiceImpl::OnDetectUncertainFuture,
                                 AsWeakPtr(), number_of_start);
  brave_rpill::DetectUncertainFuture(base::BindOnce(std::move(callback)));
}

void AdsServiceImpl::OnDetectUncertainFuture(const uint32_t number_of_start,
                                             const bool is_uncertain_future) {
  ads::mojom::SysInfoPtr sys_info = ads::mojom::SysInfo::New();
  sys_info->is_uncertain_future = is_uncertain_future;
  bat_ads_service_->SetSysInfo(std::move(sys_info), base::NullCallback());

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

  // Check if another start was initiated.
  if (number_of_start != total_number_of_starts_) {
    VLOG(1) << "Do not proceed with current ads service init as another ads "
               "service start is in progress";
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
      bat_ads_client_receiver_.BindNewEndpointAndPassRemote(),
      bat_ads_.BindNewEndpointAndPassReceiver(),
      base::BindOnce(&AdsServiceImpl::OnCreate, AsWeakPtr()));

  const std::string locale = GetLocale();
  RegisterResourceComponentsForLocale(locale);

  OnWalletUpdated();

  MaybeShowMyFirstAdNotification();
}

void AdsServiceImpl::SetEnvironment() {
  ads::mojom::Environment environment;

#if defined(OFFICIAL_BUILD)
  environment = ads::mojom::Environment::kProduction;
#else
  environment = ads::mojom::Environment::kStaging;
#endif

#if defined(OS_ANDROID)
  if (GetBooleanPref(brave_rewards::prefs::kUseRewardsStagingServer)) {
    environment = ads::mojom::Environment::kStaging;
  }
#else
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kProduction)) {
    environment = ads::mojom::Environment::kProduction;
  } else if (command_line.HasSwitch(switches::kStaging)) {
    environment = ads::mojom::Environment::kStaging;
  }
#endif

  bat_ads_service_->SetEnvironment(environment, base::NullCallback());
}

void AdsServiceImpl::SetBuildChannel() {
  ads::mojom::BuildChannelPtr build_channel = ads::mojom::BuildChannel::New();
  build_channel->name = brave::GetChannelName();
  build_channel->is_release = build_channel->name == "release" ? true : false;

  bat_ads_service_->SetBuildChannel(std::move(build_channel),
                                    base::NullCallback());
}

void AdsServiceImpl::UpdateIsDebugFlag() {
  auto is_debug = IsDebug();
  bat_ads_service_->SetDebug(is_debug, base::NullCallback());
}

bool AdsServiceImpl::IsDebug() const {
#if defined(NDEBUG)
  const auto& command_line = *base::CommandLine::ForCurrentProcess();
  return command_line.HasSwitch(switches::kDebug);
#else
  return true;
#endif
}

void AdsServiceImpl::StartCheckIdleStateTimer() {
#if !defined(OS_ANDROID)
  idle_poll_timer_.Stop();

  idle_poll_timer_.Start(FROM_HERE, base::Seconds(1), this,
                         &AdsServiceImpl::CheckIdleState);
#endif
}

void AdsServiceImpl::CheckIdleState() {
  const int idle_threshold = GetIdleTimeThreshold();
  const ui::IdleState idle_state = ui::CalculateIdleState(idle_threshold);
  ProcessIdleState(idle_state, last_idle_time_);

  last_idle_time_ = ui::CalculateIdleTime();
}

void AdsServiceImpl::ProcessIdleState(const ui::IdleState idle_state,
                                      const int idle_time) {
  if (!connected() || idle_state == last_idle_state_) {
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

int AdsServiceImpl::GetIdleTimeThreshold() {
  return GetIntegerPref(ads::prefs::kIdleTimeThreshold);
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

void AdsServiceImpl::OnShowAdNotification(const std::string& notification_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnAdNotificationEvent(notification_id,
                                  ads::mojom::AdNotificationEventType::kViewed);
}

void AdsServiceImpl::OnCloseAdNotification(const std::string& notification_id,
                                           const bool by_user) {
  StopNotificationTimeoutTimer(notification_id);

  if (!connected()) {
    return;
  }

  const ads::mojom::AdNotificationEventType event_type =
      by_user ? ads::mojom::AdNotificationEventType::kDismissed
              : ads::mojom::AdNotificationEventType::kTimedOut;

  bat_ads_->OnAdNotificationEvent(notification_id, event_type);
}

void AdsServiceImpl::OnClickAdNotification(const std::string& notification_id) {
  if (!connected()) {
    return;
  }

  OpenNewTabWithAd(notification_id);

  bat_ads_->OnAdNotificationEvent(
      notification_id, ads::mojom::AdNotificationEventType::kClicked);
}

bool AdsServiceImpl::ShouldShowCustomAdNotifications() {
  const bool can_show_native_notifications =
      NotificationHelper::GetInstance()->CanShowNativeNotifications();

  bool can_fallback_to_custom_ad_notifications =
      features::CanFallbackToCustomAdNotifications();
  if (!can_fallback_to_custom_ad_notifications) {
    ClearPref(prefs::kAdNotificationDidFallbackToCustom);
  } else {
    const bool allowed_to_fallback_to_custom_ad_notifications =
        features::IsAllowedToFallbackToCustomAdNotificationsEnabled();
    if (!allowed_to_fallback_to_custom_ad_notifications) {
      can_fallback_to_custom_ad_notifications = false;
    }
  }

  const bool should_show = features::IsCustomAdNotificationsEnabled();

  const bool should_fallback =
      !can_show_native_notifications && can_fallback_to_custom_ad_notifications;
  if (should_fallback) {
    SetBooleanPref(prefs::kAdNotificationDidFallbackToCustom, true);
  }

  const bool did_fallback =
      GetBooleanPref(prefs::kAdNotificationDidFallbackToCustom);

  return should_show || should_fallback || did_fallback;
}

void AdsServiceImpl::MaybeOpenNewTabWithAd() {
  if (retry_opening_new_tab_for_ad_with_uuid_.empty()) {
    return;
  }

  OpenNewTabWithAd(retry_opening_new_tab_for_ad_with_uuid_);

  retry_opening_new_tab_for_ad_with_uuid_ = "";
}

void AdsServiceImpl::OpenNewTabWithAd(const std::string& uuid) {
  if (StopNotificationTimeoutTimer(uuid)) {
    VLOG(1) << "Cancelled timeout for ad notification with uuid " << uuid;
  }

  if (!connected() || !is_initialized_) {
    RetryOpeningNewTabWithAd(uuid);
    return;
  }

  bat_ads_->GetAdNotification(
      uuid, base::BindOnce(&AdsServiceImpl::OnOpenNewTabWithAd, AsWeakPtr()));
}

void AdsServiceImpl::OnOpenNewTabWithAd(const std::string& json) {
  ads::AdNotificationInfo notification;
  notification.FromJson(json);

  OpenNewTabWithUrl(notification.target_url);
}

void AdsServiceImpl::OnNewTabPageAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const ads::mojom::NewTabPageAdEventType event_type) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnNewTabPageAdEvent(uuid, creative_instance_id, event_type);
}

void AdsServiceImpl::OnPromotedContentAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const ads::mojom::PromotedContentAdEventType event_type) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnPromotedContentAdEvent(uuid, creative_instance_id, event_type);
}

void AdsServiceImpl::GetInlineContentAd(const std::string& dimensions,
                                        OnGetInlineContentAdCallback callback) {
  if (!connected()) {
    std::move(callback).Run(false, "", base::DictionaryValue());
    return;
  }

  bat_ads_->GetInlineContentAd(
      dimensions, base::BindOnce(&AdsServiceImpl::OnGetInlineContentAd,
                                 AsWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::OnInlineContentAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const ads::mojom::InlineContentAdEventType event_type) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnInlineContentAdEvent(uuid, creative_instance_id, event_type);
}

void AdsServiceImpl::PurgeOrphanedAdEventsForType(
    const ads::mojom::AdType ad_type) {
  if (!connected()) {
    return;
  }

  bat_ads_->PurgeOrphanedAdEventsForType(ad_type);
}

void AdsServiceImpl::RetryOpeningNewTabWithAd(const std::string& uuid) {
  VLOG(1) << "Retry opening new tab for ad with uuid " << uuid;
  retry_opening_new_tab_for_ad_with_uuid_ = uuid;
}

void AdsServiceImpl::OpenNewTabWithUrl(const std::string& url) {
  if (g_browser_process->IsShuttingDown()) {
    return;
  }

  GURL gurl(url);
  if (!gurl.is_valid()) {
    VLOG(0) << "Failed to open new tab due to invalid URL: " << url;
    return;
  }

#if defined(OS_ANDROID)
  // ServiceTabLauncher can currently only launch new tabs
  const content::OpenURLParams params(gurl, content::Referrer(),
                                      WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                      ui::PAGE_TRANSITION_LINK, true);
  ServiceTabLauncher::GetInstance()->LaunchTab(
      profile_, params, base::BindOnce([](content::WebContents*) {}));
#else
  Browser* browser = chrome::FindTabbedBrowser(profile_, false);
  if (!browser) {
    browser = Browser::Create(Browser::CreateParams(profile_, true));
  }

  NavigateParams nav_params(browser, gurl, ui::PAGE_TRANSITION_LINK);
  nav_params.disposition = WindowOpenDisposition::SINGLETON_TAB;
  nav_params.window_action = NavigateParams::SHOW_WINDOW;
  nav_params.path_behavior = NavigateParams::IGNORE_AND_NAVIGATE;
  Navigate(&nav_params);
#endif
}

void AdsServiceImpl::NotificationTimedOut(const std::string& uuid) {
  if (!connected()) {
    return;
  }

  CloseNotification(uuid);
}

void AdsServiceImpl::RegisterResourceComponentsForLocale(
    const std::string& locale) {
  g_brave_browser_process->resource_component()->RegisterComponentsForLocale(
      locale);
}

void AdsServiceImpl::OnURLRequestStarted(
    const GURL& final_url,
    const network::mojom::URLResponseHead& response_head) {
  if (response_head.headers->response_code() == -1) {
    VLOG(6) << "Response headers are malformed!!";
    return;
  }
}

void AdsServiceImpl::OnURLRequestComplete(
    SimpleURLLoaderList::iterator url_loader_it,
    ads::UrlRequestCallback callback,
    const std::unique_ptr<std::string> response_body) {
  auto url_loader = std::move(*url_loader_it);
  url_loaders_.erase(url_loader_it);

  if (!connected()) {
    return;
  }

  int response_code = -1;

  base::flat_map<std::string, std::string> headers;

  if (!url_loader->ResponseInfo()) {
    VLOG(6) << "ResponseInfo was never received";
  } else if (!url_loader->ResponseInfo()->headers) {
    VLOG(6) << "Failed to obtain headers from the network stack";
  } else {
    response_code = url_loader->ResponseInfo()->headers->response_code();

    scoped_refptr<net::HttpResponseHeaders> headers_list =
        url_loader->ResponseInfo()->headers;

    if (headers_list) {
      size_t iter = 0;
      std::string key;
      std::string value;

      while (headers_list->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
      }
    }
  }

  ads::mojom::UrlResponse url_response;
  url_response.url = url_loader->GetFinalURL().spec();
  url_response.status_code = response_code;
  url_response.body = response_body ? *response_body : "";
  url_response.headers = headers;

  callback(url_response);
}

void AdsServiceImpl::OnGetInlineContentAd(OnGetInlineContentAdCallback callback,
                                          const bool success,
                                          const std::string& dimensions,
                                          const std::string& json) {
  base::DictionaryValue dictionary;

  if (success) {
    ads::InlineContentAdInfo ad;
    ad.FromJson(json);

    dictionary = ad.ToValue();
  }

  std::move(callback).Run(success, dimensions, dictionary);
}

void AdsServiceImpl::OnGetAdsHistory(OnGetAdsHistoryCallback callback,
                                     const std::string& json) {
  ads::AdsHistoryInfo ads_history;
  ads_history.FromJson(json);

  // Build the list structure required by the webUI
  int uuid = 0;
  base::ListValue list;

  for (const auto& item : ads_history.items) {
    base::DictionaryValue ad_history_dictionary;
    base::DictionaryValue ad_content_dictionary = item.ad_content.ToValue();
    ad_history_dictionary.SetPath("adContent",
                                  std::move(ad_content_dictionary));
    base::DictionaryValue category_content_dictionary =
        item.category_content.ToValue();
    ad_history_dictionary.SetPath("categoryContent",
                                  std::move(category_content_dictionary));
    base::ListValue ad_history_list;
    ad_history_list.Append(std::move(ad_history_dictionary));

    base::DictionaryValue dictionary;
    dictionary.SetKey("uuid", base::Value(std::to_string(uuid++)));
    const base::Time time = base::Time::FromDoubleT(item.timestamp);
    const double js_time = time.ToJsTimeIgnoringNull();
    dictionary.SetKey("timestampInMilliseconds", base::Value(js_time));
    dictionary.SetPath("adDetailRows", std::move(ad_history_list));

    list.Append(std::move(dictionary));
  }

  std::move(callback).Run(list);
}

bool AdsServiceImpl::CanShowBackgroundNotifications() const {
  return NotificationHelper::GetInstance()->CanShowBackgroundNotifications();
}

void AdsServiceImpl::OnGetAccountStatement(GetAccountStatementCallback callback,
                                           const bool success,
                                           const std::string& json) {
  if (!success) {
    std::move(callback).Run(success, 0, 0, 0.0, 0.0);
    return;
  }

  ads::StatementInfo statement;
  statement.FromJson(json);

  std::move(callback).Run(
      success, statement.next_payment_date, statement.ads_received_this_month,
      statement.earnings_this_month, statement.earnings_last_month);
}

void AdsServiceImpl::OnGetAdDiagnostics(GetAdDiagnosticsCallback callback,
                                        const bool success,
                                        const std::string& json) {
  std::move(callback).Run(success, json);
}

void AdsServiceImpl::OnRemoveAllHistory(const bool success) {
  if (!success) {
    VLOG(0) << "Failed to remove ads history";
    return;
  }

  VLOG(1) << "Successfully removed ads history";
}

void AdsServiceImpl::OnToggleAdThumbUp(OnToggleAdThumbUpCallback callback,
                                       const std::string& json) {
  std::move(callback).Run(json);
}

void AdsServiceImpl::OnToggleAdThumbDown(OnToggleAdThumbDownCallback callback,
                                         const std::string& json) {
  std::move(callback).Run(json);
}

void AdsServiceImpl::OnToggleAdOptIn(OnToggleAdOptInCallback callback,
                                     const std::string& category,
                                     const int action) {
  std::move(callback).Run(category, action);
}

void AdsServiceImpl::OnToggleAdOptOut(OnToggleAdOptOutCallback callback,
                                      const std::string& category,
                                      const int action) {
  std::move(callback).Run(category, action);
}

void AdsServiceImpl::OnToggleSavedAd(OnToggleSavedAdCallback callback,
                                     const std::string& json) {
  std::move(callback).Run(json);
}

void AdsServiceImpl::OnToggleFlaggedAd(OnToggleFlaggedAdCallback callback,
                                       const std::string& json) {
  std::move(callback).Run(json);
}

void AdsServiceImpl::OnLoaded(const ads::LoadCallback& callback,
                              const std::string& value) {
  if (!connected()) {
    return;
  }

  if (value.empty())
    callback(/* success */ false, value);
  else
    callback(/* success */ true, value);
}

void AdsServiceImpl::OnSaved(const ads::ResultCallback& callback,
                             const bool success) {
  if (!connected()) {
    return;
  }

  callback(success);
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
                {{10, 11}, &AdsServiceImpl::MigratePrefsVersion10To11}});

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
      std::find(legacy_country_codes.begin(), legacy_country_codes.end(),
                country_code) != legacy_country_codes.end();

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

  SetEnabled(false);
}

void AdsServiceImpl::MigratePrefsVersion7To8() {
  const bool rewards_enabled = GetBooleanPref(brave_rewards::prefs::kEnabled);
  if (!rewards_enabled) {
    SetEnabled(false);
  }
}

void AdsServiceImpl::MigratePrefsVersion8To9() {
  // Intentionally empty as we no longer need to migrate ads per day due to
  // deprecation of prefs::kAdsPerDay
}

void AdsServiceImpl::MigratePrefsVersion9To10() {
  if (!PrefExists(ads::prefs::kAdsPerHour)) {
    return;
  }

  const int64_t ads_per_hour = GetInt64Pref(ads::prefs::kAdsPerHour);
  if (ads_per_hour == -1 || ads_per_hour == 2) {
    // The user did not change the ads per hour setting from the legacy default
    // value of 2 so we should clear the preference to transition to
    // |kDefaultAdNotificationsPerHour|
    profile_->GetPrefs()->ClearPref(ads::prefs::kAdsPerHour);
  }
}

void AdsServiceImpl::MigratePrefsVersion10To11() {
  if (!PrefExists(ads::prefs::kAdsPerHour)) {
    return;
  }

  const int64_t ads_per_hour = GetInt64Pref(ads::prefs::kAdsPerHour);
  if (ads_per_hour == 0 || ads_per_hour == -1) {
    // Clear the ads per hour preference to transition to
    // |kDefaultAdNotificationsPerHour|
    profile_->GetPrefs()->ClearPref(ads::prefs::kAdsPerHour);
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
#if !defined(OS_ANDROID)
  return GetBooleanPref(ads::prefs::kEnabled) &&
         !PrefExists(ads::prefs::kIdleTimeThreshold) &&
         !PrefExists(prefs::kVersion) && !first_run::IsChromeFirstRun();
#else
  return false;
#endif
}

void AdsServiceImpl::DisableAdsIfUpgradingFromPreBraveAdsBuild() {
  if (!is_upgrading_from_pre_brave_ads_build_) {
    return;
  }

  SetEnabled(false);
}

void AdsServiceImpl::DisableAdsForUnsupportedCountryCodes(
    const std::string& country_code,
    const std::vector<std::string>& supported_country_codes) {
  if (std::find(supported_country_codes.begin(), supported_country_codes.end(),
                country_code) != supported_country_codes.end()) {
    return;
  }

  SetEnabled(false);
}

void AdsServiceImpl::MaybeShowMyFirstAdNotification() {
  if (!ShouldShowMyFirstAdNotification()) {
    return;
  }

  if (!NotificationHelper::GetInstance()->ShowMyFirstAdNotification()) {
    return;
  }

  SetBooleanPref(prefs::kShouldShowMyFirstAdNotification, false);
}

bool AdsServiceImpl::ShouldShowMyFirstAdNotification() {
  const bool should_show_my_first_ad_notification =
      GetBooleanPref(prefs::kShouldShowMyFirstAdNotification);
  return IsEnabled() && ShouldShowNotifications() &&
         should_show_my_first_ad_notification;
}

bool AdsServiceImpl::PrefExists(const std::string& path) const {
  return profile_->GetPrefs()->HasPrefPath(path);
}

void AdsServiceImpl::OnPrefsChanged(const std::string& pref) {
  if (pref == ads::prefs::kEnabled) {
    rewards_service_->OnAdsEnabled(IsEnabled());
    if (!IsEnabled()) {
      SuspendP2AHistograms();
      VLOG(1) << "P2A histograms suspended";

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
      // Close any open captcha tooltip
      ads_tooltips_delegate_->CloseCaptchaTooltip();

      // Clear any scheduled captcha
      adaptive_captcha_service_->ClearScheduledCaptcha();
#endif
    }

    brave_rewards::p3a::UpdateAdsStateOnPreferenceChange(profile_->GetPrefs(),
                                                         pref);
    MaybeStart(/* should_restart */ false);
  } else if (pref == ads::prefs::kIdleTimeThreshold) {
    StartCheckIdleStateTimer();
  } else if (pref == brave_rewards::prefs::kWalletBrave) {
    OnWalletUpdated();
  } else if (pref == brave_news::prefs::kBraveTodayOptedIn ||
             pref == brave_news::prefs::kNewTabPageShowToday) {
    MaybeStart(/* should_restart */ false);
  }
}

bool AdsServiceImpl::connected() {
  return bat_ads_.is_bound() && !g_browser_process->IsShuttingDown();
}

///////////////////////////////////////////////////////////////////////////////

bool AdsServiceImpl::IsNetworkConnectionAvailable() const {
  return !net::NetworkChangeNotifier::IsOffline();
}

bool AdsServiceImpl::IsForeground() const {
  return BackgroundHelper::GetInstance()->IsForeground();
}

bool AdsServiceImpl::IsFullScreen() const {
#if !defined(OS_ANDROID)
  return IsFullScreenMode();
#else
  return true;
#endif
}

std::string AdsServiceImpl::GetLocale() const {
  return brave_l10n::LocaleHelper::GetInstance()->GetLocale();
}

std::string AdsServiceImpl::LoadDataResourceAndDecompressIfNeeded(
    const int id) const {
  std::string data_resource;

  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    data_resource = resource_bundle.LoadDataResourceString(id);
  } else {
    data_resource = std::string(resource_bundle.GetRawDataResource(id));
  }

  return data_resource;
}

void AdsServiceImpl::ShowNotification(const ads::AdNotificationInfo& info) {
  if (ShouldShowCustomAdNotifications()) {
    std::unique_ptr<AdNotificationPlatformBridge> platform_bridge =
        std::make_unique<AdNotificationPlatformBridge>(profile_);

    std::u16string title;
    if (base::IsStringUTF8(info.title)) {
      title = base::UTF8ToUTF16(info.title);
    }

    std::u16string body;
    if (base::IsStringUTF8(info.body)) {
      body = base::UTF8ToUTF16(info.body);
    }

    const AdNotification ad_notification(info.uuid, title, body, nullptr);

    platform_bridge->ShowAdNotification(ad_notification);
  } else {
    std::u16string title;
    if (base::IsStringUTF8(info.title)) {
      title = base::UTF8ToUTF16(info.title);
    }

    std::u16string body;
    if (base::IsStringUTF8(info.body)) {
      body = base::UTF8ToUTF16(info.body);
    }

    message_center::RichNotificationData notification_data;
    notification_data.context_message = u" ";

    const std::string url = kAdNotificationUrlPrefix + info.uuid;

    std::unique_ptr<message_center::Notification> notification =
        std::make_unique<message_center::Notification>(
            message_center::NOTIFICATION_TYPE_SIMPLE, info.uuid, title, body,
            gfx::Image(), std::u16string(), GURL(url),
            message_center::NotifierId(
                message_center::NotifierType::SYSTEM_COMPONENT,
                "service.ads_service"),
            notification_data, nullptr);

#if !defined(OS_MAC) || defined(OFFICIAL_BUILD)
    // set_never_timeout uses an XPC service which requires signing so for now
    // we don't set this for macos dev builds
    notification->set_never_timeout(true);
#endif

    display_service_->Display(NotificationHandler::Type::BRAVE_ADS,
                              *notification, /*metadata=*/nullptr);
  }

  StartNotificationTimeoutTimer(info.uuid);
}

void AdsServiceImpl::StartNotificationTimeoutTimer(const std::string& uuid) {
#if defined(OS_ANDROID)
  if (!ShouldShowCustomAdNotifications()) {
    return;
  }
#endif

  const int timeout_in_seconds = features::AdNotificationTimeout();
  if (timeout_in_seconds == 0) {
    // Never time out
    return;
  }

  const base::TimeDelta timeout = base::Seconds(timeout_in_seconds);

  notification_timers_[uuid] = std::make_unique<base::OneShotTimer>();
  notification_timers_[uuid]->Start(
      FROM_HERE, timeout,
      base::BindOnce(&AdsServiceImpl::NotificationTimedOut, AsWeakPtr(), uuid));

  VLOG(1) << "Timeout ad notification with uuid " << uuid << " in "
          << timeout_in_seconds << " seconds";
}

bool AdsServiceImpl::StopNotificationTimeoutTimer(const std::string& uuid) {
  const auto iter = notification_timers_.find(uuid);
  if (iter == notification_timers_.end()) {
    return false;
  }

  notification_timers_.erase(iter);

  return true;
}

bool AdsServiceImpl::ShouldShowNotifications() {
  if (!features::IsAdNotificationsEnabled()) {
    LOG(INFO) << "Notification not made: Ad notifications feature is disabled";
    return false;
  }

  if (!NotificationHelper::GetInstance()->CanShowNativeNotifications()) {
    return ShouldShowCustomAdNotifications();
  }

  return true;
}

void AdsServiceImpl::CloseNotification(const std::string& uuid) {
  if (ShouldShowCustomAdNotifications()) {
    std::unique_ptr<AdNotificationPlatformBridge> platform_bridge =
        std::make_unique<AdNotificationPlatformBridge>(profile_);

    platform_bridge->CloseAdNotification(uuid);
  } else {
#if defined(OS_ANDROID)
    const std::string brave_ads_url_prefix = kAdNotificationUrlPrefix;
    const GURL service_worker_scope =
        GURL(brave_ads_url_prefix.substr(0, brave_ads_url_prefix.size() - 1));
    BraveNotificationPlatformBridgeHelperAndroid::MaybeRegenerateNotification(
        uuid, service_worker_scope);
#endif
    display_service_->Close(NotificationHandler::Type::BRAVE_ADS, uuid);
  }
}

void AdsServiceImpl::RecordAdEventForId(const std::string& id,
                                        const std::string& ad_type,
                                        const std::string& confirmation_type,
                                        const double timestamp) const {
  FrequencyCappingHelper::GetInstance()->RecordAdEventForId(
      id, ad_type, confirmation_type, timestamp);
}

std::vector<double> AdsServiceImpl::GetAdEvents(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  return FrequencyCappingHelper::GetInstance()->GetAdEvents(ad_type,
                                                            confirmation_type);
}

void AdsServiceImpl::ResetAdEventsForId(const std::string& id) const {
  return FrequencyCappingHelper::GetInstance()->ResetAdEventsForId(id);
}

void AdsServiceImpl::UrlRequest(ads::mojom::UrlRequestPtr url_request,
                                ads::UrlRequestCallback callback) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(url_request->url);
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

  url_loader->SetOnResponseStartedCallback(base::BindOnce(
      &AdsServiceImpl::OnURLRequestStarted, base::Unretained(this)));

  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  url_loader->SetAllowHttpErrorResults(true);

  auto url_loader_it =
      url_loaders_.insert(url_loaders_.end(), std::move(url_loader));
  url_loader_it->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      profile_->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess()
          .get(),
      base::BindOnce(&AdsServiceImpl::OnURLRequestComplete,
                     base::Unretained(this), url_loader_it, callback));
}

void AdsServiceImpl::Save(const std::string& name,
                          const std::string& value,
                          ads::ResultCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&base::ImportantFileWriter::WriteFileAtomically,
                     base_path_.AppendASCII(name), value, base::StringPiece()),
      base::BindOnce(&AdsServiceImpl::OnSaved, AsWeakPtr(),
                     std::move(callback)));
}

void AdsServiceImpl::LoadAdsResource(const std::string& id,
                                     const int version,
                                     ads::LoadCallback callback) {
  const absl::optional<base::FilePath> path =
      g_brave_browser_process->resource_component()->GetPath(id, version);

  if (!path) {
    callback(/* success */ false, "");
    return;
  }

  VLOG(1) << "Loading ads resource from " << path.value();

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, path.value()),
      base::BindOnce(&AdsServiceImpl::OnLoaded, AsWeakPtr(),
                     std::move(callback)));
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
      &task_tracker_);
}

void AdsServiceImpl::OnBrowsingHistorySearchComplete(
    ads::GetBrowsingHistoryCallback callback,
    history::QueryResults results) {
  if (!connected()) {
    return;
  }

  std::vector<std::string> history;
  for (const auto& result : results) {
    history.push_back(result.url().GetWithEmptyPath().spec());
  }

  std::sort(history.begin(), history.end());
  history.erase(std::unique(history.begin(), history.end()), history.end());

  callback(history);
}

void AdsServiceImpl::RecordP2AEvent(const std::string& name,
                                    const ads::mojom::P2AEventType type,
                                    const std::string& value) {
  switch (type) {
    case ads::mojom::P2AEventType::kListType: {
      absl::optional<base::Value> maybe_list = base::JSONReader::Read(value);
      if (!maybe_list || !maybe_list->is_list()) {
        break;
      }

      base::ListValue* list = nullptr;
      if (!maybe_list->GetAsList(&list)) {
        break;
      }

      for (auto& item : list->GetList()) {
        RecordInWeeklyStorageAndEmitP2AHistogramAnswer(profile_->GetPrefs(),
                                                       item.GetString());
      }
      break;
    }
  }
}

void AdsServiceImpl::LogTrainingCovariates(
    const ads::mojom::TrainingCovariatesPtr training_covariates) {
  if (!ad_notification_timing_data_store_) {
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/21189): Refactor DB to
  // use generic key/value schema across all data stores
  brave_federated::AdNotificationTimingTaskLog log;

  for (const auto& covariate : training_covariates->covariates) {
    switch (covariate->covariate_type) {
      case ads::mojom::CovariateType::kAdNotificationWasClicked: {
        DCHECK_EQ(ads::mojom::DataType::kBool, covariate->data_type)
            << "covariate type should be a bool";

        bool value_as_bool = false;
        if (covariate->value == kTrue) {
          value_as_bool = true;
        }

        log.label = value_as_bool;
        break;
      }

      case ads::mojom::CovariateType::
          kAdNotificationLocaleCountryAtTimeOfServing: {
        DCHECK_EQ(ads::mojom::DataType::kString, covariate->data_type)
            << "covariate type should be a string";

        log.locale = covariate->value;
        break;
      }

      case ads::mojom::CovariateType::kAdNotificationImpressionServedAt: {
        DCHECK_EQ(ads::mojom::DataType::kDouble, covariate->data_type)
            << "covariate type should be a double";

        double value_as_double = 0.0;
        if (!base::StringToDouble(covariate->value, &value_as_double)) {
          NOTREACHED() << "Failed to convert covariate value to double";
          break;
        }

        log.time = base::Time::FromDoubleT(value_as_double);
        break;
      }

      case ads::mojom::CovariateType::
          kAdNotificationNumberOfTabsOpenedInPast30Minutes: {
        DCHECK_EQ(ads::mojom::DataType::kInt, covariate->data_type)
            << "covariate type should be an int";

        int value_as_int = 0;
        if (!base::StringToInt(covariate->value, &value_as_int)) {
          NOTREACHED() << "Failed to convert covariate value to int";
          break;
        }

        log.number_of_tabs = value_as_int;
        break;
      }
    }
  }

  auto callback =
      base::BindOnce(&AdsServiceImpl::OnLogTrainingCovariates, AsWeakPtr());
  ad_notification_timing_data_store_->AddLog(log, std::move(callback));
}

void AdsServiceImpl::OnLogTrainingCovariates(bool success) {
  if (!success) {
    VLOG(1) << "Failed to log training covariates";
    return;
  }

  VLOG(1) << "Successfully logged training covariates";
}

void AdsServiceImpl::Load(const std::string& name, ads::LoadCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(&AdsServiceImpl::OnLoaded, AsWeakPtr(),
                     std::move(callback)));
}

std::string AdsServiceImpl::LoadResourceForId(const std::string& id) {
  const auto resource_id = GetSchemaResourceId(id);
  return LoadDataResourceAndDecompressIfNeeded(resource_id);
}

void AdsServiceImpl::ClearScheduledCaptcha() {
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  adaptive_captcha_service_->ClearScheduledCaptcha();
#endif
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
    const std::string& captcha_id) {
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  PrefService* pref_service = profile_->GetPrefs();
  if (pref_service->GetBoolean(
          brave_adaptive_captcha::kScheduledCaptchaPaused)) {
    VLOG(0) << "Ads paused; support intervention required";
    return;
  }

  const int snooze_count = pref_service->GetInteger(
      brave_adaptive_captcha::kScheduledCaptchaSnoozeCount);

  ads_tooltips_delegate_->ShowCaptchaTooltip(
      payment_id, captcha_id, snooze_count == 0,
      base::BindOnce(&AdsServiceImpl::ShowScheduledCaptcha, AsWeakPtr()),
      base::BindOnce(&AdsServiceImpl::SnoozeScheduledCaptcha, AsWeakPtr()));
#endif
}

ads::mojom::DBCommandResponsePtr RunDBTransactionOnFileTaskRunner(
    ads::mojom::DBTransactionPtr transaction,
    ads::Database* database) {
  DCHECK(database);

  auto response = ads::mojom::DBCommandResponse::New();

  if (!database) {
    response->status = ads::mojom::DBCommandResponse::Status::RESPONSE_ERROR;
  } else {
    database->RunTransaction(std::move(transaction), response.get());
  }

  return response;
}

void AdsServiceImpl::RunDBTransaction(ads::mojom::DBTransactionPtr transaction,
                                      ads::RunDBTransactionCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&RunDBTransactionOnFileTaskRunner, std::move(transaction),
                     database_.get()),
      base::BindOnce(&AdsServiceImpl::OnRunDBTransaction, AsWeakPtr(),
                     std::move(callback)));
}

void AdsServiceImpl::OnRunDBTransaction(
    ads::RunDBTransactionCallback callback,
    ads::mojom::DBCommandResponsePtr response) {
  callback(std::move(response));
}

void AdsServiceImpl::OnAdRewardsChanged() {
  for (AdsServiceObserver& observer : observers_) {
    observer.OnAdRewardsChanged();
  }
}

void AdsServiceImpl::WriteDiagnosticLog(const std::string& file,
                                        const int line,
                                        const int verbose_level,
                                        const std::string& message) {
  rewards_service_->WriteDiagnosticLog(file, line, verbose_level, message);
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

bool AdsServiceImpl::GetBooleanPref(const std::string& path) const {
  return profile_->GetPrefs()->GetBoolean(path);
}

void AdsServiceImpl::SetBooleanPref(const std::string& path, const bool value) {
  profile_->GetPrefs()->SetBoolean(path, value);
  OnPrefChanged(path);
}

int AdsServiceImpl::GetIntegerPref(const std::string& path) const {
  return profile_->GetPrefs()->GetInteger(path);
}

void AdsServiceImpl::SetIntegerPref(const std::string& path, const int value) {
  profile_->GetPrefs()->SetInteger(path, value);
  OnPrefChanged(path);
}

double AdsServiceImpl::GetDoublePref(const std::string& path) const {
  return profile_->GetPrefs()->GetDouble(path);
}

void AdsServiceImpl::SetDoublePref(const std::string& path,
                                   const double value) {
  profile_->GetPrefs()->SetDouble(path, value);
  OnPrefChanged(path);
}

std::string AdsServiceImpl::GetStringPref(const std::string& path) const {
  return profile_->GetPrefs()->GetString(path);
}

void AdsServiceImpl::SetStringPref(const std::string& path,
                                   const std::string& value) {
  profile_->GetPrefs()->SetString(path, value);
  OnPrefChanged(path);
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
  OnPrefChanged(path);
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
  OnPrefChanged(path);
}

void AdsServiceImpl::ClearPref(const std::string& path) {
  profile_->GetPrefs()->ClearPref(path);
  OnPrefChanged(path);
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceImpl::OnBackground() {
  if (!connected()) {
    return;
  }

  bat_ads_->OnBackground();
}

void AdsServiceImpl::OnForeground() {
  if (!connected()) {
    return;
  }

  bat_ads_->OnForeground();
}

}  // namespace brave_ads
