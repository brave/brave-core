/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include <limits>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/i18n/time_formatting.h"
#include "bat/ads/ad_history.h"
#include "bat/ads/ads.h"
#include "bat/ads/ads_history.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/resources/grit/bat_ads_resources.h"
#include "brave/components/brave_ads/browser/ad_notification.h"
#include "brave/components/brave_ads/browser/ads_notification_handler.h"
#include "brave/components/brave_ads/browser/bundle_state_database.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/common/switches.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/services/bat_ads/public/cpp/ads_client_mojo_bridge.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "brave/components/brave_ads/browser/notification_helper.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile.h"
#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#endif
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_constants.h"
#include "components/prefs/pref_service.h"
#include "components/wifi/wifi_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/service_manager_connection.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/dom_distiller_js/dom_distiller.pb.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/public/cpp/notification.h"

#if defined(OS_ANDROID)
#include "brave/browser/notifications/brave_notification_platform_bridge_helper_android.h"
#include "chrome/browser/android/service_tab_launcher.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "content/public/browser/page_navigator.h"
#endif

using brave_rewards::RewardsNotificationService;

namespace brave_ads {

namespace {

const char kRewardsNotificationAdsOnboarding[] =
    "rewards_notification_ads_onboarding";

const unsigned int kRetriesCountOnNetworkChange = 1;

}  // namespace

namespace {

static std::map<std::string, int> g_schema_resource_ids = {
  {ads::_catalog_schema_resource_name, IDR_ADS_CATALOG_SCHEMA},
  {ads::_bundle_schema_resource_name, IDR_ADS_BUNDLE_SCHEMA},
};

int GetSchemaResourceId(
    const std::string& name) {
  if (g_schema_resource_ids.find(name) != g_schema_resource_ids.end()) {
    return g_schema_resource_ids[name];
  }

  NOTREACHED();

  return 0;
}

static std::map<std::string, int> g_user_model_resource_ids = {
  {"en", IDR_ADS_USER_MODEL_EN},
  {"de", IDR_ADS_USER_MODEL_DE},
  {"fr", IDR_ADS_USER_MODEL_FR},
};

int GetUserModelResourceId(
    const std::string& locale) {
  if (g_user_model_resource_ids.find(locale) !=
      g_user_model_resource_ids.end()) {
    return g_user_model_resource_ids[locale];
  }

  NOTREACHED();

  return 0;
}

std::string URLMethodToRequestType(
    ads::URLRequestMethod method) {
  switch (method) {
    case ads::URLRequestMethod::GET: {
      return "GET";
    }

    case ads::URLRequestMethod::POST: {
      return "POST";
    }

    case ads::URLRequestMethod::PUT: {
      return "PUT";
    }

    default: {
      NOTREACHED();

      return "GET";
    }
  }
}

void PostWriteCallback(
    const base::Callback<void(bool success)>& callback,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    const bool success) {
  // We can't run |callback| on the current thread. Bounce back to
  // the |reply_task_runner| which is the correct sequenced thread.
  reply_task_runner->PostTask(FROM_HERE, base::Bind(callback, success));
}

std::string LoadOnFileTaskRunner(
    const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    return "";
  }

  return data;
}

bool EnsureBaseDirectoryExistsOnFileTaskRunner(
    const base::FilePath& path) {
  if (base::DirectoryExists(path)) {
    return true;
  }

  return base::CreateDirectory(path);
}

ads::CreativeAdNotificationList GetCreativeAdNotificationsOnFileTaskRunner(
    const std::vector<std::string>& categories,
    BundleStateDatabase* backend) {
  ads::CreativeAdNotificationList ads;

  if (!backend) {
    return ads;
  }

  backend->GetCreativeAdNotifications(categories, &ads);
  return ads;
}

ads::AdConversionList GetAdConversionsOnFileTaskRunner(
    BundleStateDatabase* backend) {
  ads::AdConversionList ad_conversions;

  if (!backend) {
    return ad_conversions;
  }

  backend->GetAdConversions(&ad_conversions);

  return ad_conversions;
}

bool ResetOnFileTaskRunner(const base::FilePath& path) {
  bool recursive;

  base::File::Info file_info;
  if (base::GetFileInfo(path, &file_info)) {
    recursive = file_info.is_directory;
  } else {
    recursive = false;
  }

  return base::DeleteFile(path, recursive);
}

bool SaveBundleStateOnFileTaskRunner(
    std::unique_ptr<ads::BundleState> bundle_state,
    BundleStateDatabase* backend) {
  if (backend && backend->SaveBundleState(*bundle_state))
    return true;

  return false;
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

AdsServiceImpl::AdsServiceImpl(Profile* profile) :
    profile_(profile),
    is_initialized_(false),
    file_task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::MayBlock(),
           base::TaskPriority::BEST_EFFORT,
            base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    base_path_(profile_->GetPath().AppendASCII("ads_service")),
    last_idle_state_(ui::IdleState::IDLE_STATE_ACTIVE),
    bundle_state_backend_(new BundleStateDatabase(
        base_path_.AppendASCII("bundle_state"))),
    display_service_(NotificationDisplayService::GetForProfile(profile_)),
    rewards_service_(brave_rewards::RewardsServiceFactory::GetForProfile(
        profile_)),
    bat_ads_client_receiver_(new bat_ads::AdsClientMojoBridge(this)) {
  DCHECK(!profile_->IsOffTheRecord());

  MigratePrefs();

  profile_pref_change_registrar_.Init(profile_->GetPrefs());

  profile_pref_change_registrar_.Add(prefs::kEnabled,
      base::Bind(&AdsServiceImpl::OnPrefsChanged, base::Unretained(this)));

  profile_pref_change_registrar_.Add(brave_rewards::prefs::kBraveRewardsEnabled,
      base::Bind(&AdsServiceImpl::OnPrefsChanged, base::Unretained(this)));

  profile_pref_change_registrar_.Add(prefs::kIdleThreshold,
      base::Bind(&AdsServiceImpl::OnPrefsChanged, base::Unretained(this)));

#if !defined(OS_ANDROID)
  // TODO(tmancey): Refactor on-boarding to be platform agnostic
  MaybeShowOnboarding();
#endif

  MaybeStart(false);
}

AdsServiceImpl::~AdsServiceImpl() {
  file_task_runner_->DeleteSoon(FROM_HERE, bundle_state_backend_.release());
}

bool AdsServiceImpl::IsSupportedLocale() const {
  const std::string locale = GetLocale();
  return ads::IsSupportedLocale(locale);
}

bool AdsServiceImpl::IsNewlySupportedLocale() {
  if (!IsSupportedLocale()) {
    return false;
  }

  const int schema_version =
      GetIntegerPref(prefs::kSupportedRegionsSchemaVersion);
  if (schema_version != prefs::kSupportedRegionsSchemaVersionNumber) {
    SetIntegerPref(prefs::kSupportedRegionsLastSchemaVersion, schema_version);

    SetIntegerPref(prefs::kSupportedRegionsSchemaVersion,
        prefs::kSupportedRegionsSchemaVersionNumber);
  }

  const int last_schema_version =
      GetIntegerPref(prefs::kSupportedRegionsLastSchemaVersion);

  const std::string locale = GetLocale();
  return ads::IsNewlySupportedLocale(locale, last_schema_version);
}

void AdsServiceImpl::SetEnabled(
    const bool is_enabled) {
  SetBooleanPref(prefs::kEnabled, is_enabled);
  rewards_service_->OnAdsEnabled(is_enabled);
}

void AdsServiceImpl::SetAllowAdConversionTracking(
    const bool should_allow) {
  SetBooleanPref(prefs::kShouldAllowAdConversionTracking, should_allow);
}

void AdsServiceImpl::SetAdsPerHour(
    const uint64_t ads_per_hour) {
  SetUint64Pref(prefs::kAdsPerHour, ads_per_hour);
}

void AdsServiceImpl::SetConfirmationsIsReady(
    const bool is_ready) {
  if (!connected()) {
    return;
  }

  bat_ads_->SetConfirmationsIsReady(is_ready);
}

void AdsServiceImpl::ChangeLocale(
    const std::string& locale) {
  if (!connected()) {
    return;
  }

  bat_ads_->ChangeLocale(locale);
}

void AdsServiceImpl::OnPageLoaded(
    const std::string& url,
    const std::string& content) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnPageLoaded(url, content);
}

void AdsServiceImpl::OnMediaStart(
    const SessionID& tab_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnMediaPlaying(tab_id.id());
}

void AdsServiceImpl::OnMediaStop(
    const SessionID& tab_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnMediaStopped(tab_id.id());
}

void AdsServiceImpl::OnTabUpdated(
    const SessionID& tab_id,
    const GURL& url,
    const bool is_active) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnTabUpdated(tab_id.id(), url.spec(), is_active,
      profile_->IsOffTheRecord());
}

void AdsServiceImpl::OnTabClosed(
    const SessionID& tab_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnTabClosed(tab_id.id());
}

void AdsServiceImpl::GetAdsHistory(
    const uint64_t from_timestamp,
    const uint64_t to_timestamp,
    OnGetAdsHistoryCallback callback) {
  if (!connected()) {
    return;
  }

  bat_ads_->GetAdsHistory(from_timestamp, to_timestamp,
      base::BindOnce(&AdsServiceImpl::OnGetAdsHistory, AsWeakPtr(),
          std::move(callback)));
}

void AdsServiceImpl::ToggleAdThumbUp(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const int action,
    OnToggleAdThumbUpCallback callback) {
  bat_ads_->ToggleAdThumbUp(creative_instance_id, creative_set_id, action,
      base::BindOnce(&AdsServiceImpl::OnToggleAdThumbUp, AsWeakPtr(),
          std::move(callback)));
}

void AdsServiceImpl::ToggleAdThumbDown(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const int action,
    OnToggleAdThumbDownCallback callback) {
  bat_ads_->ToggleAdThumbDown(creative_instance_id, creative_set_id, action,
      base::BindOnce(&AdsServiceImpl::OnToggleAdThumbDown, AsWeakPtr(),
          std::move(callback)));
}

void AdsServiceImpl::ToggleAdOptInAction(
    const std::string& category,
    const int action,
    OnToggleAdOptInActionCallback callback) {
  bat_ads_->ToggleAdOptInAction(category, action,
      base::BindOnce(&AdsServiceImpl::OnToggleAdOptInAction, AsWeakPtr(),
          std::move(callback)));
}

void AdsServiceImpl::ToggleAdOptOutAction(
    const std::string& category,
    const int action,
    OnToggleAdOptOutActionCallback callback) {
  bat_ads_->ToggleAdOptOutAction(category, action,
      base::BindOnce(&AdsServiceImpl::OnToggleAdOptOutAction, AsWeakPtr(),
          std::move(callback)));
}

void AdsServiceImpl::ToggleSaveAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool saved,
    OnToggleSaveAdCallback callback) {
  bat_ads_->ToggleSaveAd(creative_instance_id, creative_set_id, saved,
      base::BindOnce(&AdsServiceImpl::OnToggleSaveAd, AsWeakPtr(),
          std::move(callback)));
}

void AdsServiceImpl::ToggleFlagAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool flagged,
    OnToggleFlagAdCallback callback) {
  bat_ads_->ToggleFlagAd(creative_instance_id, creative_set_id, flagged,
      base::BindOnce(&AdsServiceImpl::OnToggleFlagAd, AsWeakPtr(),
          std::move(callback)));
}

bool AdsServiceImpl::IsEnabled() const {
  auto is_enabled = GetBooleanPref(prefs::kEnabled);

  auto is_rewards_enabled =
      GetBooleanPref(brave_rewards::prefs::kBraveRewardsEnabled);

  return is_enabled && is_rewards_enabled;
}

bool AdsServiceImpl::ShouldAllowAdConversionTracking() const {
  return GetBooleanPref(prefs::kShouldAllowAdConversionTracking);
}

uint64_t AdsServiceImpl::GetAdsPerHour() const {
  return GetUint64Pref(prefs::kAdsPerHour);
}

uint64_t AdsServiceImpl::GetAdsPerDay() const {
  return GetUint64Pref(prefs::kAdsPerDay);
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceImpl::Shutdown() {
  BackgroundHelper::GetInstance()->RemoveObserver(this);

  for (auto* const loader : url_loaders_) {
    delete loader;
  }
  url_loaders_.clear();

  idle_poll_timer_.Stop();

  bat_ads_.reset();
  bat_ads_client_receiver_.reset();
  bat_ads_service_.reset();

  is_initialized_ = false;
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceImpl::OnCreate() {
  if (!connected()) {
    return;
  }

  bat_ads_->Initialize(base::BindOnce(&AdsServiceImpl::OnInitialize,
      AsWeakPtr()));
}

void AdsServiceImpl::OnInitialize(
    const int32_t result) {
  if (result != ads::Result::SUCCESS) {
    VLOG(0) << "Failed to initialize ads";
    is_initialized_ = false;
  } else {
    is_initialized_ = true;

    SetAdsServiceForNotificationHandler();

    MaybeViewAdNotification();

    StartCheckIdleStateTimer();
  }
}

void AdsServiceImpl::ShutdownBatAds() {
  VLOG(1) << "Shutting down ads";

  bat_ads_->Shutdown(base::BindOnce(&AdsServiceImpl::OnShutdownBatAds,
      AsWeakPtr()));
}

void AdsServiceImpl::OnShutdownBatAds(
    const int32_t result) {
  DCHECK(is_initialized_);

  if (result != ads::Result::SUCCESS) {
    VLOG(0) << "Failed to shutdown ads";
    return;
  }

  Shutdown();

  ResetAllState();

  VLOG(1) << "Successfully shutdown ads";
}

bool AdsServiceImpl::StartService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!connected());

  auto* connection = content::ServiceManagerConnection::GetForProcess();
  if (!connection) {
    return false;
  }

  if (!bat_ads_service_.is_bound()) {
    connection->GetConnector()->Connect(
        bat_ads::mojom::kServiceName,
        bat_ads_service_.BindNewPipeAndPassReceiver());

    bat_ads_service_.set_disconnect_handler(
        base::Bind(&AdsServiceImpl::MaybeStart, AsWeakPtr(), true));
  }

  SetEnvironment();
  UpdateIsDebugFlag();

  return true;
}

void AdsServiceImpl::MaybeStart(
    const bool should_restart) {
  if (!IsSupportedLocale()) {
    VLOG(1) << GetLocale() << " locale does not support ads";
    Shutdown();
    return;
  }

  if (should_restart) {
    VLOG(1) << "Restarting ads service";
    Shutdown();
  }

  if (!StartService()) {
    VLOG(0) << "Failed to start ads service";
    return;
  }

  if (IsEnabled()) {
    if (should_restart) {
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(FROM_HERE,
          base::BindOnce(&AdsServiceImpl::Start, AsWeakPtr()),
          base::TimeDelta::FromSeconds(1));
    } else {
      Start();
    }
  } else {
    Stop();
  }
}

void AdsServiceImpl::Start() {
  EnsureBaseDirectoryExists();
}

void AdsServiceImpl::Stop() {
  if (!connected()) {
    return;
  }

  ShutdownBatAds();
}

void AdsServiceImpl::ResetAllState() {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ResetOnFileTaskRunner, base_path_),
      base::BindOnce(&AdsServiceImpl::OnResetAllState, AsWeakPtr()));
}

void AdsServiceImpl::OnResetAllState(
    const bool success) {
  if (!success) {
    VLOG(0) << "Failed to reset ads state";
    return;
  }

  VLOG(1) << "Successfully reset ads state";
}

void AdsServiceImpl::EnsureBaseDirectoryExists() {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&EnsureBaseDirectoryExistsOnFileTaskRunner, base_path_),
      base::BindOnce(&AdsServiceImpl::OnEnsureBaseDirectoryExists,
                     AsWeakPtr()));
}

void AdsServiceImpl::OnEnsureBaseDirectoryExists(
    const bool success) {
  if (!success) {
    VLOG(0) << "Failed to create base directory";
    return;
  }

  BackgroundHelper::GetInstance()->AddObserver(this);

  bat_ads_service_->Create(
      bat_ads_client_receiver_.BindNewEndpointAndPassRemote(),
      bat_ads_.BindNewEndpointAndPassReceiver(),
      base::BindOnce(&AdsServiceImpl::OnCreate, AsWeakPtr()));

  MaybeShowMyFirstAdNotification();
}

void AdsServiceImpl::SetEnvironment() {
  ads::Environment environment;

#if defined(OFFICIAL_BUILD)
  environment = ads::Environment::PRODUCTION;
#else
  environment = ads::Environment::STAGING;
#endif

#if defined(OS_ANDROID)
  if (GetBooleanPref(brave_rewards::prefs::kUseRewardsStagingServer)) {
    environment = ads::Environment::STAGING;
  }
#else
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kProduction)) {
    environment = ads::Environment::PRODUCTION;
  } else if (command_line.HasSwitch(switches::kStaging)) {
    environment = ads::Environment::STAGING;
  } else if (command_line.HasSwitch(switches::kDevelopment)) {
    environment = ads::Environment::DEVELOPMENT;
  }
#endif

  bat_ads_service_->SetEnvironment(environment, base::NullCallback());
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

  idle_poll_timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(1), this,
      &AdsServiceImpl::CheckIdleState);
#endif
}

void AdsServiceImpl::CheckIdleState() {
  auto idle_state = ui::CalculateIdleState(GetIdleThreshold());
  ProcessIdleState(idle_state);
}

void AdsServiceImpl::ProcessIdleState(
    const ui::IdleState idle_state) {
  if (!connected() || idle_state == last_idle_state_) {
    return;
  }

  if (idle_state == ui::IdleState::IDLE_STATE_ACTIVE) {
    bat_ads_->OnUnIdle();
  } else {
    bat_ads_->OnIdle();
  }

  last_idle_state_ = idle_state;
}

int AdsServiceImpl::GetIdleThreshold() {
  return GetIntegerPref(prefs::kIdleThreshold);
}

void AdsServiceImpl::OnShow(
    Profile* profile,
    const std::string& uuid) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnAdNotificationEvent(uuid, ads::AdNotificationEventType::kViewed);
}

void AdsServiceImpl::OnClose(
    Profile* profile,
    const GURL& origin,
    const std::string& uuid,
    const bool by_user,
    base::OnceClosure completed_closure) {
  if (StopNotificationTimeoutTimer(uuid)) {
    if (by_user) {
      VLOG(1) << "Cancelled timeout for ad notification with uuid " << uuid;
    } else {
      VLOG(1) << "Timed out ad notification with uuid " << uuid;
    }
  }

  if (connected()) {
    const ads::AdNotificationEventType event_type =
        by_user ? ads::AdNotificationEventType::kDismissed :
            ads::AdNotificationEventType::kTimedOut;

    bat_ads_->OnAdNotificationEvent(uuid, event_type);
  }

  if (completed_closure) {
    std::move(completed_closure).Run();
  }
}

void AdsServiceImpl::MaybeViewAdNotification() {
  if (retry_viewing_ad_notification_with_uuid_.empty()) {
    return;
  }

  ViewAdNotification(retry_viewing_ad_notification_with_uuid_);

  retry_viewing_ad_notification_with_uuid_ = "";
}

void AdsServiceImpl::ViewAdNotification(
    const std::string& uuid) {
  if (StopNotificationTimeoutTimer(uuid)) {
    VLOG(1) << "Cancelled timeout for ad notification with uuid " << uuid;
  }

  if (!connected() || !is_initialized_) {
    RetryViewingAdNotification(uuid);
    return;
  }

  VLOG(1) << "View ad notification with uuid " << uuid;

  bat_ads_->GetAdNotification(
      uuid, base::BindOnce(&AdsServiceImpl::OnViewAdNotification, AsWeakPtr()));
}

void AdsServiceImpl::OnViewAdNotification(
    const std::string& json) {
  ads::AdNotificationInfo notification;
  notification.FromJson(json);

  bat_ads_->OnAdNotificationEvent(notification.uuid,
      ads::AdNotificationEventType::kClicked);

  OpenNewTabWithUrl(notification.target_url);
}

void AdsServiceImpl::RetryViewingAdNotification(
    const std::string& uuid) {
  VLOG(1) << "Retry viewing ad notification with uuid " << uuid;
  retry_viewing_ad_notification_with_uuid_ = uuid;
}

void AdsServiceImpl::SetAdsServiceForNotificationHandler() {
  auto* unowned_ptr = static_cast<AdsNotificationHandler::UnownedPointer*>(
      profile_->GetUserData(AdsNotificationHandler::UserDataKey()));
  CHECK(unowned_ptr);
  unowned_ptr->get()->SetAdsService(this);
}

void AdsServiceImpl::ClearAdsServiceForNotificationHandler() {
  auto* unowned_ptr = static_cast<AdsNotificationHandler::UnownedPointer*>(
      profile_->GetUserData(AdsNotificationHandler::UserDataKey()));
  CHECK(unowned_ptr);
  unowned_ptr->get()->SetAdsService(nullptr);
}

void AdsServiceImpl::OpenNewTabWithUrl(
    const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    VLOG(0) << "Failed to open new tab due to invalid URL: " << url;
    return;
  }

#if defined(OS_ANDROID)
  // ServiceTabLauncher can currently only launch new tabs
  const content::OpenURLParams params(gurl, content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
      true);

  base::Callback<void(content::WebContents*)> callback =
      base::Bind([] (content::WebContents*) {});

  ServiceTabLauncher::GetInstance()->LaunchTab(profile_, params, callback);
#else
  Browser* browser = chrome::FindTabbedBrowser(profile_, false);
  if (!browser) {
    browser = new Browser(Browser::CreateParams(profile_, true));
  }

  NavigateParams nav_params(browser, gurl, ui::PAGE_TRANSITION_LINK);
  nav_params.disposition = WindowOpenDisposition::SINGLETON_TAB;
  nav_params.window_action = NavigateParams::SHOW_WINDOW;
  Navigate(&nav_params);
#endif
}

void AdsServiceImpl::NotificationTimedOut(
    const std::string& uuid) {
  if (!connected()) {
    return;
  }

  CloseNotification(uuid);

  OnClose(profile_, GURL(), uuid, false, base::OnceClosure());
}

void AdsServiceImpl::OnURLLoaderComplete(
    network::SimpleURLLoader* loader,
    ads::URLRequestCallback callback,
    const std::unique_ptr<std::string> response_body) {
  DCHECK(url_loaders_.find(loader) != url_loaders_.end());
  url_loaders_.erase(loader);

  if (!connected()) {
    return;
  }

  auto response_code = -1;

  std::map<std::string, std::string> headers;

  if (loader->ResponseInfo() && loader->ResponseInfo()->headers) {
    response_code = loader->ResponseInfo()->headers->response_code();

    auto headers_list = loader->ResponseInfo()->headers;
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

  callback(response_code, response_body ? *response_body : "", headers);
}

bool AdsServiceImpl::CanShowBackgroundNotifications() const {
  return NotificationHelper::GetInstance()->CanShowBackgroundNotifications();
}

void AdsServiceImpl::OnGetCreativeAdNotifications(
    const ads::GetCreativeAdNotificationsCallback& callback,
    const std::vector<std::string>& categories,
    const ads::CreativeAdNotificationList& ads) {
  if (!connected()) {
    return;
  }

  auto result = ads.empty() ? ads::Result::FAILED : ads::Result::SUCCESS;

  callback(result, categories, ads);
}

void AdsServiceImpl::OnGetAdConversions(
    const ads::GetAdConversionsCallback& callback,
    const ads::AdConversionList& ad_conversions) {
  if (!connected()) {
    return;
  }

  const auto result = ad_conversions.empty() ?
      ads::Result::FAILED : ads::Result::SUCCESS;

  callback(result, ad_conversions);
}

void AdsServiceImpl::OnGetAdsHistory(
    OnGetAdsHistoryCallback callback,
    const std::string& json) {
  ads::AdsHistory ads_history;
  ads_history.FromJson(json);

  // Build the list structure required by the webUI
  int uuid = 0;
  base::ListValue list;
  for (const auto& entry : ads_history.entries) {
    base::DictionaryValue ad_content_dictionary;
    ad_content_dictionary.SetKey("creativeInstanceId",
        base::Value(entry.ad_content.creative_instance_id));
    ad_content_dictionary.SetKey("creativeSetId",
        base::Value(entry.ad_content.creative_set_id));
    ad_content_dictionary.SetKey("brand", base::Value(entry.ad_content.brand));
    ad_content_dictionary.SetKey("brandInfo",
        base::Value(entry.ad_content.brand_info));
    ad_content_dictionary.SetKey("brandLogo",
        base::Value(entry.ad_content.brand_logo));
    ad_content_dictionary.SetKey("brandDisplayUrl",
        base::Value(entry.ad_content.brand_display_url));
    ad_content_dictionary.SetKey("brandUrl",
        base::Value(entry.ad_content.brand_url));
    ad_content_dictionary.SetKey("likeAction",
        base::Value(static_cast<int>(entry.ad_content.like_action)));
    ad_content_dictionary.SetKey("adAction",
        base::Value(std::string(entry.ad_content.ad_action)));
    ad_content_dictionary.SetKey("savedAd",
        base::Value(entry.ad_content.saved_ad));
    ad_content_dictionary.SetKey("flaggedAd",
        base::Value(entry.ad_content.flagged_ad));

    base::DictionaryValue category_content_dictionary;
    category_content_dictionary.SetKey("category",
        base::Value(entry.category_content.category));
    category_content_dictionary.SetKey("optAction",
        base::Value(static_cast<int>(entry.category_content.opt_action)));

    base::DictionaryValue ad_history_dictionary;
    ad_history_dictionary.SetKey("uuid", base::Value(entry.uuid));
    ad_history_dictionary.SetKey("parentUuid", base::Value(entry.parent_uuid));
    ad_history_dictionary.SetPath("adContent",
        std::move(ad_content_dictionary));
    ad_history_dictionary.SetPath("categoryContent",
        std::move(category_content_dictionary));

    base::DictionaryValue dictionary;

    dictionary.SetKey("uuid", base::Value(std::to_string(uuid++)));
    auto time = base::Time::FromDoubleT(entry.timestamp_in_seconds);
    auto js_time = time.ToJsTime();
    dictionary.SetKey("timestampInMilliseconds", base::Value(js_time));

    base::ListValue ad_history_list;
    ad_history_list.Append(std::move(ad_history_dictionary));
    dictionary.SetPath("adDetailRows", std::move(ad_history_list));

    list.Append(std::move(dictionary));
  }

  std::move(callback).Run(list);
}

void AdsServiceImpl::OnRemoveAllHistory(
    const int32_t result) {
  if (result != ads::Result::SUCCESS) {
    VLOG(0) << "Failed to remove ads history";
    return;
  }

  VLOG(1) << "Successfully removed ads history";
}

void AdsServiceImpl::OnToggleAdThumbUp(
    OnToggleAdThumbUpCallback callback,
    const std::string& creative_instance_id,
    const int action) {
  std::move(callback).Run(creative_instance_id, action);
}

void AdsServiceImpl::OnToggleAdThumbDown(
    OnToggleAdThumbDownCallback callback,
    const std::string& creative_instance_id,
    const int action) {
  std::move(callback).Run(creative_instance_id, action);
}

void AdsServiceImpl::OnToggleAdOptInAction(
    OnToggleAdOptInActionCallback callback,
    const std::string& category,
    const int action) {
  std::move(callback).Run(category, action);
}

void AdsServiceImpl::OnToggleAdOptOutAction(
    OnToggleAdOptOutActionCallback callback,
    const std::string& category,
    const int action) {
  std::move(callback).Run(category, action);
}

void AdsServiceImpl::OnToggleSaveAd(
    OnToggleSaveAdCallback callback,
    const std::string& creative_instance_id,
    const bool saved) {
  std::move(callback).Run(creative_instance_id, saved);
}

void AdsServiceImpl::OnToggleFlagAd(
    OnToggleSaveAdCallback callback,
    const std::string& creative_instance_id,
    const bool flagged) {
  std::move(callback).Run(creative_instance_id, flagged);
}

void AdsServiceImpl::OnSaveBundleState(
    const ads::ResultCallback& callback,
    const bool success) {
  if (!connected()) {
    return;
  }

  callback(success ? ads::Result::SUCCESS : ads::Result::FAILED);
}

void AdsServiceImpl::OnLoaded(
    const ads::LoadCallback& callback,
    const std::string& value) {
  if (!connected()) {
    return;
  }

  if (value.empty())
    callback(ads::Result::FAILED, value);
  else
    callback(ads::Result::SUCCESS, value);
}

void AdsServiceImpl::OnSaved(
    const ads::ResultCallback& callback,
    const bool success) {
  if (!connected()) {
    return;
  }

  callback(success ? ads::Result::SUCCESS : ads::Result::FAILED);
}

void AdsServiceImpl::OnReset(
    const ads::ResultCallback& callback,
    const bool success) {
  if (!connected()) {
    return;
  }

  callback(success ? ads::Result::SUCCESS : ads::Result::FAILED);
}

void AdsServiceImpl::MigratePrefs() {
  is_upgrading_from_pre_brave_ads_build_ = IsUpgradingFromPreBraveAdsBuild();
  if (is_upgrading_from_pre_brave_ads_build_) {
    VLOG(1) << "Migrating ads preferences from pre Brave Ads build";

    // Force migration of preferences from version 1 if
    // |is_upgrading_from_pre_brave_ads_build_| is set to |true| to fix
    // "https://github.com/brave/brave-browser/issues/5434"
    SetIntegerPref(prefs::kVersion, 1);
  } else {
    VLOG(1) << "Migrating ads preferences";
  }

  auto source_version = GetPrefsVersion();
  auto dest_version = prefs::kCurrentVersionNumber;

  if (!MigratePrefs(source_version, dest_version, true)) {
    // Migration dry-run failed, so do not migrate preferences
    VLOG(0) << "Failed to migrate ads preferences from version "
        << source_version << " to " << dest_version;

    return;
  }

  MigratePrefs(source_version, dest_version);
}

bool AdsServiceImpl::MigratePrefs(
    const int source_version,
    const int dest_version,
    const bool is_dry_run) {
  DCHECK(source_version >= 1) << "Invalid migration path";
  DCHECK(source_version <= dest_version) << "Invalid migration path";

  if (source_version == dest_version) {
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

  static std::map<std::pair<int, int>, void (AdsServiceImpl::*)()>
      mappings {
    // {{from version, to version}, function}
    {{1, 2}, &AdsServiceImpl::MigratePrefsVersion1To2},
    {{2, 3}, &AdsServiceImpl::MigratePrefsVersion2To3},
    {{3, 4}, &AdsServiceImpl::MigratePrefsVersion3To4},
    {{4, 5}, &AdsServiceImpl::MigratePrefsVersion4To5},
    {{5, 6}, &AdsServiceImpl::MigratePrefsVersion5To6},
    {{6, 7}, &AdsServiceImpl::MigratePrefsVersion6To7}
  };

  // Cycle through migration paths, i.e. if upgrading from version 2 to 5 we
  // should migrate version 2 to 3, then 3 to 4 and finally version 4 to 5

  int from_version = source_version;
  int to_version = from_version + 1;

  do {
    auto mapping = mappings.find({from_version, to_version});
    if (mapping == mappings.end()) {
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
  // Unlike Muon, ads per day are not configurable in the UI so we can safely
  // migrate to the new value

  #if defined(OS_ANDROID)
    SetUint64Pref(prefs::kAdsPerDay, 12);
  #else
    SetUint64Pref(prefs::kAdsPerDay, 20);
  #endif
}

void AdsServiceImpl::MigratePrefsVersion2To3() {
  const auto locale = GetLocale();
  const auto region = ads::GetRegionCode(locale);

  // Disable ads if upgrading from a pre brave ads build due to a bug where ads
  // were always enabled
  DisableAdsIfUpgradingFromPreBraveAdsBuild();

  // Disable ads for unsupported legacy regions due to a bug where ads were
  // enabled even if the users region was not supported
  const std::vector<std::string> legacy_regions = {
    "US",  // United States of America
    "CA",  // Canada
    "GB",  // United Kingdom (Great Britain and Northern Ireland)
    "DE",  // Germany
    "FR"   // France
  };

  DisableAdsForUnsupportedRegions(region, legacy_regions);

  // On-board users for newly supported regions
  const std::vector<std::string> new_regions = {
    "AU",  // Australia
    "NZ",  // New Zealand
    "IE"   // Ireland
  };

  MayBeShowOnboardingForSupportedRegion(region, new_regions);
}

void AdsServiceImpl::MigratePrefsVersion3To4() {
  const auto locale = GetLocale();
  const auto region = ads::GetRegionCode(locale);

  // Disable ads for unsupported legacy regions due to a bug where ads were
  // enabled even if the users region was not supported
  const std::vector<std::string> legacy_regions = {
    "US",  // United States of America
    "CA",  // Canada
    "GB",  // United Kingdom (Great Britain and Northern Ireland)
    "DE",  // Germany
    "FR",  // France
    "AU",  // Australia
    "NZ",  // New Zealand
    "IE"   // Ireland
  };

  DisableAdsForUnsupportedRegions(region, legacy_regions);

  // On-board users for newly supported regions
  const std::vector<std::string> new_regions = {
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

  MayBeShowOnboardingForSupportedRegion(region, new_regions);
}

void AdsServiceImpl::MigratePrefsVersion4To5() {
  const auto locale = GetLocale();
  const auto region = ads::GetRegionCode(locale);

  // Disable ads for unsupported legacy regions due to a bug where ads were
  // enabled even if the users region was not supported
  const std::vector<std::string> legacy_regions = {
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

  DisableAdsForUnsupportedRegions(region, legacy_regions);

  // On-board users for newly supported regions
  const std::vector<std::string> new_regions = {
    "KY"   // Cayman Islands
  };

  MayBeShowOnboardingForSupportedRegion(region, new_regions);
}

void AdsServiceImpl::MigratePrefsVersion5To6() {
  // Unlike Muon, ads per day are not configurable in the UI so we can safely
  // migrate to the new value

  SetUint64Pref(prefs::kAdsPerDay, 20);
}

void AdsServiceImpl::MigratePrefsVersion6To7() {
  // Disable ads for newly supported regions due to a bug where ads were enabled
  // even if the users region was not supported

  const auto locale = GetLocale();
  const auto region = ads::GetRegionCode(locale);

  const std::vector<std::string> legacy_regions = {
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

  const bool is_a_legacy_region = std::find(legacy_regions.begin(),
      legacy_regions.end(), region) != legacy_regions.end();

  if (is_a_legacy_region) {
    // Do not disable Brave Ads for legacy regions introduced before version
    // 1.3.x
    return;
  }

  const int last_schema_version =
      GetIntegerPref(prefs::kSupportedRegionsLastSchemaVersion);

  if (last_schema_version >= 4) {
    // Do not disable Brave Ads if |prefs::kSupportedRegionsLastSchemaVersion|
    // is newer than or equal to schema version 4. This can occur if a user is
    // upgrading from an older version of 1.3.x or above
    return;
  }

  SetEnabled(false);

  SetBooleanPref(prefs::kShouldShowOnboarding, true);
  SetUint64Pref(prefs::kOnboardingTimestamp, 0);
}

int AdsServiceImpl::GetPrefsVersion() const {
  return GetIntegerPref(prefs::kVersion);
}

bool AdsServiceImpl::IsUpgradingFromPreBraveAdsBuild() {
  // Brave ads was hidden in 0.62.x however due to a bug |prefs::kEnabled| was
  // set to |true| causing "https://github.com/brave/brave-browser/issues/5434"

  // |prefs::kIdleThreshold| was not serialized in 0.62.x

  // |prefs::kVersion| was introduced in 0.63.x

  // We can detect if we are upgrading from a pre Brave ads build by checking
  // |prefs::kEnabled| is set to |true|, |prefs::kIdleThreshold| does not exist,
  // |prefs::kVersion| does not exist and it is not the first time the browser
  // has run for this user
#if !defined(OS_ANDROID)
  return GetBooleanPref(prefs::kEnabled) && !PrefExists(prefs::kIdleThreshold)
      && !PrefExists(prefs::kVersion) && !first_run::IsChromeFirstRun();
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

void AdsServiceImpl::DisableAdsForUnsupportedRegions(
    const std::string& region,
    const std::vector<std::string>& supported_regions) {
  if (std::find(supported_regions.begin(), supported_regions.end(), region)
      != supported_regions.end()) {
    return;
  }

  SetEnabled(false);
}

void AdsServiceImpl::MayBeShowOnboardingForSupportedRegion(
    const std::string& region,
    const std::vector<std::string>& supported_regions) {
  if (IsEnabled()) {
    return;
  }

  if (std::find(supported_regions.begin(), supported_regions.end(), region)
      == supported_regions.end()) {
    return;
  }

  SetBooleanPref(prefs::kShouldShowOnboarding, true);
  SetUint64Pref(prefs::kOnboardingTimestamp, 0);
}

uint64_t AdsServiceImpl::MigrateTimestampToDoubleT(
    const uint64_t timestamp_in_seconds) const {
  if (timestamp_in_seconds < 10000000000) {
    // Already migrated as DoubleT will never reach 10000000000 in our lifetime
    // and legacy timestamps are above 10000000000
    return timestamp_in_seconds;
  }

  // Migrate date to DoubleT
  auto now = base::Time::Now();
  auto now_in_seconds = static_cast<uint64_t>((now - base::Time()).InSeconds());

  auto delta = timestamp_in_seconds - now_in_seconds;

  auto date = now + base::TimeDelta::FromSeconds(delta);
  return static_cast<uint64_t>(date.ToDoubleT());
}

void AdsServiceImpl::MaybeShowOnboarding() {
  if (!ShouldShowOnboarding()) {
    MaybeStartRemoveOnboardingTimer();
    return;
  }

  ShowOnboarding();
}

bool AdsServiceImpl::ShouldShowOnboarding() {
  auto is_ads_enabled = GetBooleanPref(prefs::kEnabled);

  auto is_rewards_enabled =
      GetBooleanPref(brave_rewards::prefs::kBraveRewardsEnabled);

  auto should_show = GetBooleanPref(prefs::kShouldShowOnboarding);

  return IsNewlySupportedLocale() && !is_ads_enabled && is_rewards_enabled
      && should_show;
}

void AdsServiceImpl::ShowOnboarding() {
  auto type = RewardsNotificationService::REWARDS_NOTIFICATION_ADS_ONBOARDING;
  RewardsNotificationService::RewardsNotificationArgs args;
  auto* id = kRewardsNotificationAdsOnboarding;

  auto* notification_service = rewards_service_->GetNotificationService();
  notification_service->AddNotification(type, args, id);

  SetBooleanPref(prefs::kShouldShowOnboarding, false);

  auto now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  SetUint64Pref(prefs::kOnboardingTimestamp, now);

  StartRemoveOnboardingTimer();
}

void AdsServiceImpl::RemoveOnboarding() {
  if (!ShouldRemoveOnboarding()) {
    return;
  }

  onboarding_timer_.Stop();

  auto* notification_service = rewards_service_->GetNotificationService();
  notification_service->DeleteNotification(kRewardsNotificationAdsOnboarding);

  VLOG(1) << "Removed onboarding";
}

void AdsServiceImpl::MaybeStartRemoveOnboardingTimer() {
  if (!ShouldRemoveOnboarding()) {
    return;
  }

  StartRemoveOnboardingTimer();
}

bool AdsServiceImpl::ShouldRemoveOnboarding() const {
  auto* notification_service = rewards_service_->GetNotificationService();
  return notification_service->Exists(kRewardsNotificationAdsOnboarding);
}

void AdsServiceImpl::StartRemoveOnboardingTimer() {
  if (onboarding_timer_.IsRunning()) {
    return;
  }

  auto now_in_seconds = static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  auto timestamp_in_seconds =
      MigrateTimestampToDoubleT(GetUint64Pref(prefs::kOnboardingTimestamp));

  if (IsDebug()) {
    timestamp_in_seconds += 5 * base::Time::kSecondsPerMinute;
  } else {
    timestamp_in_seconds += base::Time::kMicrosecondsPerWeek /
        base::Time::kMicrosecondsPerSecond;
  }

  uint64_t timer_offset_in_seconds;
  if (now_in_seconds >= timestamp_in_seconds) {
    timer_offset_in_seconds = 1 * base::Time::kSecondsPerMinute;
  } else {
    timer_offset_in_seconds = timestamp_in_seconds - now_in_seconds;
  }

  onboarding_timer_.Start(FROM_HERE,
      base::TimeDelta::FromSeconds(timer_offset_in_seconds),
          base::BindOnce(&AdsServiceImpl::RemoveOnboarding, AsWeakPtr()));

  const std::string friendly_date_and_time =
      base::UTF16ToUTF8(base::TimeFormatFriendlyDateAndTime(
          base::Time::FromDoubleT(now_in_seconds + timer_offset_in_seconds)));

  VLOG(1) << "Started timer to remove onboarding " << friendly_date_and_time;
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

bool AdsServiceImpl::ShouldShowMyFirstAdNotification() const {
  auto should_show = GetBooleanPref(prefs::kShouldShowMyFirstAdNotification);
  return IsEnabled() && should_show;
}

bool AdsServiceImpl::GetBooleanPref(
    const std::string& path) const {
  auto* prefs = profile_->GetPrefs();
  const auto value = prefs->GetBoolean(path);

  if (!prefs->HasPrefPath(path)) {
    // If the preference path does not exist then the default value set with
    // RegisterBooleanPref has not been serialized, so we need to serialize the
    // default value
    prefs->SetBoolean(path, value);
  }

  // If the preference path does exist then a value was serialized, so return
  // the serialized value
  return value;
}

void AdsServiceImpl::SetBooleanPref(
    const std::string& path,
    const bool value) {
  profile_->GetPrefs()->SetBoolean(path, value);
}

int AdsServiceImpl::GetIntegerPref(
    const std::string& path) const {
  auto* prefs = profile_->GetPrefs();
  const auto value = prefs->GetInteger(path);

  if (!prefs->HasPrefPath(path)) {
    // If the preference path does not exist then the default value set with
    // RegisterIntegerPref has not been serialized, so we need to serialize the
    // default value
    prefs->SetInteger(path, value);
  }

  // If the preference path does exist then a value was serialized, so return
  // the serialized value
  return value;
}

void AdsServiceImpl::SetIntegerPref(
    const std::string& path,
    const int value) {
  profile_->GetPrefs()->SetInteger(path, value);
}

double AdsServiceImpl::GetDoublePref(
    const std::string& path) const {
  auto* prefs = profile_->GetPrefs();
  const auto value = prefs->GetDouble(path);

  if (!prefs->HasPrefPath(path)) {
    // If the preference path does not exist then the default value set with
    // RegisterDoublePref has not been serialized, so we need to serialize the
    // default value
    prefs->SetDouble(path, value);
  }

  // If the preference path does exist then a value was serialized, so return
  // the serialized value
  return value;
}

void AdsServiceImpl::SetDoublePref(
    const std::string& path,
    const double value) {
  profile_->GetPrefs()->SetDouble(path, value);
}

std::string AdsServiceImpl::GetStringPref(
    const std::string& path) const {
  auto* prefs = profile_->GetPrefs();
  const auto value = prefs->GetString(path);

  if (!prefs->HasPrefPath(path)) {
    // If the preference path does not exist then the default value set with
    // RegisterStringPref has not been serialized, so we need to serialize the
    // default value
    prefs->SetString(path, value);
  }

  // If the preference path does exist then a value was serialized, so return
  // the serialized value
  return value;
}

void AdsServiceImpl::ResetTheWholeState(
    const base::Callback<void(bool)>& callback) {
  SetEnabled(false);
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ResetOnFileTaskRunner,
                     base_path_),
      base::BindOnce(&AdsServiceImpl::OnResetTheWholeState,
                     AsWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::OnResetTheWholeState(
    base::Callback<void(bool)> callback,
    bool success) {
  callback.Run(success);
}

void AdsServiceImpl::SetStringPref(
    const std::string& path,
    const std::string& value) {
  profile_->GetPrefs()->SetString(path, value);
}

int64_t AdsServiceImpl::GetInt64Pref(
    const std::string& path) const {
  auto* prefs = profile_->GetPrefs();
  const auto value = prefs->GetInt64(path);

  if (!prefs->HasPrefPath(path)) {
    // If the preference path does not exist then the default value set with
    // RegisterInt64Pref has not been serialized, so we need to serialize the
    // default value
    prefs->SetInt64(path, value);
  }

  // If the preference path does exist then a value was serialized, so return
  // the serialized value
  return value;
}

void AdsServiceImpl::SetInt64Pref(
    const std::string& path,
    const int64_t value) {
  profile_->GetPrefs()->SetInt64(path, value);
}

uint64_t AdsServiceImpl::GetUint64Pref(
    const std::string& path) const {
  auto* prefs = profile_->GetPrefs();
  const auto value = prefs->GetUint64(path);

  if (!prefs->HasPrefPath(path)) {
    // If the preference path does not exist then the default value set with
    // RegisterUint64Pref has not been serialized, so we need to serialize the
    // default value
    prefs->SetUint64(path, value);
  }

  // If the preference path does exist then a value was serialized, so return
  // the serialized value
  return value;
}

void AdsServiceImpl::SetUint64Pref(
    const std::string& path,
    const uint64_t value) {
  profile_->GetPrefs()->SetUint64(path, value);
}

bool AdsServiceImpl::PrefExists(
    const std::string& path) const {
  return profile_->GetPrefs()->HasPrefPath(path);
}

void AdsServiceImpl::OnPrefsChanged(
    const std::string& pref) {
  if (pref == prefs::kEnabled ||
      pref == brave_rewards::prefs::kBraveRewardsEnabled) {
    if (IsEnabled()) {
#if !defined(OS_ANDROID)
      if (first_run::IsChromeFirstRun()) {
        SetBooleanPref(prefs::kShouldShowOnboarding, false);
      } else {
        RemoveOnboarding();
      }
#endif

      MaybeStart(false);
    } else {
      Stop();
    }

    // Record P3A.
    brave_rewards::UpdateAdsP3AOnPreferenceChange(profile_->GetPrefs(), pref);
  } else if (pref == prefs::kIdleThreshold) {
    StartCheckIdleStateTimer();
  }
}

bool AdsServiceImpl::connected() {
  return bat_ads_.is_bound();
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceImpl::GetClientInfo(ads::ClientInfo* client_info) const {
#if defined(OS_MACOSX)
  client_info->platform = ads::ClientInfoPlatformType::MACOS;
#elif defined(OS_WIN)
  client_info->platform = ads::ClientInfoPlatformType::WINDOWS;
#elif defined(OS_LINUX)
  client_info->platform = ads::ClientInfoPlatformType::LINUX;
#elif defined(OS_ANDROID)
  client_info->platform = ads::ClientInfoPlatformType::ANDROID_OS;
#else
  NOTREACHED();
  client_info->platform = ads::ClientInfoPlatformType::UNKNOWN;
#endif
}

const std::string AdsServiceImpl::GetCountryCode() const {
  const std::string locale = GetLocale();
  const std::string country_code =
      brave_l10n::LocaleHelper::GetInstance()->GetCountryCode(locale);
  return country_code;
}

bool AdsServiceImpl::IsNetworkConnectionAvailable() const {
  return !net::NetworkChangeNotifier::IsOffline();
}

void AdsServiceImpl::SetIdleThreshold(const int threshold) {
  SetIntegerPref(prefs::kIdleThreshold, threshold);
}

bool AdsServiceImpl::IsForeground() const {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  if (!profile_->IsSameProfile(profile)) {
    return false;
  }

  return BackgroundHelper::GetInstance()->IsForeground();
}

std::vector<std::string> AdsServiceImpl::GetUserModelLanguages() const {
  std::vector<std::string> languages;

  for (const auto& user_model_resource_id : g_user_model_resource_ids) {
    languages.push_back(user_model_resource_id.first);
  }

  return languages;
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
    data_resource = resource_bundle.GetRawDataResource(id).as_string();
  }

  return data_resource;
}

void AdsServiceImpl::LoadUserModelForLanguage(
    const std::string& language,
    ads::LoadCallback callback) const {
  const auto resource_id = GetUserModelResourceId(language);
  const auto user_model = LoadDataResourceAndDecompressIfNeeded(resource_id);
  callback(ads::Result::SUCCESS, user_model);
}

void AdsServiceImpl::ShowNotification(
    const std::unique_ptr<ads::AdNotificationInfo> info) {
  auto notification = CreateAdNotification(*info);

  display_service_->Display(NotificationHandler::Type::BRAVE_ADS,
      *notification, /*metadata=*/nullptr);

  StartNotificationTimeoutTimer(info->uuid);
}

void AdsServiceImpl::StartNotificationTimeoutTimer(
    const std::string& uuid) {
#if !defined(OS_ANDROID)
  const uint64_t timeout_in_seconds = 120;

  notification_timers_[uuid] = std::make_unique<base::OneShotTimer>();

  const base::TimeDelta timeout =
      base::TimeDelta::FromSeconds(timeout_in_seconds);

  notification_timers_[uuid]->Start(FROM_HERE, timeout,
      base::BindOnce(&AdsServiceImpl::NotificationTimedOut, AsWeakPtr(), uuid));

  VLOG(1) << "Timeout ad notification with uuid " << uuid << " in "
      << timeout_in_seconds << " seconds";
#endif
}

bool AdsServiceImpl::StopNotificationTimeoutTimer(
    const std::string& uuid) {
  const auto iter = notification_timers_.find(uuid);
  if (iter == notification_timers_.end()) {
    return false;
  }

  notification_timers_.erase(iter);

  return true;
}

bool AdsServiceImpl::ShouldShowNotifications() {
  return NotificationHelper::GetInstance()->ShouldShowNotifications();
}

void AdsServiceImpl::CloseNotification(
    const std::string& uuid) {
#if defined(OS_ANDROID)
  const std::string brave_ads_url_prefix = kBraveAdsUrlPrefix;
  const GURL service_worker_scope =
      GURL(brave_ads_url_prefix.substr(0, brave_ads_url_prefix.size() - 1));
  BraveNotificationPlatformBridgeHelperAndroid::MaybeRegenerateNotification(
      uuid, service_worker_scope);
#endif
  display_service_->Close(NotificationHandler::Type::BRAVE_ADS, uuid);
}

void AdsServiceImpl::SetCatalogIssuers(
    const std::unique_ptr<ads::IssuersInfo> info) {
  rewards_service_->SetCatalogIssuers(info->ToJson());
}

void AdsServiceImpl::ConfirmAd(
    const ads::AdInfo& info,
    const ads::ConfirmationType confirmation_type) {
  rewards_service_->ConfirmAd(info.ToJson(), confirmation_type);
}

void AdsServiceImpl::ConfirmAction(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const ads::ConfirmationType confirmation_type) {
  rewards_service_->ConfirmAction(creative_instance_id, creative_set_id,
      confirmation_type);
}

void AdsServiceImpl::URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ads::URLRequestMethod method,
      ads::URLRequestCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(url);
  request->method = URLMethodToRequestType(method);
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  for (const auto& header : headers) {
    request->headers.AddHeaderFromString(header);
  }

  auto* url_loader = network::SimpleURLLoader::Create(std::move(request),
      GetNetworkTrafficAnnotationTag()).release();

  if (!content.empty()) {
    url_loader->AttachStringForUpload(content, content_type);
  }

  url_loader->SetRetryOptions(kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  url_loaders_.insert(url_loader);

  auto* default_storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(profile_);

  auto* url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess().get();

  url_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory, base::BindOnce(&AdsServiceImpl::OnURLLoaderComplete,
          base::Unretained(this), url_loader, callback));
}

void AdsServiceImpl::Save(
    const std::string& name,
    const std::string& value,
    ads::ResultCallback callback) {
  base::ImportantFileWriter writer(
      base_path_.AppendASCII(name), file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
      base::Closure(),
      base::Bind(
        &PostWriteCallback,
        base::Bind(&AdsServiceImpl::OnSaved, AsWeakPtr(),
            std::move(callback)),
        base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(value));
}

void AdsServiceImpl::Load(
    const std::string& name,
    ads::LoadCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(&AdsServiceImpl::OnLoaded,
                     AsWeakPtr(),
                     std::move(callback)));
}

void AdsServiceImpl::Reset(
    const std::string& name,
    ads::ResultCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ResetOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(&AdsServiceImpl::OnReset,
          AsWeakPtr(), std::move(callback)));
}

std::string AdsServiceImpl::LoadJsonSchema(
    const std::string& name) {
  const auto resource_id = GetSchemaResourceId(name);
  return LoadDataResourceAndDecompressIfNeeded(resource_id);
}

void AdsServiceImpl::SaveBundleState(
    std::unique_ptr<ads::BundleState> bundle_state,
    ads::ResultCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&SaveBundleStateOnFileTaskRunner,
                    base::Passed(std::move(bundle_state)),
                    bundle_state_backend_.get()),
      base::BindOnce(&AdsServiceImpl::OnSaveBundleState,
                     AsWeakPtr(),
                     callback));
}

void AdsServiceImpl::GetCreativeAdNotifications(
    const std::vector<std::string>& categories,
    ads::GetCreativeAdNotificationsCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&GetCreativeAdNotificationsOnFileTaskRunner,
          categories, bundle_state_backend_.get()),
      base::BindOnce(&AdsServiceImpl::OnGetCreativeAdNotifications,
          AsWeakPtr(), std::move(callback), categories));
}

void AdsServiceImpl::GetAdConversions(
    ads::GetAdConversionsCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&GetAdConversionsOnFileTaskRunner,
          bundle_state_backend_.get()),
      base::BindOnce(&AdsServiceImpl::OnGetAdConversions, AsWeakPtr(),
          std::move(callback)));
}

void AdsServiceImpl::Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) const {
  const int vlog_level = ::logging::GetVlogLevelHelper(file, strlen(file));
  if (verbose_level <= vlog_level) {
    ::logging::LogMessage(file, line, -verbose_level).stream() << message;
  }
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
