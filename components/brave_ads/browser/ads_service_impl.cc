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
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/notification_info.h"
#include "bat/ads/notification_event_type.h"
#include "bat/ads/resources/grit/bat_ads_resources.h"
#include "brave/components/brave_ads/browser/ad_notification.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/brave_ads/browser/bundle_state_database.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_ads/common/switches.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/services/bat_ads/public/cpp/ads_client_mojo_bridge.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_impl.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "chrome/browser/profiles/profile.h"
#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#endif
#include "chrome/browser/ui/browser_navigator_params.h"
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
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/android/service_tab_launcher.h"
#include "content/public/browser/page_navigator.h"
#endif

using brave_rewards::RewardsNotificationService;

namespace brave_ads {

namespace {

const char kRewardsNotificationAdsLaunch[] = "rewards_notification_ads_launch";

const unsigned int kRetriesCountOnNetworkChange = 1;

}

class LogStreamImpl : public ads::LogStream {
 public:
  LogStreamImpl(const char* file,
                int line,
                const ads::LogLevel log_level) {
    switch (log_level) {
      case ads::LogLevel::LOG_INFO:
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_INFO);
        break;
      case ads::LogLevel::LOG_WARNING:
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_WARNING);
        break;
      default:
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_ERROR);
    }
  }

  std::ostream& stream() override {
    return log_message_->stream();
  }

 private:
  std::unique_ptr<logging::LogMessage> log_message_;

  DISALLOW_COPY_AND_ASSIGN(LogStreamImpl);
};

class AdsNotificationHandler : public NotificationHandler {
 public:
  explicit AdsNotificationHandler(AdsServiceImpl* ads_service) :
      ads_service_(ads_service->AsWeakPtr()) {
  }

  ~AdsNotificationHandler() override {
  }

  // NotificationHandler implementation.
  void OnShow(
      Profile* profile,
      const std::string& notification_id) override {
    if (ads_service_)
      ads_service_->OnShow(profile, notification_id);
  }

  void OnClose(
      Profile* profile,
      const GURL& origin,
      const std::string& notification_id,
      bool by_user,
      base::OnceClosure completed_closure) override {
    if (!ads_service_) {
      std::move(completed_closure).Run();
      return;
    }

    ads_service_->OnClose(
        profile, origin, notification_id, by_user,
        std::move(completed_closure));
  }

  void OnClick(
      Profile* profile,
      const GURL& origin,
      const std::string& notification_id,
      const base::Optional<int>& action_index,
      const base::Optional<base::string16>& reply,
      base::OnceClosure completed_closure) override {
    if (!ads_service_) {
      return;
    }

    ads_service_->ViewAd(notification_id);
  }

  void DisableNotifications(
      Profile* profile,
      const GURL& origin) override {
  }

  void OpenSettings(Profile* profile, const GURL& origin) override {
    if (!ads_service_) {
      return;
    }

    DCHECK(origin.has_query());

    auto id = origin.query();
    ads_service_->ViewAd(id);
  }

 private:
  base::WeakPtr<AdsServiceImpl> ads_service_;

  DISALLOW_COPY_AND_ASSIGN(AdsNotificationHandler);
};

namespace {

int32_t ToMojomNotificationEventType(ads::NotificationEventType type) {
  return (int32_t)type;
}

static std::map<std::string, int> g_schema_resource_ids = {
  {ads::_catalog_schema_name, IDR_ADS_CATALOG_SCHEMA},
  {ads::_bundle_schema_name, IDR_ADS_BUNDLE_SCHEMA},
};

int GetSchemaResourceId(const std::string& name) {
  if (g_schema_resource_ids.find(name) != g_schema_resource_ids.end())
    return g_schema_resource_ids[name];

  NOTREACHED();
  return 0;
}

static std::map<std::string, int> g_user_model_resource_ids = {
  {"en", IDR_ADS_USER_MODEL_EN},
  {"de", IDR_ADS_USER_MODEL_DE},
  {"fr", IDR_ADS_USER_MODEL_FR},
};

int GetUserModelResourceId(const std::string& locale) {
  if (g_user_model_resource_ids.find(locale) != g_user_model_resource_ids.end())
    return g_user_model_resource_ids[locale];

  NOTREACHED();
  return 0;
}

std::string URLMethodToRequestType(
    ads::URLRequestMethod method) {
  switch (method) {
    case ads::URLRequestMethod::GET:
      return "GET";
    case ads::URLRequestMethod::POST:
      return "POST";
    case ads::URLRequestMethod::PUT:
      return "PUT";
    default:
      NOTREACHED();
      return "GET";
  }
}

void PostWriteCallback(
    const base::Callback<void(bool success)>& callback,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    bool success) {
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
    LOG(ERROR) << "Failed to read file: " << path.MaybeAsASCII();
    return std::string();
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

std::vector<ads::AdInfo> GetAdsForCategoryOnFileTaskRunner(
    const std::string category,
    BundleStateDatabase* backend) {
  std::vector<ads::AdInfo> ads;
  if (!backend)
    return ads;

  backend->GetAdsForCategory(category, &ads);

  return ads;
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

AdsServiceImpl::AdsServiceImpl(Profile* profile)
    : profile_(profile),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      base_path_(profile_->GetPath().AppendASCII("ads_service")),
      is_initialized_(false),
      retry_viewing_ad_with_id_(""),
      next_timer_id_(0),
      ads_launch_id_(0),
      bundle_state_backend_(
          new BundleStateDatabase(base_path_.AppendASCII("bundle_state"))),
      display_service_(NotificationDisplayService::GetForProfile(profile_)),
      rewards_service_(
          brave_rewards::RewardsServiceFactory::GetForProfile(profile_)),
#if !defined(OS_ANDROID)
      last_idle_state_(ui::IdleState::IDLE_STATE_ACTIVE),
      is_foreground_(!!chrome::FindBrowserWithActiveWindow()),
#endif
      bat_ads_client_binding_(new bat_ads::AdsClientMojoBridge(this)) {
  DCHECK(!profile_->IsOffTheRecord());

  profile_pref_change_registrar_.Init(profile_->GetPrefs());

  profile_pref_change_registrar_.Add(prefs::kEnabled,
      base::Bind(&AdsServiceImpl::OnPrefsChanged, base::Unretained(this)));

  profile_pref_change_registrar_.Add(brave_rewards::prefs::kBraveRewardsEnabled,
      base::Bind(&AdsServiceImpl::OnPrefsChanged, base::Unretained(this)));

  profile_pref_change_registrar_.Add(prefs::kIdleThreshold,
      base::Bind(&AdsServiceImpl::OnPrefsChanged, base::Unretained(this)));

  MigratePrefs();

  auto* display_service_impl =
      static_cast<NotificationDisplayServiceImpl*>(display_service_);

  display_service_impl->AddNotificationHandler(
      NotificationHandler::Type::BRAVE_ADS,
      std::make_unique<AdsNotificationHandler>(this));

  MaybeStart(false);
}

AdsServiceImpl::~AdsServiceImpl() {
  file_task_runner_->DeleteSoon(FROM_HERE, bundle_state_backend_.release());
}

void AdsServiceImpl::OnCreate() {
  if (!connected()) {
    return;
  }

  bat_ads_->Initialize(base::BindOnce(
      &AdsServiceImpl::OnInitialize, AsWeakPtr()));
}

void AdsServiceImpl::OnInitialize(const int32_t result) {
  if (result != ads::Result::SUCCESS) {
    LOG(ERROR) << "Failed to initialize ads";
    return;
  }

  is_initialized_ = true;

  MaybeViewAd();

  ResetTimer();
}

void AdsServiceImpl::MaybeViewAd() {
  if (retry_viewing_ad_with_id_.empty()) {
    return;
  }

  ViewAd(retry_viewing_ad_with_id_);
  retry_viewing_ad_with_id_ = "";
}

void AdsServiceImpl::RetryViewingAdWithId(const std::string& id) {
  LOG(WARNING) << "Retry viewing ad with id " << id;
  retry_viewing_ad_with_id_ = id;
}

void AdsServiceImpl::MaybeStart(bool should_restart) {
  if (!IsSupportedRegion()) {
    LOG(INFO) << GetAdsLocale() << " locale does not support Ads";
    Shutdown();
    return;
  }

  if (should_restart) {
    LOG(INFO) << "Restarting ads service";
    Shutdown();
  }

  if (!StartService()) {
    LOG(ERROR) << "Failed to start ads service";
    return;
  }

  if (IsAdsEnabled()) {
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

bool AdsServiceImpl::StartService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!connected());

  content::ServiceManagerConnection* connection =
      content::ServiceManagerConnection::GetForProcess();

  if (!connection) {
    return false;
  }

  connection->GetConnector()->BindInterface(
      bat_ads::mojom::kServiceName, &bat_ads_service_);

  bat_ads_service_.set_connection_error_handler(
      base::Bind(&AdsServiceImpl::MaybeStart, AsWeakPtr(), true));

  UpdateIsProductionFlag();
  UpdateIsDebugFlag();
  UpdateIsTestingFlag();

  return true;
}

#if defined(OS_ANDROID)
void AdsServiceImpl::UpdateIsProductionFlag() {
#if defined(OFFICIAL_BUILD)
  auto is_production = !profile_->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kUseRewardsStagingServer);
#else
  auto is_production = false;
#endif

  bat_ads_service_->SetProduction(is_production, base::NullCallback());
}
#else
void AdsServiceImpl::UpdateIsProductionFlag() {
#if defined(OFFICIAL_BUILD)
  auto is_production = true;
#else
  auto is_production = false;
#endif

  const auto& command_line = *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kStaging)) {
    is_production = false;
  } else if (command_line.HasSwitch(switches::kProduction)) {
    is_production = true;
  }

  bat_ads_service_->SetProduction(is_production, base::NullCallback());
}
#endif

void AdsServiceImpl::UpdateIsDebugFlag() {
#if defined(NDEBUG)
  auto is_debug = false;
#else
  auto is_debug = true;
#endif

  const auto& command_line = *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDebug)) {
    is_debug = true;
  }

  bat_ads_service_->SetDebug(is_debug, base::NullCallback());
}

void AdsServiceImpl::UpdateIsTestingFlag() {
  auto is_testing = false;

  const auto& command_line = *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kTesting)) {
    is_testing = true;
  }

  bat_ads_service_->SetTesting(is_testing, base::NullCallback());
}

void AdsServiceImpl::Start() {
  EnsureBaseDirectoryExists();
}

void AdsServiceImpl::EnsureBaseDirectoryExists() {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&EnsureBaseDirectoryExistsOnFileTaskRunner, base_path_),
      base::Bind(&AdsServiceImpl::OnEnsureBaseDirectoryExists, AsWeakPtr()));
}

void AdsServiceImpl::OnEnsureBaseDirectoryExists(bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to create base directory";
    return;
  }

  BackgroundHelper::GetInstance()->AddObserver(this);

  bat_ads::mojom::BatAdsClientAssociatedPtrInfo client_ptr_info;
  bat_ads_client_binding_.Bind(mojo::MakeRequest(&client_ptr_info));

  bat_ads_service_->Create(std::move(client_ptr_info), MakeRequest(&bat_ads_),
      base::BindOnce(&AdsServiceImpl::OnCreate, AsWeakPtr()));

  MaybeStartFirstLaunchNotificationTimeoutTimer();
  MaybeShowFirstLaunchNotification();

  MaybeShowMyFirstAdNotification();
}

void AdsServiceImpl::MaybeShowMyFirstAdNotification() {
  if (!ShouldShowMyFirstAdNotification()) {
    return;
  }

  if (!NotificationHelper::GetInstance()->ShowMyFirstAdNotification()) {
    return;
  }

  profile_->GetPrefs()->SetBoolean(
      prefs::kShouldShowMyFirstAdNotification, false);
}

bool AdsServiceImpl::ShouldShowMyFirstAdNotification() {
  auto is_ads_enabled = IsAdsEnabled();

  auto should_show = profile_->GetPrefs()->GetBoolean(
      prefs::kShouldShowMyFirstAdNotification);

  return is_ads_enabled && should_show;
}

void AdsServiceImpl::MaybeShowFirstLaunchNotification() {
  if (!ShouldShowFirstLaunchNotification()) {
    return;
  }

  auto now = static_cast<uint64_t>(
      (base::Time::Now() - base::Time()).InSeconds());
  profile_->GetPrefs()->SetUint64(
      prefs::kLastShownFirstLaunchNotificationTimestamp, now);

  ShowFirstLaunchNotification();
}

bool AdsServiceImpl::ShouldShowFirstLaunchNotification() {
  auto should_show = profile_->GetPrefs()->GetBoolean(
      prefs::kShouldShowFirstLaunchNotification);

  return !IsAdsEnabled() && should_show;
}

void AdsServiceImpl::ShowFirstLaunchNotification() {
  auto* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);

  auto* rewards_notification_service =
      rewards_service->GetNotificationService();

  RewardsNotificationService::RewardsNotificationArgs args;
  rewards_notification_service->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_ADS_LAUNCH,
      args, kRewardsNotificationAdsLaunch);

  profile_->GetPrefs()->SetBoolean(
      prefs::kShouldShowFirstLaunchNotification, false);

  StartFirstLaunchNotificationTimeoutTimer();
}

void AdsServiceImpl::MaybeStartFirstLaunchNotificationTimeoutTimer() {
  bool has_removed = profile_->GetPrefs()->GetBoolean(
      prefs::kHasRemovedFirstLaunchNotification);

  if (has_removed) {
    return;
  }

  StartFirstLaunchNotificationTimeoutTimer();
}

void AdsServiceImpl::StartFirstLaunchNotificationTimeoutTimer() {
  uint64_t timer_offset_in_seconds;

  if (HasFirstLaunchNotificationExpired()) {
    timer_offset_in_seconds = base::Time::kSecondsPerMinute;
  } else {
    timer_offset_in_seconds = GetFirstLaunchNotificationTimeoutTimerOffset();
  }

  auto timer_offset = base::TimeDelta::FromSeconds(timer_offset_in_seconds);

  uint32_t timer_id = next_timer_id();
  ads_launch_id_ = timer_id;
  timers_[timer_id] = std::make_unique<base::OneShotTimer>();
  timers_[timer_id]->Start(FROM_HERE,
      timer_offset,
      base::BindOnce(&AdsServiceImpl::OnFirstLaunchNotificationTimedOut,
          AsWeakPtr(),
          timer_id));
}

void AdsServiceImpl::OnFirstLaunchNotificationTimedOut(uint32_t timer_id) {
  timers_.erase(timer_id);
  RemoveFirstLaunchNotification();
}

uint64_t AdsServiceImpl::GetFirstLaunchNotificationTimeoutTimerOffset() {
  auto timeout_in_seconds = GetFirstLaunchNotificationTimeout();

  auto timestamp_in_seconds = profile_->GetPrefs()->GetUint64(
      prefs::kLastShownFirstLaunchNotificationTimestamp);

  auto now_in_seconds = static_cast<uint64_t>(
      (base::Time::Now() - base::Time()).InSeconds());

  auto timer_offset = (timestamp_in_seconds +
      timeout_in_seconds) - now_in_seconds;

  return timer_offset;
}

bool AdsServiceImpl::HasFirstLaunchNotificationExpired() {
  auto timeout_in_seconds = GetFirstLaunchNotificationTimeout();

  auto timestamp_in_seconds = profile_->GetPrefs()->GetUint64(
      prefs::kLastShownFirstLaunchNotificationTimestamp);

  auto now_in_seconds = static_cast<uint64_t>(
      (base::Time::Now() - base::Time()).InSeconds());

  if (now_in_seconds < (timestamp_in_seconds + timeout_in_seconds)) {
    return false;
  }

  return true;
}

uint64_t AdsServiceImpl::GetFirstLaunchNotificationTimeout() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  auto timeout_in_seconds =
      command_line.HasSwitch(switches::kDebug)
      ? (5 * base::Time::kSecondsPerMinute)
      : (base::Time::kMicrosecondsPerWeek /
         base::Time::kMicrosecondsPerSecond);
  return timeout_in_seconds;
}

void AdsServiceImpl::RemoveFirstLaunchNotification() {
  bool has_removed = profile_->GetPrefs()->GetBoolean(
      prefs::kHasRemovedFirstLaunchNotification);

  if (has_removed) {
    return;
  }

  KillTimer(ads_launch_id_);

  auto* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);

  if (!rewards_service) {
    return;
  }

  auto* rewards_notification_service =
      rewards_service->GetNotificationService();

  if (!rewards_notification_service) {
    return;
  }

  rewards_notification_service->DeleteNotification(
      kRewardsNotificationAdsLaunch);

  profile_->GetPrefs()->SetBoolean(
      prefs::kHasRemovedFirstLaunchNotification, true);
}

void AdsServiceImpl::Stop() {
  if (!connected()) {
    return;
  }

  ShutdownBatAds();
}

void AdsServiceImpl::ShutdownBatAds() {
  LOG(INFO) << "Shutting down ads";

  bat_ads_->Shutdown(base::BindOnce(&AdsServiceImpl::OnShutdownBatAds,
      AsWeakPtr()));
}

void AdsServiceImpl::OnShutdownBatAds(const int32_t result) {
  DCHECK(is_initialized_);

  if (result != ads::Result::SUCCESS) {
    LOG(ERROR) << "Failed to shutdown ads";
    return;
  }

  RemoveFirstLaunchNotification();
  Shutdown();
  ResetAllState();

  LOG(INFO) << "Successfully shutdown ads";
}

void AdsServiceImpl::ResetTimer() {
  idle_poll_timer_.Stop();
  idle_poll_timer_.Start(FROM_HERE,
                         base::TimeDelta::FromSeconds(1),
                         this,
                         &AdsServiceImpl::CheckIdleState);
}

void AdsServiceImpl::CheckIdleState() {
#if !defined(OS_ANDROID)
  ProcessIdleState(ui::CalculateIdleState(GetIdleThreshold()));
#endif
}

#if !defined(OS_ANDROID)
void AdsServiceImpl::ProcessIdleState(ui::IdleState idle_state) {
  if (!connected() || idle_state == last_idle_state_)
    return;

  if (idle_state == ui::IdleState::IDLE_STATE_ACTIVE)
    bat_ads_->OnUnIdle();
  else
    bat_ads_->OnIdle();

  last_idle_state_ = idle_state;
}
#endif

void AdsServiceImpl::Shutdown() {
  BackgroundHelper::GetInstance()->RemoveObserver(this);

  for (auto* const loader : url_loaders_) {
    delete loader;
  }
  url_loaders_.clear();

  idle_poll_timer_.Stop();

  bat_ads_.reset();
  bat_ads_client_binding_.Close();

  is_initialized_ = false;
}

void AdsServiceImpl::ResetAllState() {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&ResetOnFileTaskRunner, base_path_),
      base::Bind(&AdsServiceImpl::OnResetAllState, AsWeakPtr()));
}

void AdsServiceImpl::OnResetAllState(bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to reset ads state";
    return;
  }

  LOG(INFO) << "Successfully reset ads state";
}

void AdsServiceImpl::MigratePrefs() const {
  auto source_version = GetPrefsVersion();
  auto dest_version = prefs::kCurrentVersionNumber;

  if (!MigratePrefs(source_version, dest_version, true)) {
    // Migration dry-run failed, so do not migrate preferences
    LOG(ERROR) << "Failed to migrate ads preferences from version "
        << source_version << " to " << dest_version;

    return;
  }

  MigratePrefs(source_version, dest_version);
}

bool AdsServiceImpl::MigratePrefs(
    const int source_version,
    const int dest_version,
    const bool is_dry_run) const {
  DCHECK(source_version <= dest_version) << "Invalid migration path";

  if (source_version == dest_version) {
    if (!is_dry_run) {
      LOG(INFO) << "Ads preferences are up to date on version " << dest_version;
    }

    return true;
  }

  // Migration paths should be added to the below map, i.e.
  //
  //   {{1, 2}, &AdsServiceImpl::MigratePrefsVersion1To2},
  //   {{2, 3}, &AdsServiceImpl::MigratePrefsVersion2To3},
  //   {{3, 4}, &AdsServiceImpl::MigratePrefsVersion3To4}

  static std::map<std::pair<int, int>, void (AdsServiceImpl::*)() const>
      mappings {
    // {{from version, to version}, function}
    {{1, 2}, &AdsServiceImpl::MigratePrefsVersion1To2},
    {{2, 3}, &AdsServiceImpl::MigratePrefsVersion2To3}
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
      LOG(INFO) << "Migrating ads preferences from mapping version "
          << from_version << " to " << to_version;

      (this->*(mapping->second))();
    }

    from_version++;
    if (to_version < dest_version) {
      to_version++;
    }
  } while (from_version != to_version);

  if (!is_dry_run) {
    profile_->GetPrefs()->SetInteger(prefs::kVersion, dest_version);

    LOG(INFO) << "Successfully migrated Ads preferences from version "
        << source_version << " to " << dest_version;
  }

  return true;
}

void AdsServiceImpl::MigratePrefsVersion1To2() const {
  DCHECK_EQ(1, GetPrefsVersion()) << "Invalid migration path";

  // Unlike Muon, ads per day are not configurable in the UI so we can safely
  // migrate to the new value

  #if defined(OS_ANDROID)
    profile_->GetPrefs()->SetUint64(prefs::kAdsPerDay, 12);
    profile_->GetPrefs()->SetUint64(prefs::kAdsPerSameTime, 3);
  #else
    profile_->GetPrefs()->SetUint64(prefs::kAdsPerDay, 20);
  #endif
}

void AdsServiceImpl::MigratePrefsVersion2To3() const {
  DCHECK_EQ(2, GetPrefsVersion()) << "Invalid migration path";

  auto locale = GetAdsLocale();
  auto region = ads::Ads::GetRegion(locale);

  // Disable ads for unsupported legacy regions due to a bug where ads were
  // enabled even if the users region was not supported
  std::vector<std::string> legacy_regions = {
    "US",  // United States of America
    "CA",  // Canada
    "GB",  // United Kingdom (Great Britain and Northern Ireland)
    "DE",  // Germany
    "FR"   // France
  };

  DisableAdsForUnsupportedRegion(region, legacy_regions);

  // On-board users for newly supported regions
  std::vector<std::string> new_regions = {
    "AU",  // Australia
    "NZ",  // New Zealand
    "IE"   // Ireland
  };

  MayBeShowFirstLaunchNotificationForSupportedRegion(region, new_regions);
}

int AdsServiceImpl::GetPrefsVersion() const {
  return profile_->GetPrefs()->GetInteger(prefs::kVersion);
}

void AdsServiceImpl::OnPrefsChanged(const std::string& pref) {
  if (pref == prefs::kEnabled ||
      pref == brave_rewards::prefs::kBraveRewardsEnabled) {
    if (IsAdsEnabled()) {
      MaybeStart(false);
    } else {
      Stop();
    }
  } else if (pref == prefs::kIdleThreshold) {
    ResetTimer();
  }
}

void AdsServiceImpl::DisableAdsForUnsupportedRegion(
    const std::string& region,
    const std::vector<std::string>& supported_regions) const {
  if (std::find(supported_regions.begin(), supported_regions.end(), region)
      != supported_regions.end()) {
    return;
  }

  profile_->GetPrefs()->SetBoolean(prefs::kEnabled, false);
}

void AdsServiceImpl::MayBeShowFirstLaunchNotificationForSupportedRegion(
    const std::string& region,
    const std::vector<std::string>& supported_regions) const {
  if (IsAdsEnabled()) {
    return;
  }

  auto* prefs = profile_->GetPrefs();
  prefs->SetBoolean(prefs::kHasRemovedFirstLaunchNotification, false);
  prefs->SetUint64(prefs::kLastShownFirstLaunchNotificationTimestamp, 0);

  if (std::find(supported_regions.begin(), supported_regions.end(), region)
      == supported_regions.end()) {
    // Do not show first launch notification for unsupported region
  prefs->SetBoolean(prefs::kShouldShowFirstLaunchNotification, false);
    return;
  }

  prefs->SetBoolean(prefs::kShouldShowFirstLaunchNotification, true);
}

bool AdsServiceImpl::IsSupportedRegion() const {
  auto locale = LocaleHelper::GetInstance()->GetLocale();
  return ads::Ads::IsSupportedRegion(locale);
}

bool AdsServiceImpl::IsAdsEnabled() const {
  auto is_ads_enabled = profile_->GetPrefs()->GetBoolean(prefs::kEnabled);
  auto is_rewards_enabled = profile_->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled);

  return is_ads_enabled && is_rewards_enabled;
}

void AdsServiceImpl::SetAdsEnabled(const bool is_enabled) {
  profile_->GetPrefs()->SetBoolean(prefs::kEnabled, is_enabled);
}

uint64_t AdsServiceImpl::GetAdsPerHour() const {
  return profile_->GetPrefs()->GetUint64(prefs::kAdsPerHour);
}

uint64_t AdsServiceImpl::GetAdsPerSameTime() const {
  return profile_->GetPrefs()->GetUint64(prefs::kAdsPerSameTime);
}

void AdsServiceImpl::SetAdsPerHour(const uint64_t ads_per_hour) {
  profile_->GetPrefs()->SetUint64(prefs::kAdsPerHour, ads_per_hour);
}

uint64_t AdsServiceImpl::GetAdsPerDay() const {
  return profile_->GetPrefs()->GetUint64(prefs::kAdsPerDay);
}

bool AdsServiceImpl::IsForeground() const {
  return BackgroundHelper::GetInstance()->IsForeground();
}

void AdsServiceImpl::OnTabUpdated(
    SessionID tab_id,
    const GURL& url,
    const bool is_active) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnTabUpdated(tab_id.id(), url.spec(), is_active,
      profile_->IsOffTheRecord());
}

void AdsServiceImpl::OnTabClosed(SessionID tab_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnTabClosed(tab_id.id());
}

void AdsServiceImpl::SetConfirmationsIsReady(const bool is_ready) {
  if (!connected())
    return;

  bat_ads_->SetConfirmationsIsReady(is_ready);
}

void AdsServiceImpl::ChangeLocale(const std::string& locale) {
  if (!connected()) {
    return;
  }

  bat_ads_->ChangeLocale(locale);
}

void AdsServiceImpl::ClassifyPage(const std::string& url,
                                  const std::string& page) {
  if (!connected())
    return;

  bat_ads_->ClassifyPage(url, page);
}

int AdsServiceImpl::GetIdleThreshold() {
  return profile_->GetPrefs()->GetInteger(prefs::kIdleThreshold);
}

void AdsServiceImpl::SetIdleThreshold(const int threshold) {
  profile_->GetPrefs()->SetInteger(prefs::kIdleThreshold, threshold);
}

bool AdsServiceImpl::IsNotificationsAvailable() const {
#if BUILDFLAG(ENABLE_NATIVE_NOTIFICATIONS)
  return NotificationHelper::GetInstance()->IsNotificationsAvailable();
#else
  return false;
#endif
}

void AdsServiceImpl::LoadUserModelForLocale(
    const std::string& locale,
    ads::OnLoadCallback callback) const {
  base::StringPiece user_model_raw =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          GetUserModelResourceId(locale));

  std::string user_model;
  user_model_raw.CopyToString(&user_model);
  callback(ads::Result::SUCCESS, user_model);
}

void AdsServiceImpl::OnURLsDeleted(
    history::HistoryService* history_service,
    const history::DeletionInfo& deletion_info) {
  if (!connected()) {
    return;
  }

  bat_ads_->RemoveAllHistory(base::BindOnce(
      &AdsServiceImpl::OnRemoveAllHistory, AsWeakPtr()));
}

void AdsServiceImpl::OnRemoveAllHistory(const int32_t result) {
  if (result != ads::Result::SUCCESS) {
    LOG(ERROR) << "Failed to remove all ads history";
    return;
  }

  LOG(INFO) << "Successfully removed all ads history";
}

void AdsServiceImpl::OnMediaStart(SessionID tab_id) {
  if (!connected())
    return;

  bat_ads_->OnMediaPlaying(tab_id.id());
}

void AdsServiceImpl::OnMediaStop(SessionID tab_id) {
  if (!connected())
    return;

  bat_ads_->OnMediaStopped(tab_id.id());
}

void AdsServiceImpl::ShowNotification(
    std::unique_ptr<ads::NotificationInfo> info) {
  auto notification = CreateAdNotification(*info);

  display_service_->Display(NotificationHandler::Type::BRAVE_ADS,
      *notification, /*metadata=*/nullptr);

#if !defined(OS_ANDROID)
  uint32_t timer_id = next_timer_id();

  timers_[timer_id] = std::make_unique<base::OneShotTimer>();
  timers_[timer_id]->Start(FROM_HERE,
      base::TimeDelta::FromSeconds(120),
      base::BindOnce(&AdsServiceImpl::NotificationTimedOut, AsWeakPtr(),
          timer_id, info->id));
#endif
}

void AdsServiceImpl::CloseNotification(const std::string& id) {
  display_service_->Close(NotificationHandler::Type::BRAVE_ADS, id);
}

void AdsServiceImpl::SetCatalogIssuers(std::unique_ptr<ads::IssuersInfo> info) {
  rewards_service_->SetCatalogIssuers(info->ToJson());
}

void AdsServiceImpl::ConfirmAd(std::unique_ptr<ads::NotificationInfo> info) {
  rewards_service_->ConfirmAd(info->ToJson());
}

void AdsServiceImpl::NotificationTimedOut(
    uint32_t timer_id,
    const std::string& notification_id) {
  timers_.erase(timer_id);

  if (!connected()) {
    return;
  }

  CloseNotification(notification_id);

  OnClose(profile_, GURL(), notification_id, false, base::OnceClosure());
}

void AdsServiceImpl::Save(const std::string& name,
                          const std::string& value,
                          ads::OnSaveCallback callback) {
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

void AdsServiceImpl::Load(const std::string& name,
                          ads::OnLoadCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(&AdsServiceImpl::OnLoaded,
                     AsWeakPtr(),
                     std::move(callback)));
}

const std::string AdsServiceImpl::LoadJsonSchema(const std::string& name) {
  base::StringPiece schema_raw =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          GetSchemaResourceId(name));

  std::string schema;
  schema_raw.CopyToString(&schema);
  return schema;
}

void AdsServiceImpl::SaveBundleState(
    std::unique_ptr<ads::BundleState> bundle_state,
    ads::OnSaveCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&SaveBundleStateOnFileTaskRunner,
                    base::Passed(std::move(bundle_state)),
                    bundle_state_backend_.get()),
      base::BindOnce(&AdsServiceImpl::OnSaveBundleState,
                     AsWeakPtr(),
                     callback));
}

void AdsServiceImpl::OnSaveBundleState(const ads::OnSaveCallback& callback,
                                       bool success) {
  if (connected())
    callback(success ? ads::Result::SUCCESS : ads::Result::FAILED);
}

void AdsServiceImpl::OnLoaded(
    const ads::OnLoadCallback& callback,
    const std::string& value) {
  if (!connected())
    return;

  if (value.empty())
    callback(ads::Result::FAILED, value);
  else
    callback(ads::Result::SUCCESS, value);
}

void AdsServiceImpl::OnSaved(
    const ads::OnSaveCallback& callback,
    bool success) {
  if (connected())
    callback(success ? ads::Result::SUCCESS : ads::Result::FAILED);
}

void AdsServiceImpl::Reset(const std::string& name,
                           ads::OnResetCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ResetOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(&AdsServiceImpl::OnReset,
          AsWeakPtr(), std::move(callback)));
}

void AdsServiceImpl::OnReset(const ads::OnResetCallback& callback,
                             bool success) {
  if (connected())
    callback(success ? ads::Result::SUCCESS : ads::Result::FAILED);
}

void AdsServiceImpl::ResetTheWholeState(
    const base::Callback<void(bool)>& callback) {
  SetAdsEnabled(false);
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

void AdsServiceImpl::GetAds(
      const std::string& category,
      ads::OnGetAdsCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&GetAdsForCategoryOnFileTaskRunner,
                    category,
                    bundle_state_backend_.get()),
      base::BindOnce(&AdsServiceImpl::OnGetAdsForCategory,
                     AsWeakPtr(),
                     std::move(callback),
                     category));
}

void AdsServiceImpl::OnGetAdsForCategory(
    const ads::OnGetAdsCallback& callback,
    const std::string& category,
    const std::vector<ads::AdInfo>& ads) {
  if (!connected())
    return;

  callback(ads.empty() ? ads::Result::FAILED : ads::Result::SUCCESS,
      category,
      ads);
}

void AdsServiceImpl::LoadSampleBundle(
    ads::OnLoadSampleBundleCallback callback) {
  base::StringPiece sample_bundle_raw =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_ADS_SAMPLE_BUNDLE);

  std::string sample_bundle;
  sample_bundle_raw.CopyToString(&sample_bundle);
  callback(ads::Result::SUCCESS, sample_bundle);
}

bool AdsServiceImpl::IsNetworkConnectionAvailable() {
  return !net::NetworkChangeNotifier::IsOffline();
}

void AdsServiceImpl::OnShow(
    Profile* profile,
    const std::string& notification_id) {
  if (!connected()) {
    return;
  }

  bat_ads_->OnNotificationEvent(notification_id,
      ToMojomNotificationEventType(ads::NotificationEventType::VIEWED));
}

void AdsServiceImpl::OnClose(
    Profile* profile,
    const GURL& origin,
    const std::string& notification_id,
    bool by_user,
    base::OnceClosure completed_closure) {
  if (connected()) {
    auto event_type = by_user
        ? ads::NotificationEventType::DISMISSED
        : ads::NotificationEventType::TIMEOUT;

    bat_ads_->OnNotificationEvent(notification_id,
        ToMojomNotificationEventType(event_type));
  }

  if (completed_closure) {
    std::move(completed_closure).Run();
  }
}

void AdsServiceImpl::ViewAd(const std::string& id) {
  if (!connected() || !is_initialized_) {
    RetryViewingAdWithId(id);
    return;
  }

  LOG(INFO) << "View ad with id " << id;

  bat_ads_->GetNotificationForId(
      id, base::BindOnce(&AdsServiceImpl::OnViewAd, AsWeakPtr()));
}

void AdsServiceImpl::OnViewAd(const std::string& json) {
  ads::NotificationInfo notification;
  notification.FromJson(json);

  bat_ads_->OnNotificationEvent(notification.id,
      ToMojomNotificationEventType(ads::NotificationEventType::CLICKED));

  OpenNewTabWithUrl(notification.url);
}

void AdsServiceImpl::OpenNewTabWithUrl(const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    LOG(WARNING) << "Invalid URL: " << url;
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

const std::vector<std::string> AdsServiceImpl::GetLocales() const {
  std::vector<std::string> locales;

  for (std::map<std::string, int>::iterator it =
          g_user_model_resource_ids.begin();
        it != g_user_model_resource_ids.end();
        ++it) {
    locales.push_back(it->first);
  }

  return locales;
}

const std::string AdsServiceImpl::GetAdsLocale() const {
  return LocaleHelper::GetInstance()->GetLocale();
}

void AdsServiceImpl::URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      ads::URLRequestMethod method,
      ads::URLRequestCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(url);
  request->method = URLMethodToRequestType(method);
  request->allow_credentials = false;
  for (size_t i = 0; i < headers.size(); i++)
    request->headers.AddHeaderFromString(headers[i]);

  network::SimpleURLLoader* loader =
      network::SimpleURLLoader::Create(
          std::move(request),
          GetNetworkTrafficAnnotationTag()).release();
  url_loaders_.insert(loader);
  loader->SetRetryOptions(kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  if (!content.empty())
    loader->AttachStringForUpload(content, content_type);

  loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      content::BrowserContext::GetDefaultStoragePartition(profile_)
          ->GetURLLoaderFactoryForBrowserProcess().get(),
      base::BindOnce(&AdsServiceImpl::OnURLLoaderComplete,
                     base::Unretained(this),
                     loader,
                     callback));
}

void AdsServiceImpl::OnURLLoaderComplete(
    network::SimpleURLLoader* loader,
    ads::URLRequestCallback callback,
    std::unique_ptr<std::string> response_body) {
  DCHECK(url_loaders_.find(loader) != url_loaders_.end());
  url_loaders_.erase(loader);
  std::unique_ptr<network::SimpleURLLoader> scoped_loader(loader);

  int response_code = -1;
  if (loader->ResponseInfo() && loader->ResponseInfo()->headers)
    response_code = loader->ResponseInfo()->headers->response_code();

  std::map<std::string, std::string> headers;
  if (loader->ResponseInfo()) {
    scoped_refptr<net::HttpResponseHeaders> headersList =
        loader->ResponseInfo()->headers;

    if (headersList) {
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headersList->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
      }
    }
  }

  if (connected()) {
    callback(response_code,
             response_body ? *response_body : std::string(),
             headers);
  }
}

void AdsServiceImpl::OnBackground() {
  if (connected()) {
    bat_ads_->OnBackground();
  }
}

void AdsServiceImpl::OnForeground() {
  if (connected()) {
    bat_ads_->OnForeground();
  }
}

void AdsServiceImpl::EventLog(const std::string& json) {
  VLOG(0) << "AdsService Event Log: " << json;
}

uint32_t AdsServiceImpl::next_timer_id() {
  if (next_timer_id_ == std::numeric_limits<uint32_t>::max())
    next_timer_id_ = 1;
  else
    ++next_timer_id_;
  return next_timer_id_;
}

uint32_t AdsServiceImpl::SetTimer(const uint64_t time_offset) {
  uint32_t timer_id = next_timer_id();

  timers_[timer_id] = std::make_unique<base::OneShotTimer>();
  timers_[timer_id]->Start(FROM_HERE,
      base::TimeDelta::FromSeconds(time_offset),
      base::BindOnce(&AdsServiceImpl::OnTimer, AsWeakPtr(), timer_id));

  return timer_id;
}

void AdsServiceImpl::KillTimer(uint32_t timer_id) {
  if (timers_.find(timer_id) == timers_.end())
    return;

  timers_[timer_id]->Stop();
  timers_.erase(timer_id);
}

void AdsServiceImpl::OnTimer(uint32_t timer_id) {
  if (!connected())
    return;

  timers_.erase(timer_id);
  bat_ads_->OnTimer(timer_id);
}

std::unique_ptr<ads::LogStream> AdsServiceImpl::Log(
    const char* file,
    int line,
    const ads::LogLevel log_level) const {
  return std::make_unique<LogStreamImpl>(file, line, log_level);
}

bool AdsServiceImpl::connected() {
  return bat_ads_.is_bound();
}

}  // namespace brave_ads
