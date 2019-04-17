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
#include "bat/ads/notification_result_type.h"
#include "bat/ads/resources/grit/bat_ads_resources.h"
#include "brave/components/brave_ads/browser/ad_notification.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/brave_ads/browser/bundle_state_database.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_ads/common/switches.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/services/bat_ads/public/cpp/ads_client_mojo_bridge.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_impl.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_constants.h"
#include "components/prefs/pref_service.h"
#include "components/wifi/wifi_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/service_manager_connection.h"
#include "net/url_request/url_fetcher.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/dom_distiller_js/dom_distiller.pb.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/public/cpp/notification.h"

#if defined(OS_ANDROID)
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "net/android/network_library.h"
#endif

namespace brave_ads {

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
      ads_service_(ads_service->AsWeakPtr()) {}

  ~AdsNotificationHandler() override {}

  // NotificationHandler implementation.
  void OnShow(Profile* profile,
              const std::string& notification_id) override {
    if (ads_service_)
      ads_service_->OnShow(profile, notification_id);
  }

  void OnClose(Profile* profile,
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

  void OnClick(Profile* profile,
               const GURL& origin,
               const std::string& notification_id,
               const base::Optional<int>& action_index,
               const base::Optional<base::string16>& reply,
               base::OnceClosure completed_closure) override {
    if (ads_service_ && !action_index.has_value()) {
      ads_service_->OpenSettings(profile, origin, true);
    }
  }

  void DisableNotifications(Profile* profile,
                            const GURL& origin) override {}


  void OpenSettings(Profile* profile, const GURL& origin) override {
    if (ads_service_)
      ads_service_->OpenSettings(profile, origin, false);
  }

 private:
  base::WeakPtr<AdsServiceImpl> ads_service_;

  DISALLOW_COPY_AND_ASSIGN(AdsNotificationHandler);
};

namespace {

int32_t ToMojomNotificationResultInfoResultType(
    ads::NotificationResultInfoResultType result_type) {
  return (int32_t)result_type;
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
  {"de", IDR_ADS_USER_MODEL_DE},
  {"fr", IDR_ADS_USER_MODEL_FR},
  {"en", IDR_ADS_USER_MODEL_EN},
};

int GetUserModelResourceId(const std::string& locale) {
  if (g_user_model_resource_ids.find(locale) != g_user_model_resource_ids.end())
    return g_user_model_resource_ids[locale];

  NOTREACHED();
  return 0;
}

net::URLFetcher::RequestType URLMethodToRequestType(
    ads::URLRequestMethod method) {
  switch (method) {
    case ads::URLRequestMethod::GET:
      return net::URLFetcher::RequestType::GET;
    case ads::URLRequestMethod::POST:
      return net::URLFetcher::RequestType::POST;
    case ads::URLRequestMethod::PUT:
      return net::URLFetcher::RequestType::PUT;
    default:
      NOTREACHED();
      return net::URLFetcher::RequestType::GET;
  }
}

void EnsureBaseDirectoryExists(const base::FilePath& path) {
  if (!DirectoryExists(path))
    base::CreateDirectory(path);
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

std::vector<ads::AdInfo> GetAdsForCategoryOnFileTaskRunner(
    const std::string category,
    BundleStateDatabase* backend) {
  std::vector<ads::AdInfo> ads;
  if (!backend)
    return ads;

  backend->GetAdsForCategory(category, &ads);

  return ads;
}

bool ResetOnFileTaskRunner(
    const base::FilePath& path) {
  return base::DeleteFile(path, false);
}

bool SaveBundleStateOnFileTaskRunner(
    std::unique_ptr<ads::BundleState> bundle_state,
    BundleStateDatabase* backend) {
  if (backend && backend->SaveBundleState(*bundle_state))
    return true;

  return false;
}

}  // namespace

AdsServiceImpl::AdsServiceImpl(Profile* profile)
    : profile_(profile),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      base_path_(profile_->GetPath().AppendASCII("ads_service")),
      next_timer_id_(0),
      is_supported_region_(false),
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

  file_task_runner_->PostTask(FROM_HERE,
      base::BindOnce(&EnsureBaseDirectoryExists, base_path_));
  profile_pref_change_registrar_.Init(profile_->GetPrefs());
  profile_pref_change_registrar_.Add(
      prefs::kBraveAdsEnabled,
      base::Bind(&AdsServiceImpl::OnPrefsChanged,
                 base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      brave_rewards::prefs::kBraveRewardsEnabled,
      base::Bind(&AdsServiceImpl::OnPrefsChanged,
                 base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kBraveAdsIdleThreshold,
      base::Bind(&AdsServiceImpl::OnPrefsChanged,
                 base::Unretained(this)));

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

void AdsServiceImpl::OnInitialize() {
  ResetTimer();
}

void AdsServiceImpl::OnCreate() {
  if (connected()) {
    bat_ads_->Initialize(
        base::BindOnce(&AdsServiceImpl::OnInitialize, AsWeakPtr()));
  }
}

void AdsServiceImpl::MaybeStart(bool should_restart) {
  if (should_restart)
    Shutdown();

  if (!StartService()) {
    LOG(ERROR) << "Failed to start Ads service";
    return;
  }

  bat_ads_service_->IsSupportedRegion(GetAdsLocale(),
      base::BindOnce(&AdsServiceImpl::OnMaybeStartForRegion,
          AsWeakPtr(), should_restart));
}

void AdsServiceImpl::OnMaybeStartForRegion(
    bool should_restart,
    bool is_supported_region) {
  is_supported_region_ = is_supported_region;

  if (!is_supported_region_) {
    Shutdown();
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

  if (!connection)
    return false;

  connection->GetConnector()->BindInterface(
      bat_ads::mojom::kServiceName, &bat_ads_service_);

  bat_ads_service_.set_connection_error_handler(
      base::Bind(&AdsServiceImpl::MaybeStart, AsWeakPtr(), true));

  bool is_production = false;
#if defined(OFFICIAL_BUILD)
  is_production = true;
#endif
  bool is_debug = true;
#if defined(NDEBUG)
  is_debug = false;
#endif
  bool is_testing = false;

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kStaging)) {
    is_production = false;
  }
  if (command_line.HasSwitch(switches::kProduction)) {
    is_production = true;
  }
  if (command_line.HasSwitch(switches::kDebug)) {
    is_debug = true;
  }
  if (command_line.HasSwitch(switches::kTesting)) {
    is_testing = true;
  }

  bat_ads_service_->SetProduction(is_production, base::NullCallback());
  bat_ads_service_->SetDebug(is_debug, base::NullCallback());
  bat_ads_service_->SetTesting(is_testing, base::NullCallback());

  return true;
}

void AdsServiceImpl::Start() {
  BackgroundHelper::GetInstance()->AddObserver(this);

  bat_ads::mojom::BatAdsClientAssociatedPtrInfo client_ptr_info;
  bat_ads_client_binding_.Bind(mojo::MakeRequest(&client_ptr_info));

  bat_ads_service_->Create(std::move(client_ptr_info), MakeRequest(&bat_ads_),
      base::BindOnce(&AdsServiceImpl::OnCreate, AsWeakPtr()));
}

void AdsServiceImpl::Stop() {
  if (connected()) {
    // this is kind of weird, but we need to call Initialize on disable too
    bat_ads_->Initialize(base::NullCallback());
  }
  Shutdown();
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

  for (const auto fetcher : fetchers_) {
    delete fetcher.first;
  }
  fetchers_.clear();
  idle_poll_timer_.Stop();

  bat_ads_.reset();
  bat_ads_client_binding_.Close();

  for (NotificationInfoMap::iterator it = notification_ids_.begin();
      it != notification_ids_.end(); ++it) {
    const std::string notification_id = it->first;
    display_service_->Close(NotificationHandler::Type::BRAVE_ADS,
                            notification_id);
  }
  notification_ids_.clear();
}

void AdsServiceImpl::MigratePrefs() const {
  auto source_version = GetPrefsVersion();
  auto dest_version = prefs::kBraveAdsPrefsCurrentVersion;

  if (!MigratePrefs(source_version, dest_version, true)) {
    // Migration dry-run failed, so do not migrate preferences
    LOG(ERROR) << "Failed to migrate Ads preferences from version "
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
    {{1, 2}, &AdsServiceImpl::MigratePrefsVersion1To2}
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
      LOG(INFO) << "Migrating Ads preferences from mapping version "
          << from_version << " to " << to_version;

      (this->*(mapping->second))();

      profile_->GetPrefs()->SetInteger(prefs::kBraveAdsPrefsVersion,
          to_version);
    }

    from_version++;
    if (to_version < dest_version) {
      to_version++;
    }
  } while (from_version != to_version);

  if (!is_dry_run) {
    LOG(INFO) << "Successfully migrated Ads preferences from version "
        << source_version << " to " << dest_version;
  }

  return true;
}

void AdsServiceImpl::MigratePrefsVersion1To2() const {
  DCHECK_EQ(1, GetPrefsVersion()) << "Invalid migration path";

  // Unlike Muon, Ads per day are not configurable in the UI so we can safely
  // migrate to the new value

  #if defined(OS_ANDROID)
    profile_->GetPrefs()->SetUint64(prefs::kBraveAdsPerDay, 12);
  #else
    profile_->GetPrefs()->SetUint64(prefs::kBraveAdsPerDay, 20);
  #endif
}

int AdsServiceImpl::GetPrefsVersion() const {
  return profile_->GetPrefs()->GetInteger(prefs::kBraveAdsPrefsVersion);
}

void AdsServiceImpl::OnPrefsChanged(const std::string& pref) {
  if (pref == prefs::kBraveAdsEnabled ||
      pref == brave_rewards::prefs::kBraveRewardsEnabled) {
    if (IsAdsEnabled()) {
      MaybeStart(false);
    } else {
      Stop();
    }
  } else if (pref == prefs::kBraveAdsIdleThreshold) {
    ResetTimer();
  }
}

bool AdsServiceImpl::IsSupportedRegion() const {
  return is_supported_region_;
}

bool AdsServiceImpl::IsAdsEnabled() const {
  auto ads_enabled = profile_->GetPrefs()->GetBoolean(
      prefs::kBraveAdsEnabled);

  auto prefs_migrated_from_62 = profile_->GetPrefs()->GetBoolean(
      prefs::kBraveAdsPrefsMigratedFrom62);

  if (ads_enabled && prefs_migrated_from_62) {
    // Disable Ads by default when upgrading from 0.62.x to 0.63.x
    ads_enabled = false;

    profile_->GetPrefs()->SetBoolean(
        prefs::kBraveAdsEnabled, ads_enabled);

    profile_->GetPrefs()->SetBoolean(
        prefs::kBraveAdsPrefsMigratedFrom62, false);
  }

  auto rewards_enabled = profile_->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled);

  return IsSupportedRegion() && ads_enabled && rewards_enabled;
}

void AdsServiceImpl::SetAdsEnabled(const bool is_enabled) {
  profile_->GetPrefs()->SetBoolean(prefs::kBraveAdsEnabled, is_enabled);
}

uint64_t AdsServiceImpl::GetAdsPerHour() const {
  return profile_->GetPrefs()->GetUint64(prefs::kBraveAdsPerHour);
}

void AdsServiceImpl::SetAdsPerHour(const uint64_t ads_per_hour) {
  profile_->GetPrefs()->SetUint64(prefs::kBraveAdsPerHour, ads_per_hour);
}

uint64_t AdsServiceImpl::GetAdsPerDay() const {
  return profile_->GetPrefs()->GetUint64(prefs::kBraveAdsPerDay);
}

bool AdsServiceImpl::IsForeground() const {
  return BackgroundHelper::GetInstance()->IsForeground();
}

void AdsServiceImpl::TabUpdated(SessionID tab_id,
                                const GURL& url,
                                const bool is_active) {
  if (!connected())
    return;

  bat_ads_->TabUpdated(tab_id.id(),
                   url.spec(),
                   is_active,
                   profile_->IsOffTheRecord());
}

void AdsServiceImpl::TabClosed(SessionID tab_id) {
  if (!connected())
    return;

  bat_ads_->TabClosed(tab_id.id());
}

void AdsServiceImpl::SetConfirmationsIsReady(const bool is_ready) {
  if (!connected())
    return;

  bat_ads_->SetConfirmationsIsReady(is_ready);
}

void AdsServiceImpl::ClassifyPage(const std::string& url,
                                  const std::string& page) {
  if (!connected())
    return;

  bat_ads_->ClassifyPage(url, page);
}

int AdsServiceImpl::GetIdleThreshold() {
  return profile_->GetPrefs()->GetInteger(prefs::kBraveAdsIdleThreshold);
}

void AdsServiceImpl::SetIdleThreshold(const int threshold) {
  profile_->GetPrefs()->SetInteger(prefs::kBraveAdsIdleThreshold, threshold);
}

bool AdsServiceImpl::IsNotificationsAvailable() const {
#if BUILDFLAG(ENABLE_NATIVE_NOTIFICATIONS)
  return true;
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

void AdsServiceImpl::OnURLsDeleted(history::HistoryService* history_service,
                                   const history::DeletionInfo& deletion_info) {
  if (!connected())
    return;

  bat_ads_->RemoveAllHistory(base::NullCallback());
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
  std::string notification_id;
  auto notification =
      CreateAdNotification(*info, &notification_id);

  notification_ids_[notification_id] = std::move(info);

  display_service_->Display(NotificationHandler::Type::BRAVE_ADS,
                            *notification);

  uint32_t timer_id = next_timer_id();

  timers_[timer_id] = std::make_unique<base::OneShotTimer>();
  timers_[timer_id]->Start(FROM_HERE,
      base::TimeDelta::FromSeconds(120),
      base::BindOnce(
          &AdsServiceImpl::NotificationTimedOut, AsWeakPtr(),
              timer_id, notification_id));
}

void AdsServiceImpl::SetCatalogIssuers(std::unique_ptr<ads::IssuersInfo> info) {
  rewards_service_->SetCatalogIssuers(info->ToJson());
}

void AdsServiceImpl::ConfirmAd(std::unique_ptr<ads::NotificationInfo> info) {
  rewards_service_->ConfirmAd(info->ToJson());
}

void AdsServiceImpl::NotificationTimedOut(uint32_t timer_id,
                                          const std::string& notification_id) {
  timers_.erase(timer_id);
  if (notification_ids_.find(notification_id) != notification_ids_.end()) {
    display_service_->Close(NotificationHandler::Type::BRAVE_ADS,
                            notification_id);
    OnClose(profile_, GURL(), notification_id, false, base::OnceClosure());
  }
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
                     AsWeakPtr(),
                     std::move(callback)));
}

void AdsServiceImpl::OnReset(const ads::OnResetCallback& callback,
                             bool success) {
  if (connected())
    callback(success ? ads::Result::SUCCESS : ads::Result::FAILED);
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
  return !content::GetNetworkConnectionTracker()->IsOffline();
}

void AdsServiceImpl::OnShow(Profile* profile,
                            const std::string& notification_id) {
  if (!connected() ||
      notification_ids_.find(notification_id) == notification_ids_.end())
    return;

  bat_ads_->GenerateAdReportingNotificationShownEvent(
      notification_ids_[notification_id]->ToJson());
}

void AdsServiceImpl::OnClose(Profile* profile,
                             const GURL& origin,
                             const std::string& notification_id,
                             bool by_user,
                             base::OnceClosure completed_closure) {
  if (notification_ids_.find(notification_id) != notification_ids_.end()) {
    auto notification_info = base::WrapUnique(
        notification_ids_[notification_id].release());
    notification_ids_.erase(notification_id);

    if (connected()) {
      auto result_type = by_user
          ? ads::NotificationResultInfoResultType::DISMISSED
          : ads::NotificationResultInfoResultType::TIMEOUT;
      bat_ads_->GenerateAdReportingNotificationResultEvent(
          notification_info->ToJson(),
          ToMojomNotificationResultInfoResultType(result_type));
    }
  }

  if (completed_closure)
    std::move(completed_closure).Run();
}

void AdsServiceImpl::OpenSettings(Profile* profile,
                                  const GURL& origin,
                                  bool should_close) {
  DCHECK(origin.has_query());
  auto notification_id = origin.query();

  if (notification_ids_.find(notification_id) == notification_ids_.end())
    return;

  auto notification_info = base::WrapUnique(
      notification_ids_[notification_id].release());
  notification_ids_.erase(notification_id);

  if (should_close)
    display_service_->Close(NotificationHandler::Type::BRAVE_ADS,
                            notification_id);

  if (connected()) {
    bat_ads_->GenerateAdReportingNotificationResultEvent(
        notification_info->ToJson(),
        ToMojomNotificationResultInfoResultType(
            ads::NotificationResultInfoResultType::CLICKED));
  }

  GURL url(notification_info->url);
  if (!url.is_valid()) {
    LOG(WARNING) << "Invalid notification URL: " << notification_info->url;
    return;
  }

#if defined(OS_ANDROID)
  NavigateParams nav_params(profile, url, ui::PAGE_TRANSITION_LINK);
#else
  Browser* browser = chrome::FindTabbedBrowser(profile, false);
  if (!browser)
    browser = new Browser(Browser::CreateParams(profile, true));

  NavigateParams nav_params(browser, url, ui::PAGE_TRANSITION_LINK);
#endif
  nav_params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  nav_params.window_action = NavigateParams::SHOW_WINDOW;
#if defined(OS_ANDROID)
  TabModelList::HandlePopupNavigation(&nav_params);
#else
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

const std::string AdsServiceImpl::GenerateUUID() const {
  return base::GenerateGUID();
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
  net::URLFetcher::RequestType request_type = URLMethodToRequestType(method);

  net::URLFetcher* fetcher = net::URLFetcher::Create(
      GURL(url), request_type, this).release();
  fetcher->SetRequestContext(g_browser_process->system_request_context());

  for (size_t i = 0; i < headers.size(); i++)
    fetcher->AddExtraRequestHeader(headers[i]);

  if (!content.empty())
    fetcher->SetUploadData(content_type, content);

  fetchers_[fetcher] = callback;

  fetcher->Start();
}

void AdsServiceImpl::OnURLFetchComplete(
    const net::URLFetcher* source) {
  if (fetchers_.find(source) == fetchers_.end()) {
    delete source;
    return;
  }

  auto callback = fetchers_[source];
  fetchers_.erase(source);
  int response_code = source->GetResponseCode();
  std::string body;
  std::map<std::string, std::string> headers;
  scoped_refptr<net::HttpResponseHeaders> headersList =
      source->GetResponseHeaders();

  if (headersList) {
    size_t iter = 0;
    std::string key;
    std::string value;
    while (headersList->EnumerateHeaderLines(&iter, &key, &value)) {
      key = base::ToLowerASCII(key);
      headers[key] = value;
    }
  }

  if (response_code != net::URLFetcher::ResponseCode::RESPONSE_CODE_INVALID &&
      source->GetStatus().is_success()) {
    source->GetResponseAsString(&body);
  }

  delete source;

  if (connected())
    callback(response_code, body, headers);
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
