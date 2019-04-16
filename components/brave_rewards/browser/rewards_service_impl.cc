/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service_impl.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/guid.h"
#include "base/i18n/time_formatting.h"
#include "base/time/time.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/auto_contribute_props.h"
#include "bat/ledger/media_publisher_info.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/wallet_info.h"
#include "bat/ledger/transactions_info.h"
#include "brave/browser/ui/webui/brave_rewards_source.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_rewards/browser/auto_contribution_props.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
#include "brave/components/brave_rewards/browser/publisher_info_database.h"
#include "brave/components/brave_rewards/browser/rewards_fetcher_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/switches.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/services/bat_ledger/public/cpp/ledger_client_mojo_proxy.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/map.h"
#include "net/base/escape.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"
#include "url/url_canon_stdstring.h"

#if !defined(OS_ANDROID)
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "components/grit/brave_components_resources.h"
#else
#include "components/grit/components_resources.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/components/brave_rewards/browser/extension_rewards_service_observer.h"
#endif

using net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES;
using std::placeholders::_1;
using std::placeholders::_2;

namespace brave_rewards {

class LogStreamImpl : public ledger::LogStream {
 public:
  LogStreamImpl(const char* file,
                int line,
                const ledger::LogLevel log_level) {
    logging::LogSeverity severity;

    switch (log_level) {
      case ledger::LogLevel::LOG_INFO:
        severity = logging::LOG_INFO;
        break;
      case ledger::LogLevel::LOG_WARNING:
        severity = logging::LOG_WARNING;
        break;
      case ledger::LogLevel::LOG_ERROR:
        severity = logging::LOG_ERROR;
        break;
      default:
        severity = logging::LOG_VERBOSE;
        break;
    }

    log_message_ = std::make_unique<logging::LogMessage>(file, line, severity);
  }

  LogStreamImpl(const char* file,
                int line,
                int log_level) {
    // VLOG has negative log level
    log_message_ =
        std::make_unique<logging::LogMessage>(file, line, -log_level);
  }

  std::ostream& stream() override {
    return log_message_->stream();
  }

 private:
  std::unique_ptr<logging::LogMessage> log_message_;
  DISALLOW_COPY_AND_ASSIGN(LogStreamImpl);
};

namespace {

ledger::ACTIVITY_MONTH GetPublisherMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return (ledger::ACTIVITY_MONTH)exploded.month;
}

int GetPublisherYear(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return exploded.year;
}

ContentSite PublisherInfoToContentSite(
    const ledger::PublisherInfo& publisher_info) {
  ContentSite content_site(publisher_info.id);
  content_site.percentage = publisher_info.percent;
  content_site.verified = publisher_info.verified;
  content_site.excluded = publisher_info.excluded;
  content_site.name = publisher_info.name;
  content_site.url = publisher_info.url;
  content_site.provider = publisher_info.provider;
  content_site.favicon_url = publisher_info.favicon_url;
  content_site.id = publisher_info.id;
  content_site.weight = publisher_info.weight;
  content_site.reconcile_stamp = publisher_info.reconcile_stamp;
  return content_site;
}

net::URLFetcher::RequestType URLMethodToRequestType(ledger::URL_METHOD method) {
  switch (method) {
    case ledger::URL_METHOD::GET:
      return net::URLFetcher::RequestType::GET;
    case ledger::URL_METHOD::POST:
      return net::URLFetcher::RequestType::POST;
    case ledger::URL_METHOD::PUT:
      return net::URLFetcher::RequestType::PUT;
    default:
      NOTREACHED();
      return net::URLFetcher::RequestType::GET;
  }
}

std::string LoadStateOnFileTaskRunner(
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

bool SaveMediaPublisherInfoOnFileTaskRunner(
    const std::string& media_key,
    const std::string& publisher_id,
    PublisherInfoDatabase* backend) {
  if (backend && backend->InsertOrUpdateMediaPublisherInfo(media_key,
        publisher_id))
    return true;

  return false;
}

std::unique_ptr<ledger::PublisherInfo>
LoadPublisherInfoOnFileTaskRunner(
    const std::string publisher_key,
    PublisherInfoDatabase* backend) {
  if (!backend)
    return nullptr;

  return backend->GetPublisherInfo(publisher_key);
}

std::unique_ptr<ledger::PublisherInfo>
LoadMediaPublisherInfoOnFileTaskRunner(
    const std::string media_key,
    PublisherInfoDatabase* backend) {
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!backend)
    return info;

  return backend->GetMediaPublisherInfo(media_key);
}

bool SavePublisherInfoOnFileTaskRunner(
    const ledger::PublisherInfo publisher_info,
    PublisherInfoDatabase* backend) {
  if (backend && backend->InsertOrUpdatePublisherInfo(publisher_info))
    return true;

  return false;
}

bool SaveActivityInfoOnFileTaskRunner(
    const ledger::PublisherInfo publisher_info,
    PublisherInfoDatabase* backend) {
  if (backend && backend->InsertOrUpdateActivityInfo(publisher_info))
    return true;

  return false;
}

ledger::PublisherInfoList GetActivityListOnFileTaskRunner(
    uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilter filter,
    PublisherInfoDatabase* backend) {
  ledger::PublisherInfoList list;
  if (!backend)
    return list;

  ignore_result(backend->GetActivityList(start, limit, filter, &list));
  return list;
}

std::unique_ptr<ledger::PublisherInfo> GetPanelPublisherInfoOnFileTaskRunner(
    ledger::ActivityInfoFilter filter,
    PublisherInfoDatabase* backend) {
  ledger::PublisherInfoList list;
  if (!backend) {
    return nullptr;
  }

  return backend->GetPanelPublisher(filter);
}

// `callback` has a WeakPtr so this won't crash if the file finishes
// writing after RewardsServiceImpl has been destroyed
void PostWriteCallback(
    const base::Callback<void(bool success)>& callback,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    bool write_success) {
  // We can't run |callback| on the current thread. Bounce back to
  // the |reply_task_runner| which is the correct sequenced thread.
  reply_task_runner->PostTask(FROM_HERE,
                              base::Bind(callback, write_success));
}

time_t GetCurrentTimestamp() {
  return base::Time::NowFromSystemTime().ToTimeT();
}

std::string LoadOnFileTaskRunner(const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    LOG(ERROR) << "Failed to read file: " << path.MaybeAsASCII();
    return std::string();
  }
  return data;
}

bool ResetOnFileTaskRunner(const base::FilePath& path) {
  return base::DeleteFile(path, false);
}

void EnsureRewardsBaseDirectoryExists(const base::FilePath& path) {
  if (!DirectoryExists(path))
    base::CreateDirectory(path);
}

}  // namespace

bool IsMediaLink(const GURL& url,
                 const GURL& first_party_url,
                 const GURL& referrer) {
  return ledger::Ledger::IsMediaLink(url.spec(),
                                     first_party_url.spec(),
                                     referrer.spec());
}


// read comment about file pathes at src\base\files\file_path.h
#if defined(OS_WIN)
const base::FilePath::StringType kLedger_state(L"ledger_state");
const base::FilePath::StringType kPublisher_state(L"publisher_state");
const base::FilePath::StringType kPublisher_info_db(L"publisher_info_db");
const base::FilePath::StringType kPublishers_list(L"publishers_list");
const base::FilePath::StringType kRewardsStatePath(L"rewards_service");
#else
const base::FilePath::StringType kLedger_state("ledger_state");
const base::FilePath::StringType kPublisher_state("publisher_state");
const base::FilePath::StringType kPublisher_info_db("publisher_info_db");
const base::FilePath::StringType kPublishers_list("publishers_list");
const base::FilePath::StringType kRewardsStatePath("rewards_service");
#endif

RewardsServiceImpl::RewardsServiceImpl(Profile* profile)
    : profile_(profile),
      bat_ledger_client_binding_(new bat_ledger::LedgerClientMojoProxy(this)),
#if BUILDFLAG(ENABLE_EXTENSIONS)
      extension_rewards_service_observer_(
          std::make_unique<ExtensionRewardsServiceObserver>(profile_)),
#endif
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ledger_state_path_(profile_->GetPath().Append(kLedger_state)),
      publisher_state_path_(profile_->GetPath().Append(kPublisher_state)),
      publisher_info_db_path_(profile->GetPath().Append(kPublisher_info_db)),
      publisher_list_path_(profile->GetPath().Append(kPublishers_list)),
      rewards_base_path_(profile_->GetPath().Append(kRewardsStatePath)),
      publisher_info_backend_(
          new PublisherInfoDatabase(publisher_info_db_path_)),
      notification_service_(new RewardsNotificationServiceImpl(profile)),
#if BUILDFLAG(ENABLE_EXTENSIONS)
      private_observer_(
          std::make_unique<ExtensionRewardsServiceObserver>(profile_)),
#endif
      next_timer_id_(0) {
  file_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&EnsureRewardsBaseDirectoryExists,
                                rewards_base_path_));
  // Set up the rewards data source
  content::URLDataSource::Add(profile_,
                              std::make_unique<BraveRewardsSource>(profile_));
}

RewardsServiceImpl::~RewardsServiceImpl() {
  file_task_runner_->DeleteSoon(FROM_HERE, publisher_info_backend_.release());
  StopNotificationTimers();
}

void RewardsServiceImpl::ConnectionClosed() {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(FROM_HERE,
      base::BindOnce(&RewardsServiceImpl::StartLedger, AsWeakPtr()),
      base::TimeDelta::FromSeconds(1));
}

void RewardsServiceImpl::Init() {
  AddObserver(notification_service_.get());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  AddObserver(extension_rewards_service_observer_.get());
  private_observers_.AddObserver(private_observer_.get());
#endif

  StartLedger();
}

void RewardsServiceImpl::StartLedger() {
  bat_ledger::mojom::BatLedgerClientAssociatedPtrInfo client_ptr_info;
  bat_ledger_client_binding_.Bind(mojo::MakeRequest(&client_ptr_info));

  content::ServiceManagerConnection* connection =
          content::ServiceManagerConnection::GetForProcess();
  if (!connection) {
    return;
  }

  connection->GetConnector()->BindInterface(
      bat_ledger::mojom::kServiceName, &bat_ledger_service_);
  bat_ledger_service_.set_connection_error_handler(
      base::Bind(&RewardsServiceImpl::ConnectionClosed, AsWeakPtr()));

  bool isProduction = true;
  // Environment
  #if defined(OFFICIAL_BUILD)
    isProduction = true;
  #else
    isProduction = false;
  #endif
  SetProduction(isProduction);

  SetDebug(false);

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kRewards)) {
    std::string options = command_line.GetSwitchValueASCII(switches::kRewards);

    if (!options.empty()) {
      HandleFlags(options);
    }
  }

  bat_ledger_service_->Create(std::move(client_ptr_info),
      MakeRequest(&bat_ledger_));

  bat_ledger_->Initialize();
}

void RewardsServiceImpl::MaybeShowBackupNotification(uint64_t boot_stamp) {
  PrefService* pref_service = profile_->GetPrefs();
  bool user_has_funded = pref_service->GetBoolean(prefs::kRewardsUserHasFunded);
  bool backup_succeeded = pref_service->GetBoolean(
      prefs::kRewardsBackupSucceeded);
  if (user_has_funded && !backup_succeeded) {
    base::Time now = base::Time::Now();
    base::Time boot_timestamp = base::Time::FromDoubleT(boot_stamp);
    base::TimeDelta backup_notification_frequency =
        pref_service->GetTimeDelta(prefs::kRewardsBackupNotificationFrequency);
    base::TimeDelta backup_notification_interval =
        pref_service->GetTimeDelta(prefs::kRewardsBackupNotificationInterval);
    base::TimeDelta elapsed = now - boot_timestamp;
    if (elapsed > backup_notification_interval) {
      base::TimeDelta next_backup_notification_interval =
          backup_notification_interval + backup_notification_frequency;
      pref_service->SetTimeDelta(prefs::kRewardsBackupNotificationInterval,
                                 next_backup_notification_interval);
      RewardsNotificationService::RewardsNotificationArgs args;
      notification_service_->AddNotification(
          RewardsNotificationService::REWARDS_NOTIFICATION_BACKUP_WALLET, args,
          "rewards_notification_backup_wallet");
    }
  }
}

void RewardsServiceImpl::MaybeShowAddFundsNotification(
    uint64_t reconcile_stamp) {
  // Show add funds notification if reconciliation will occur in the
  // next 3 days and balance is too low.
  base::Time now = base::Time::Now();
  if (reconcile_stamp - now.ToDoubleT() <
      3 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour) {
    if (ShouldShowNotificationAddFunds()) {
      MaybeShowNotificationAddFunds();
    }
  }
}

void RewardsServiceImpl::CreateWallet() {
  if (ready().is_signaled()) {
    if (Connected())
      bat_ledger_->CreateWallet();
  } else {
    ready().Post(FROM_HERE,
        base::Bind(&brave_rewards::RewardsService::CreateWallet,
            base::Unretained(this)));
  }
}

void RewardsServiceImpl::GetContentSiteList(
    uint32_t start,
    uint32_t limit,
    uint64_t min_visit_time,
    uint64_t reconcile_stamp,
    bool allow_non_verified,
    uint32_t min_visits,
    const GetContentSiteListCallback& callback) {
  ledger::ActivityInfoFilter filter;
  filter.min_duration = min_visit_time;
  filter.order_by.push_back(std::pair<std::string, bool>("ai.percent", false));
  filter.reconcile_stamp = reconcile_stamp;
  filter.excluded =
    ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED;
  filter.percent = 1;
  filter.non_verified = allow_non_verified;
  filter.min_visits = min_visits;

  bat_ledger_->GetActivityInfoList(
      start,
      limit,
      filter.ToJson(),
      base::BindOnce(&RewardsServiceImpl::OnGetContentSiteList,
                     AsWeakPtr(),
                     callback));
}

void RewardsServiceImpl::OnGetContentSiteList(
    const GetContentSiteListCallback& callback,
    const std::vector<std::string>& json_list,
    uint32_t next_record) {
  std::unique_ptr<ContentSiteList> site_list(new ContentSiteList);

  for (auto &json_publisher : json_list) {
    ledger::PublisherInfo publisher;
    publisher.loadFromJson(json_publisher);
    site_list->push_back(PublisherInfoToContentSite(publisher));
  }

  callback.Run(std::move(site_list), next_record);
}

void RewardsServiceImpl::OnLoad(SessionID tab_id, const GURL& url) {
  if (!Connected())
    return;

  auto origin = url.GetOrigin();
  const std::string baseDomain =
      GetDomainAndRegistry(origin.host(), INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "")
    return;

  const std::string publisher_url = origin.scheme() + "://" + baseDomain + "/";

  ledger::VisitData data(baseDomain,
                         origin.host(),
                         url.path(),
                         tab_id.id(),
                         baseDomain,
                         publisher_url,
                         "",
                         "");
  bat_ledger_->OnLoad(data.ToJson(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnUnload(SessionID tab_id) {
  if (!Connected())
    return;

  bat_ledger_->OnUnload(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnShow(SessionID tab_id) {
  if (!Connected())
    return;

  bat_ledger_->OnShow(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnHide(SessionID tab_id) {
  if (!Connected())
    return;

  bat_ledger_->OnHide(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnForeground(SessionID tab_id) {
  if (!Connected())
    return;

  bat_ledger_->OnForeground(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnBackground(SessionID tab_id) {
  if (!Connected())
    return;

  bat_ledger_->OnBackground(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnMediaStart(SessionID tab_id) {
  if (!Connected())
    return;

  bat_ledger_->OnMediaStart(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnMediaStop(SessionID tab_id) {
  if (!Connected())
    return;

  bat_ledger_->OnMediaStop(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnPostData(SessionID tab_id,
                                    const GURL& url,
                                    const GURL& first_party_url,
                                    const GURL& referrer,
                                    const std::string& post_data) {
  if (!Connected())
    return;

  std::string output;
  url::RawCanonOutputW<1024> canonOutput;
  url::DecodeURLEscapeSequences(post_data.c_str(),
                                post_data.length(),
                                url::DecodeURLMode::kUTF8OrIsomorphic,
                                &canonOutput);
  output = base::UTF16ToUTF8(base::StringPiece16(canonOutput.data(),
                                                 canonOutput.length()));

  if (output.empty())
    return;

  ledger::VisitData visit_data(std::string(),
                               std::string(),
                               url.spec(),
                               tab_id.id(),
                               std::string(),
                               std::string(),
                               std::string(),
                               std::string());

  bat_ledger_->OnPostData(url.spec(),
                          first_party_url.spec(),
                          referrer.spec(),
                          output,
                          visit_data.ToJson());
}

void RewardsServiceImpl::OnXHRLoad(SessionID tab_id,
                                   const GURL& url,
                                   const GURL& first_party_url,
                                   const GURL& referrer) {
  if (!Connected())
    return;

  std::map<std::string, std::string> parts;

  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    parts[it.GetKey()] = it.GetUnescapedValue();
  }

  ledger::VisitData data(std::string(),
                         std::string(),
                         url.spec(),
                         tab_id.id(),
                         std::string(),
                         std::string(),
                         std::string(),
                         std::string());

  bat_ledger_->OnXHRLoad(tab_id.id(),
                         url.spec(),
                         mojo::MapToFlatMap(parts),
                         first_party_url.spec(),
                         referrer.spec(),
                         data.ToJson());
}

void RewardsServiceImpl::LoadPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadPublisherInfoOnFileTaskRunner,
          publisher_key, publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnPublisherInfoLoaded,
                     AsWeakPtr(),
                     callback));
}

void RewardsServiceImpl::OnPublisherInfoLoaded(
    ledger::PublisherInfoCallback callback,
    std::unique_ptr<ledger::PublisherInfo> info) {
  if (!info) {
    callback(ledger::Result::NOT_FOUND, nullptr);
    return;
  }

  callback(ledger::Result::LEDGER_OK, std::move(info));
}

void RewardsServiceImpl::LoadMediaPublisherInfo(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadMediaPublisherInfoOnFileTaskRunner,
          media_key, publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnMediaPublisherInfoLoaded,
                     AsWeakPtr(),
                     callback));
}

void RewardsServiceImpl::OnMediaPublisherInfoLoaded(
    ledger::PublisherInfoCallback callback,
    std::unique_ptr<ledger::PublisherInfo> info) {
  if (!Connected())
    return;

  if (!info) {
    callback(ledger::Result::NOT_FOUND, nullptr);
    return;
  }

  callback(ledger::Result::LEDGER_OK, std::move(info));
}

void RewardsServiceImpl::SaveMediaPublisherInfo(
    const std::string& media_key,
    const std::string& publisher_id) {
base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&SaveMediaPublisherInfoOnFileTaskRunner,
                    media_key,
                    publisher_id,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnMediaPublisherInfoSaved,
                     AsWeakPtr()));
}

void RewardsServiceImpl::ExcludePublisher(
    const std::string publisherKey) const {
  if (!Connected())
    return;

  bat_ledger_->SetPublisherExclude(publisherKey,
                                   ledger::PUBLISHER_EXCLUDE::EXCLUDED);
}

void RewardsServiceImpl::RestorePublishers() {
  if (!Connected())
    return;

  bat_ledger_->RestorePublishers();
}

void RewardsServiceImpl::OnMediaPublisherInfoSaved(bool success) {
  if (!success) {
    LOG(ERROR) << "Error in OnMediaPublisherInfoSaved";
  }
}

std::string RewardsServiceImpl::URIEncode(const std::string& value) {
  return net::EscapeQueryParamValue(value, false);
}

std::string RewardsServiceImpl::GenerateGUID() const {
  return base::GenerateGUID();
}

void RewardsServiceImpl::Shutdown() {
  RemoveObserver(notification_service_.get());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  RemoveObserver(extension_rewards_service_observer_.get());
  private_observers_.RemoveObserver(private_observer_.get());
#endif
  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    for (auto request_id : request_ids_) {
      image_service->CancelRequest(request_id);
    }
  }

  for (const auto fetcher : fetchers_) {
    delete fetcher.first;
  }
  fetchers_.clear();

  bat_ledger_.reset();
  RewardsService::Shutdown();
}

void RewardsServiceImpl::OnWalletInitialized(ledger::Result result) {
  if (!ready_.is_signaled())
    ready_.Signal();

  if (result == ledger::Result::WALLET_CREATED) {
    SetRewardsMainEnabled(true);
    SetAutoContribute(true);
    StartNotificationTimers(true);
  }

  TriggerOnWalletInitialized(result);
}

void RewardsServiceImpl::OnWalletProperties(
    ledger::Result result,
    std::unique_ptr<ledger::WalletInfo> wallet_info) {
  if (wallet_info && wallet_info->balance_ > 0) {
    profile_->GetPrefs()->SetBoolean(prefs::kRewardsUserHasFunded, true);
  }

  std::unique_ptr<brave_rewards::WalletProperties> wallet_properties;
  for (auto& observer : observers_) {
    if (wallet_info) {
      wallet_properties.reset(new brave_rewards::WalletProperties);
      wallet_properties->probi = wallet_info->probi_;
      wallet_properties->balance = wallet_info->balance_;
      wallet_properties->rates = wallet_info->rates_;
      wallet_properties->parameters_choices = wallet_info->parameters_choices_;
      wallet_properties->parameters_range = wallet_info->parameters_range_;
      wallet_properties->parameters_days = wallet_info->parameters_days_;
      wallet_properties->monthly_amount = wallet_info->fee_amount_;

      for (size_t i = 0; i < wallet_info->grants_.size(); i ++) {
        brave_rewards::Grant grant;

        grant.altcurrency = wallet_info->grants_[i].altcurrency;
        grant.probi = wallet_info->grants_[i].probi;
        grant.expiryTime = wallet_info->grants_[i].expiryTime;

        wallet_properties->grants.push_back(grant);
      }
    }

    // webui
    observer.OnWalletProperties(this,
                                static_cast<int>(result),
                                std::move(wallet_properties));
  }
}

void RewardsServiceImpl::OnGetAutoContributeProps(
    const GetAutoContributePropsCallback& callback,
    const std::string& json_props) {
  ledger::AutoContributeProps props;
  props.loadFromJson(json_props);

  auto auto_contri_props =
    std::make_unique<brave_rewards::AutoContributeProps>();
  auto_contri_props->enabled_contribute = props.enabled_contribute;
  auto_contri_props->contribution_min_time = props.contribution_min_time;
  auto_contri_props->contribution_min_visits = props.contribution_min_visits;
  auto_contri_props->contribution_non_verified =
    props.contribution_non_verified;
  auto_contri_props->contribution_videos = props.contribution_videos;
  auto_contri_props->reconcile_stamp = props.reconcile_stamp;

  callback.Run(std::move(auto_contri_props));
}

void RewardsServiceImpl::OnGetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback,
    const std::string& json_info) {
  ledger::RewardsInternalsInfo info;
  info.loadFromJson(json_info);

  auto rewards_internals_info =
      std::make_unique<brave_rewards::RewardsInternalsInfo>();
  rewards_internals_info->payment_id = info.payment_id;
  rewards_internals_info->is_key_info_seed_valid = info.is_key_info_seed_valid;
  rewards_internals_info->persona_id = info.persona_id;
  rewards_internals_info->user_id = info.user_id;
  rewards_internals_info->boot_stamp = info.boot_stamp;

  for (const auto& item : info.current_reconciles) {
    ReconcileInfo reconcile_info;
    reconcile_info.viewing_id_ = item.second.viewingId_;
    reconcile_info.amount_ = item.second.amount_;
    reconcile_info.retry_step_ =
        static_cast<ContributionRetry>(item.second.retry_step_);
    reconcile_info.retry_level_ = item.second.retry_level_;
    rewards_internals_info->current_reconciles[item.first] = reconcile_info;
  }

  std::move(callback).Run(std::move(rewards_internals_info));
}

void RewardsServiceImpl::GetAutoContributeProps(
    const GetAutoContributePropsCallback& callback) {
  if (!Connected())
    return;

  bat_ledger_->GetAutoContributeProps(base::BindOnce(
        &RewardsServiceImpl::OnGetAutoContributeProps, AsWeakPtr(), callback));
}

void RewardsServiceImpl::OnGrant(ledger::Result result,
                                 const ledger::Grant& grant) {
  TriggerOnGrant(result, grant);
}

void RewardsServiceImpl::OnGrantCaptcha(const std::string& image,
    const std::string& hint) {
  TriggerOnGrantCaptcha(image, hint);
}

void RewardsServiceImpl::OnRecoverWallet(ledger::Result result,
                                    double balance,
                                    const std::vector<ledger::Grant>& grants) {
  TriggerOnRecoverWallet(result, balance, grants);
}

void RewardsServiceImpl::OnGrantFinish(ledger::Result result,
                                       const ledger::Grant& grant) {
  ledger::BalanceReportInfo report_info;
  auto now = base::Time::Now();
  if (result == ledger::Result::LEDGER_OK) {
    if (!Connected())
      return;

    int report_type = grant.type == "ads"
      ? ledger::ReportType::ADS
      : ledger::ReportType::GRANT;
    bat_ledger_->SetBalanceReportItem(GetPublisherMonth(now),
                                      GetPublisherYear(now),
                                      report_type,
                                      grant.probi);
  }

  GetCurrentBalanceReport();
  TriggerOnGrantFinish(result, grant);
}

void RewardsServiceImpl::OnReconcileComplete(ledger::Result result,
  const std::string& viewing_id,
  ledger::REWARDS_CATEGORY category,
  const std::string& probi) {
  if (result == ledger::Result::LEDGER_OK) {
    auto now = base::Time::Now();
    if (!Connected())
      return;

    FetchWalletProperties();

    if (category == ledger::REWARDS_CATEGORY::RECURRING_TIP) {
      MaybeShowNotificationTipsPaid();
    }

    bat_ledger_->OnReconcileCompleteSuccess(viewing_id,
        category,
        probi,
        GetPublisherMonth(now),
        GetPublisherYear(now),
        GetCurrentTimestamp());
  }

  GetCurrentBalanceReport();
  for (auto& observer : observers_)
    observer.OnReconcileComplete(this,
                                 result,
                                 viewing_id,
                                 std::to_string(category),
                                 probi);
}

void RewardsServiceImpl::LoadLedgerState(
    ledger::LedgerCallbackHandler* handler) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadStateOnFileTaskRunner, ledger_state_path_),
      base::Bind(&RewardsServiceImpl::OnLedgerStateLoaded,
                     AsWeakPtr(),
                     base::Unretained(handler)));
}

void RewardsServiceImpl::OnLedgerStateLoaded(
    ledger::LedgerCallbackHandler* handler,
    const std::string& data) {
  if (!Connected())
    return;

  handler->OnLedgerStateLoaded(data.empty() ? ledger::Result::NO_LEDGER_STATE
                                            : ledger::Result::LEDGER_OK,
                               data);

  bat_ledger_->GetRewardsMainEnabled(
      base::BindOnce(&RewardsServiceImpl::StartNotificationTimers,
        AsWeakPtr()));
}

void RewardsServiceImpl::LoadPublisherState(
    ledger::LedgerCallbackHandler* handler) {
  if (!profile_->GetPrefs()->GetBoolean(prefs::kBraveRewardsEnabledMigrated)) {
    bat_ledger_->GetRewardsMainEnabled(
        base::BindOnce(&RewardsServiceImpl::SetRewardsMainEnabledPref,
          AsWeakPtr()));
  }
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadStateOnFileTaskRunner, publisher_state_path_),
      base::Bind(&RewardsServiceImpl::OnPublisherStateLoaded,
                     AsWeakPtr(),
                     base::Unretained(handler)));
}

void RewardsServiceImpl::OnPublisherStateLoaded(
    ledger::LedgerCallbackHandler* handler,
    const std::string& data) {
  if (!Connected())
    return;

  handler->OnPublisherStateLoaded(
      data.empty() ? ledger::Result::NO_PUBLISHER_STATE
                   : ledger::Result::LEDGER_OK,
      data);
}

void RewardsServiceImpl::SaveLedgerState(const std::string& ledger_state,
                                      ledger::LedgerCallbackHandler* handler) {
  base::ImportantFileWriter writer(
      ledger_state_path_, file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
      base::Closure(),
      base::Bind(
        &PostWriteCallback,
        base::Bind(&RewardsServiceImpl::OnLedgerStateSaved, AsWeakPtr(),
            base::Unretained(handler)),
        base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(ledger_state));
}

void RewardsServiceImpl::OnLedgerStateSaved(
    ledger::LedgerCallbackHandler* handler,
    bool success) {
  if (!Connected())
    return;

  handler->OnLedgerStateSaved(success ? ledger::Result::LEDGER_OK
                                      : ledger::Result::NO_LEDGER_STATE);
}

void RewardsServiceImpl::SavePublisherState(const std::string& publisher_state,
                                      ledger::LedgerCallbackHandler* handler) {
  base::ImportantFileWriter writer(publisher_state_path_, file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
      base::Closure(),
      base::Bind(
        &PostWriteCallback,
        base::Bind(&RewardsServiceImpl::OnPublisherStateSaved, AsWeakPtr(),
            base::Unretained(handler)),
        base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(publisher_state));
}

void RewardsServiceImpl::OnPublisherStateSaved(
    ledger::LedgerCallbackHandler* handler,
    bool success) {
  if (!Connected())
    return;

  handler->OnPublisherStateSaved(success ? ledger::Result::LEDGER_OK
                                         : ledger::Result::LEDGER_ERROR);
}

void RewardsServiceImpl::LoadNicewareList(
  ledger::GetNicewareListCallback callback) {
  if (!Connected())
    return;

  std::string data = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
      IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();

  if (data.empty()) {
    LOG(ERROR) << "Failed to read in niceware list";
  }
  callback(data.empty() ? ledger::Result::LEDGER_ERROR
                        : ledger::Result::LEDGER_OK, data);
}

void RewardsServiceImpl::SavePublisherInfo(
    std::unique_ptr<ledger::PublisherInfo> publisher_info,
    ledger::PublisherInfoCallback callback) {
  ledger::PublisherInfo info_copy = *publisher_info;
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&SavePublisherInfoOnFileTaskRunner,
                    info_copy,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnPublisherInfoSaved,
                     AsWeakPtr(),
                     callback,
                     base::Passed(std::move(publisher_info))));
}

void RewardsServiceImpl::OnPublisherInfoSaved(
    ledger::PublisherInfoCallback callback,
    std::unique_ptr<ledger::PublisherInfo> info,
    bool success) {
  if (Connected()) {
    callback(success ? ledger::Result::LEDGER_OK
                     : ledger::Result::LEDGER_ERROR, std::move(info));
  }
}

void RewardsServiceImpl::SaveActivityInfo(
    std::unique_ptr<ledger::PublisherInfo> publisher_info,
    ledger::PublisherInfoCallback callback) {
  ledger::PublisherInfo info_copy = *publisher_info;
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&SaveActivityInfoOnFileTaskRunner,
                    info_copy,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnActivityInfoSaved,
                     AsWeakPtr(),
                     callback,
                     base::Passed(std::move(publisher_info))));
}

void RewardsServiceImpl::OnActivityInfoSaved(
    ledger::PublisherInfoCallback callback,
    std::unique_ptr<ledger::PublisherInfo> info,
    bool success) {
  if (Connected()) {
    callback(success ? ledger::Result::LEDGER_OK
                     : ledger::Result::LEDGER_ERROR, std::move(info));
  }
}

void RewardsServiceImpl::LoadActivityInfo(
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&GetActivityListOnFileTaskRunner,
          // set limit to 2 to make sure there is
          // only 1 valid result for the filter
          0, 2, filter, publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnActivityInfoLoaded,
                     AsWeakPtr(),
                     callback,
                     filter.id));
}

void RewardsServiceImpl::OnPublisherActivityInfoLoaded(
    ledger::PublisherInfoCallback callback,
    uint32_t result,
    const std::string& info_json) {
  std::unique_ptr<ledger::PublisherInfo> publisher;

  if (!info_json.empty()) {
    publisher = std::make_unique<ledger::PublisherInfo>();
    publisher->loadFromJson(info_json);
  }

  callback(static_cast<ledger::Result>(result), std::move(publisher));
}

void RewardsServiceImpl::OnActivityInfoLoaded(
    ledger::PublisherInfoCallback callback,
    const std::string& publisher_key,
    const ledger::PublisherInfoList list) {
  if (!Connected()) {
    return;
  }

  // activity info not found
  if (list.size() == 0) {
    // we need to try to get at least publisher info in this case
    // this way we preserve publisher info
    bat_ledger_->LoadPublisherInfo(
        publisher_key,
        base::BindOnce(&RewardsServiceImpl::OnPublisherActivityInfoLoaded,
                       AsWeakPtr(),
                       callback));
    return;
  } else if (list.size() > 1) {
    callback(ledger::Result::TOO_MANY_RESULTS,
        std::unique_ptr<ledger::PublisherInfo>());
    return;
  }

  callback(ledger::Result::LEDGER_OK,
      std::make_unique<ledger::PublisherInfo>(list[0]));
}

void RewardsServiceImpl::LoadPanelPublisherInfo(
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&GetPanelPublisherInfoOnFileTaskRunner,
                 filter,
                 publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnPanelPublisherInfoLoaded,
                     AsWeakPtr(),
                     callback));
}

void RewardsServiceImpl::OnPanelPublisherInfoLoaded(
    ledger::PublisherInfoCallback callback,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (!publisher_info) {
    callback(ledger::Result::NOT_FOUND,
             std::unique_ptr<ledger::PublisherInfo>());
    return;
  }

  callback(ledger::Result::LEDGER_OK, std::move(publisher_info));
}

void RewardsServiceImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoListCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&GetActivityListOnFileTaskRunner,
                    start, limit, filter,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnPublisherInfoListLoaded,
                    AsWeakPtr(),
                    start,
                    limit,
                    callback));
}

void RewardsServiceImpl::OnPublisherInfoListLoaded(
    uint32_t start,
    uint32_t limit,
    ledger::PublisherInfoListCallback callback,
    const ledger::PublisherInfoList& list) {
  if (!Connected()) {
    return;
  }

  uint32_t next_record = 0;
  if (list.size() == limit)
    next_record = start + limit + 1;

  callback(std::cref(list), next_record);
}

void RewardsServiceImpl::LoadURL(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    const ledger::URL_METHOD method,
    ledger::LoadURLCallback callback) {

  if (url.empty()) {
    callback(400, "", {});
    return;
  }

  GURL parsed_url(url);
  if (!parsed_url.is_valid()) {
    callback(400, "", {});
    return;
  }

  net::URLFetcher::RequestType request_type = URLMethodToRequestType(method);

  net::URLFetcher* fetcher = net::URLFetcher::Create(
      parsed_url, request_type, this).release();
  fetcher->SetRequestContext(g_browser_process->system_request_context());

  for (size_t i = 0; i < headers.size(); i++)
    fetcher->AddExtraRequestHeader(headers[i]);

  if (!content.empty())
    fetcher->SetUploadData(contentType, content);

  if (VLOG_IS_ON(ledger::LogLevel::LOG_REQUEST)) {
    std::string printMethod;
    switch (method) {
      case ledger::URL_METHOD::POST:
        printMethod = "POST";
        break;
      case ledger::URL_METHOD::PUT:
        printMethod = "PUT";
        break;
      default:
        printMethod = "GET";
        break;
    }

    std::string headers_log = "";
    for (auto const& header : headers) {
      headers_log += "> headers: " + header + "\n";
    }

    VLOG(ledger::LogLevel::LOG_REQUEST) << std::endl
      << "[ REQUEST ]" << std::endl
      << "> url: " << url << std::endl
      << "> method: " << printMethod << std::endl
      << "> content: " << content << std::endl
      << "> contentType: " << contentType << std::endl
      << headers_log
      << "[ END REQUEST ]";
  }

  fetchers_[fetcher] = callback;
  fetcher->Start();
}

void RewardsServiceImpl::OnURLFetchComplete(
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

  if (!Connected()) {
    return;
  }

  callback(response_code, body, headers);
}

void RewardsServiceImpl::TriggerOnWalletInitialized(ledger::Result result) {
  for (auto& observer : observers_)
    observer.OnWalletInitialized(this, result);
}

void RewardsServiceImpl::OnFetchWalletProperties(
    int result,
    const std::string& json_wallet) {
  std::unique_ptr<ledger::WalletInfo> wallet_info;

  if (!json_wallet.empty()) {
    wallet_info.reset(new ledger::WalletInfo());
    wallet_info->loadFromJson(json_wallet);
  }

  OnWalletProperties(static_cast<ledger::Result>(result),
                     std::move(wallet_info));
}

void RewardsServiceImpl::FetchWalletProperties() {
  if (ready().is_signaled()) {
    if (!Connected()) {
      return;
    }

    bat_ledger_->FetchWalletProperties(
        base::BindOnce(&RewardsServiceImpl::OnFetchWalletProperties,
                       AsWeakPtr()));
  } else {
    ready().Post(FROM_HERE,
        base::Bind(&brave_rewards::RewardsService::FetchWalletProperties,
            base::Unretained(this)));
  }
}

void RewardsServiceImpl::FetchGrants(const std::string& lang,
    const std::string& payment_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->FetchGrants(lang, payment_id);
}

void RewardsServiceImpl::TriggerOnGrant(ledger::Result result,
                                        const ledger::Grant& grant) {
  brave_rewards::Grant properties;

  properties.promotionId = grant.promotionId;
  properties.altcurrency = grant.altcurrency;
  properties.probi = grant.probi;
  properties.expiryTime = grant.expiryTime;
  properties.type = grant.type;

  for (auto& observer : observers_)
    observer.OnGrant(this, result, properties);
}

void RewardsServiceImpl::GetGrantCaptcha(
    const std::string& promotion_id,
    const std::string& promotion_type) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetGrantCaptcha(promotion_id, promotion_type);
}

void RewardsServiceImpl::TriggerOnGrantCaptcha(const std::string& image,
    const std::string& hint) {
  for (auto& observer : observers_)
    observer.OnGrantCaptcha(this, image, hint);
}

void RewardsServiceImpl::GetWalletPassphrase(
    const GetWalletPassphraseCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetWalletPassphrase(callback);
}

void RewardsServiceImpl::GetExcludedPublishersNumber(
    const GetExcludedPublishersNumberCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetExcludedPublishersNumber(callback);
}

void RewardsServiceImpl::RecoverWallet(const std::string passPhrase) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->RecoverWallet(passPhrase);
}

void RewardsServiceImpl::TriggerOnRecoverWallet(ledger::Result result,
                                                double balance,
                                    const std::vector<ledger::Grant>& grants) {
  std::vector<brave_rewards::Grant> newGrants;
  for (size_t i = 0; i < grants.size(); i ++) {
    brave_rewards::Grant grant;

    grant.altcurrency = grants[i].altcurrency;
    grant.probi = grants[i].probi;
    grant.expiryTime = grants[i].expiryTime;

    newGrants.push_back(grant);
  }
  for (auto& observer : observers_)
    observer.OnRecoverWallet(this, result, balance, newGrants);
}

void RewardsServiceImpl::SolveGrantCaptcha(const std::string& solution,
                                         const std::string& promotionId) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SolveGrantCaptcha(solution, promotionId);
}

void RewardsServiceImpl::TriggerOnGrantFinish(ledger::Result result,
                                              const ledger::Grant& grant) {
  brave_rewards::Grant properties;

  properties.promotionId = grant.promotionId;
  properties.altcurrency = grant.altcurrency;
  properties.probi = grant.probi;
  properties.expiryTime = grant.expiryTime;
  properties.type = grant.type;

  for (auto& observer : observers_)
    observer.OnGrantFinish(this, result, properties);
}

void RewardsServiceImpl::GetReconcileStamp(
    const GetReconcileStampCallback& callback)  {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetReconcileStamp(callback);
}

void RewardsServiceImpl::OnGetAddresses(const GetAddressesCallback& callback,
    const base::flat_map<std::string, std::string>& addresses) {
  callback.Run(mojo::FlatMapToMap(addresses));
}

void RewardsServiceImpl::GetAddresses(const GetAddressesCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAddresses(base::BindOnce(&RewardsServiceImpl::OnGetAddresses,
        AsWeakPtr(), callback));
}

void RewardsServiceImpl::SetRewardsMainEnabled(bool enabled) {
  if (!Connected()) {
    return;
  }

  SetRewardsMainEnabledPref(enabled);
  bat_ledger_->SetRewardsMainEnabled(enabled);
  TriggerOnRewardsMainEnabled(enabled);
}

void RewardsServiceImpl::GetRewardsMainEnabled(
    const GetRewardsMainEnabledCallback& callback) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetRewardsMainEnabled(callback);
}

void RewardsServiceImpl::SetRewardsMainEnabledPref(bool enabled) {
  profile_->GetPrefs()->SetBoolean(prefs::kBraveRewardsEnabled, enabled);
  SetRewardsMainEnabledMigratedPref(true);
}

void RewardsServiceImpl::SetRewardsMainEnabledMigratedPref(bool enabled) {
  profile_->GetPrefs()->SetBoolean(
      prefs::kBraveRewardsEnabledMigrated, enabled);
}

void RewardsServiceImpl::SetCatalogIssuers(const std::string& json) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetCatalogIssuers(json);
}

std::pair<uint64_t, uint64_t> RewardsServiceImpl::GetEarningsRange() {
  auto now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  if (exploded.day_of_month < 5) {
    exploded.month--;
    if (exploded.month < 1) {
      exploded.month = 12;

      exploded.year--;
    }
  }

  exploded.day_of_month = 1;

  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;

  base::Time from_timestamp;
  auto success = base::Time::FromLocalExploded(exploded, &from_timestamp);
  DCHECK(success);

  uint64_t from_timestamp_in_seconds =
      (from_timestamp - base::Time()).InSeconds();

  uint64_t to_timestamp_in_seconds =
      (now - base::Time()).InSeconds();

  return std::make_pair(from_timestamp_in_seconds, to_timestamp_in_seconds);
}

void RewardsServiceImpl::ConfirmAd(const std::string& json) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->ConfirmAd(json);
}

void RewardsServiceImpl::SetConfirmationsIsReady(const bool is_ready) {
  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile_);
  if (ads_service)
    ads_service->SetConfirmationsIsReady(is_ready);
}

void RewardsServiceImpl::ConfirmationsTransactionHistoryDidChange() {
    for (auto& observer : observers_)
    observer.OnConfirmationsHistoryChanged(this);
}

void RewardsServiceImpl::GetConfirmationsHistory(
    brave_rewards::ConfirmationsHistoryCallback callback) {
  if (!Connected()) {
    return;
  }

  auto earnings_range = GetEarningsRange();

  bat_ledger_->GetConfirmationsHistory(
      earnings_range.first,
      earnings_range.second,
      base::BindOnce(&RewardsServiceImpl::OnGetConfirmationsHistory,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetConfirmationsHistory(
    brave_rewards::ConfirmationsHistoryCallback callback,
    const std::string& transactions) {
  std::unique_ptr<ledger::TransactionsInfo> info;
  if (!transactions.empty()) {
    info.reset(new ledger::TransactionsInfo());
    info->FromJson(transactions);
  }

  if (!info) {
    callback.Run(0, 0.0);
  }

  double estimated_earnings = 0.0;
  int total_viewed = 0;

  for (const auto& transaction : info->transactions) {
    if (transaction.estimated_redemption_value == 0.0) {
      continue;
    }

    estimated_earnings += transaction.estimated_redemption_value;
    total_viewed++;
  }

  callback.Run(total_viewed, estimated_earnings);
}

void RewardsServiceImpl::SaveState(const std::string& name,
                                   const std::string& value,
                                   ledger::OnSaveCallback callback) {
  base::ImportantFileWriter writer(
      rewards_base_path_.AppendASCII(name), file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
      base::Closure(),
      base::Bind(&PostWriteCallback,
                 base::Bind(&RewardsServiceImpl::OnSavedState,
                            AsWeakPtr(),
                            std::move(callback)),
                 base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(value));
}

void RewardsServiceImpl::LoadState(
    const std::string& name,
    ledger::OnLoadCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner,
                     rewards_base_path_.AppendASCII(name)),
      base::BindOnce(&RewardsServiceImpl::OnLoadedState,
                     AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::ResetState(
    const std::string& name,
    ledger::OnResetCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ResetOnFileTaskRunner,
                     rewards_base_path_.AppendASCII(name)),
      base::BindOnce(&RewardsServiceImpl::OnResetState,
                     AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::OnSavedState(
  ledger::OnSaveCallback callback, bool success) {
  if (!Connected())
    return;
  callback(success ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
}

void RewardsServiceImpl::OnLoadedState(
    ledger::OnLoadCallback callback,
    const std::string& value) {
  if (!Connected())
    return;
  if (value.empty())
    callback(ledger::Result::LEDGER_ERROR, value);
  else
    callback(ledger::Result::LEDGER_OK, value);
}

void RewardsServiceImpl::KillTimer(uint32_t timer_id) {
  if (timers_.find(timer_id) == timers_.end())
    return;

  timers_[timer_id]->Stop();
  timers_.erase(timer_id);
}

void RewardsServiceImpl::OnResetState(
  ledger::OnResetCallback callback, bool success) {
  if (!Connected())
    return;
  callback(success ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
}

void RewardsServiceImpl::GetPublisherMinVisitTime(
    const GetPublisherMinVisitTimeCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherMinVisitTime(callback);
}

void RewardsServiceImpl::SetPublisherMinVisitTime(
    uint64_t duration_in_seconds) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherMinVisitTime(duration_in_seconds);
}

void RewardsServiceImpl::GetPublisherMinVisits(
    const GetPublisherMinVisitsCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherMinVisits(callback);
}

void RewardsServiceImpl::SetPublisherMinVisits(unsigned int visits) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherMinVisits(visits);
}

void RewardsServiceImpl::GetPublisherAllowNonVerified(
    const GetPublisherAllowNonVerifiedCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherAllowNonVerified(callback);
}

void RewardsServiceImpl::SetPublisherAllowNonVerified(bool allow) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherAllowNonVerified(allow);
}

void RewardsServiceImpl::GetPublisherAllowVideos(
    const GetPublisherAllowVideosCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherAllowVideos(callback);
}

void RewardsServiceImpl::SetPublisherAllowVideos(bool allow) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherAllowVideos(allow);
}

void RewardsServiceImpl::SetContributionAmount(double amount) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetUserChangedContribution();
  bat_ledger_->SetContributionAmount(amount);
}

// TODO(brave): Remove me (and pure virtual definition)
// see https://github.com/brave/brave-core/commit/c4ef62c954a64fca18ae83ff8ffd611137323420#diff-aa3505dbf36b5d03d8ba0751e0c99904R385
// and https://github.com/brave-intl/bat-native-ledger/commit/27f3ceb471d61c84052737ff201fe18cb9a6af32#diff-e303122e010480b2226895b9470891a3R135
void RewardsServiceImpl::SetUserChangedContribution() const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetUserChangedContribution();
}

void RewardsServiceImpl::GetAutoContribute(
    GetAutoContributeCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAutoContribute(std::move(callback));
}

void RewardsServiceImpl::SetAutoContribute(bool enabled) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetAutoContribute(enabled);
}

void RewardsServiceImpl::TriggerOnRewardsMainEnabled(
    bool rewards_main_enabled) {
  for (auto& observer : observers_)
    observer.OnRewardsMainEnabled(this, rewards_main_enabled);
}

void RewardsServiceImpl::SavePublishersList(const std::string& publishers_list,
                                      ledger::LedgerCallbackHandler* handler) {
  base::ImportantFileWriter writer(
      publisher_list_path_, file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
      base::Closure(),
      base::Bind(
        &PostWriteCallback,
        base::Bind(&RewardsServiceImpl::OnPublishersListSaved, AsWeakPtr(),
            base::Unretained(handler)),
        base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(publishers_list));
}

void RewardsServiceImpl::OnPublishersListSaved(
    ledger::LedgerCallbackHandler* handler,
    bool success) {
  if (!Connected()) {
    return;
  }

  handler->OnPublishersListSaved(success ? ledger::Result::LEDGER_OK
                                         : ledger::Result::LEDGER_ERROR);
}

void RewardsServiceImpl::SetTimer(uint64_t time_offset,
                                  uint32_t* timer_id) {
  if (next_timer_id_ == std::numeric_limits<uint32_t>::max())
    next_timer_id_ = 1;
  else
    ++next_timer_id_;

  *timer_id = next_timer_id_;

  timers_[next_timer_id_] = std::make_unique<base::OneShotTimer>();
  timers_[next_timer_id_]->Start(FROM_HERE,
      base::TimeDelta::FromSeconds(time_offset),
      base::BindOnce(
          &RewardsServiceImpl::OnTimer, AsWeakPtr(), next_timer_id_));
}

void RewardsServiceImpl::OnTimer(uint32_t timer_id) {
  if (!Connected()) {
    return;
  }

  timers_.erase(timer_id);
  bat_ledger_->OnTimer(timer_id);
}

void RewardsServiceImpl::LoadPublisherList(
    ledger::LedgerCallbackHandler* handler) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadStateOnFileTaskRunner, publisher_list_path_),
      base::Bind(&RewardsServiceImpl::OnPublisherListLoaded,
          AsWeakPtr(), base::Unretained(handler)));
}

void RewardsServiceImpl::OnPublisherListLoaded(
    ledger::LedgerCallbackHandler* handler,
    const std::string& data) {
  if (!Connected()) {
    return;
  }

  handler->OnPublisherListLoaded(
      data.empty() ? ledger::Result::NO_PUBLISHER_LIST
                   : ledger::Result::LEDGER_OK,
      data);
}

void RewardsServiceImpl::OnGetAllBalanceReports(
    const GetAllBalanceReportsCallback& callback,
    const base::flat_map<std::string, std::string>& json_reports) {
  std::map<std::string, ledger::BalanceReportInfo> reports;
  for (auto const& report : json_reports) {
    ledger::BalanceReportInfo info;
    info.loadFromJson(report.second);
    reports[report.first] = info;
  }

  std::map<std::string, brave_rewards::BalanceReport> newReports;
  for (auto const& report : reports) {
    brave_rewards::BalanceReport newReport;
    const ledger::BalanceReportInfo oldReport = report.second;
    newReport.opening_balance = oldReport.opening_balance_;
    newReport.closing_balance = oldReport.closing_balance_;
    newReport.grants = oldReport.grants_;
    newReport.earning_from_ads = oldReport.earning_from_ads_;
    newReport.auto_contribute = oldReport.auto_contribute_;
    newReport.recurring_donation = oldReport.recurring_donation_;
    newReport.one_time_donation = oldReport.one_time_donation_;

    newReports[report.first] = newReport;
  }

  callback.Run(newReports);
}

void RewardsServiceImpl::GetAllBalanceReports(
    const GetAllBalanceReportsCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAllBalanceReports(
      base::BindOnce(&RewardsServiceImpl::OnGetAllBalanceReports,
        AsWeakPtr(), callback));
}

void RewardsServiceImpl::OnGetCurrentBalanceReport(
    bool success, const std::string& json_report) {
  ledger::BalanceReportInfo report;
  report.loadFromJson(json_report);

  if (success) {
    TriggerOnGetCurrentBalanceReport(report);
  }
}

void RewardsServiceImpl::GetCurrentBalanceReport() {
  auto now = base::Time::Now();
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetBalanceReport(GetPublisherMonth(now), GetPublisherYear(now),
      base::BindOnce(&RewardsServiceImpl::OnGetCurrentBalanceReport,
        AsWeakPtr()));
}

void RewardsServiceImpl::IsWalletCreated(
    const IsWalletCreatedCallback& callback) {
  if (!Connected()) {
    callback.Run(false);
    return;
  }

  bat_ledger_->IsWalletCreated(callback);
}

void RewardsServiceImpl::GetPublisherActivityFromUrl(
    uint64_t windowId,
    const std::string& url,
    const std::string& favicon_url,
    const std::string& publisher_blob) {
  GURL parsedUrl(url);

  if (!parsedUrl.is_valid()) {
    return;
  }

  auto origin = parsedUrl.GetOrigin();
  std::string baseDomain =
      GetDomainAndRegistry(origin.host(), INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "") {
    std::unique_ptr<ledger::PublisherInfo> info;
    OnPanelPublisherInfo(ledger::Result::NOT_FOUND, std::move(info), windowId);
    return;
  }

  if (!Connected())
    return;

  ledger::VisitData visitData;
  visitData.domain = baseDomain;
  visitData.path = parsedUrl.PathForRequest();
  visitData.name = baseDomain;
  visitData.url = origin.spec();
  visitData.favicon_url = favicon_url;

  bat_ledger_->GetPublisherActivityFromUrl(
    windowId, visitData.ToJson(), publisher_blob);
}

void RewardsServiceImpl::OnExcludedSitesChanged(
    const std::string& publisher_id,
    ledger::PUBLISHER_EXCLUDE exclude) {

  bool excluded = exclude == ledger::PUBLISHER_EXCLUDE::EXCLUDED;

  if (excluded) {
    DeleteActivityInfo(publisher_id);
  }

  for (auto& observer : observers_) {
    observer.OnExcludedSitesChanged(this, publisher_id, excluded);
  }
}

void RewardsServiceImpl::OnPanelPublisherInfo(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    return;
  }

  for (auto& observer : private_observers_)
    observer.OnPanelPublisherInfo(this,
                                  result,
                                  std::move(info),
                                  windowId);
}

void RewardsServiceImpl::GetContributionAmount(
    const GetContributionAmountCallback& callback) {
  if (!Connected())
    return;

  bat_ledger_->GetContributionAmount(callback);
}

void RewardsServiceImpl::FetchFavIcon(const std::string& url,
                                      const std::string& favicon_key,
                                      ledger::FetchIconCallback callback) {
  GURL parsedUrl(url);

  if (!parsedUrl.is_valid()) {
    return;
  }

  std::vector<std::string>::iterator it;
  it = find(current_media_fetchers_.begin(),
      current_media_fetchers_.end(), url);
  if (it != current_media_fetchers_.end()) {
    LOG(WARNING) << "Already fetching favicon: " << url;
    return;
  }

  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_rewards_favicon_fetcher", R"(
        semantics {
          sender:
            "Brave Rewards Media Fetcher"
          description:
            "Fetches favicon for media publishers in Rewards."
          trigger:
            "User visits a media publisher content."
          data: "Favicon for media publisher."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");
    current_media_fetchers_.emplace_back(url);
    request_ids_.push_back(image_service->RequestImage(
          parsedUrl,
          // Image Service takes ownership of the observer
          new RewardsFetcherServiceObserver(
              favicon_key,
              parsedUrl,
              base::Bind(&RewardsServiceImpl::OnFetchFavIconCompleted,
                  base::Unretained(this), callback)),
          traffic_annotation));
  }
}

void RewardsServiceImpl::OnFetchFavIconCompleted(
    ledger::FetchIconCallback callback,
    const std::string& favicon_key,
    const GURL& url,
    const BitmapFetcherService::RequestId& request_id,
    const SkBitmap& image) {
  GURL favicon_url(favicon_key);
  gfx::Image gfx_image = gfx::Image::CreateFrom1xBitmap(image);
  favicon::FaviconService* favicon_service =
          FaviconServiceFactory::GetForProfile(profile_,
              ServiceAccessType::EXPLICIT_ACCESS);
  favicon_service->SetOnDemandFavicons(
      favicon_url,
      url,
      favicon_base::IconType::kFavicon,
      gfx_image,
      base::BindOnce(&RewardsServiceImpl::OnSetOnDemandFaviconComplete,
          AsWeakPtr(), favicon_url.spec(), callback));

  std::vector<std::string>::iterator it_url;
  it_url = find(current_media_fetchers_.begin(),
      current_media_fetchers_.end(), url.spec());
  if (it_url != current_media_fetchers_.end()) {
    current_media_fetchers_.erase(it_url);
  }

  std::vector<BitmapFetcherService::RequestId>::iterator it_ids;
  it_ids = find(request_ids_.begin(), request_ids_.end(), request_id);
  if (it_ids != request_ids_.end()) {
    request_ids_.erase(it_ids);
  }
}

void RewardsServiceImpl::OnSetOnDemandFaviconComplete(
    const std::string& favicon_url,
    ledger::FetchIconCallback callback, bool success) {
  if (!Connected())
    return;

  callback(success, favicon_url);
}

void RewardsServiceImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  if (!Connected())
    return;

  bat_ledger_->GetPublisherBanner(publisher_id,
      base::BindOnce(&RewardsServiceImpl::OnPublisherBanner,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnPublisherBanner(
    GetPublisherBannerCallback callback,
    const std::string& banner) {
  std::unique_ptr<brave_rewards::PublisherBanner> new_banner;
  new_banner.reset(new brave_rewards::PublisherBanner());

  std::unique_ptr<ledger::PublisherBanner> publisher_banner;
  publisher_banner.reset(new ledger::PublisherBanner());

  if (!banner.empty()) {
    publisher_banner->loadFromJson(banner);
  }

  if (!publisher_banner) {
    return;
  }

  new_banner->publisher_key = publisher_banner->publisher_key;
  new_banner->title = publisher_banner->title;
  new_banner->name = publisher_banner->name;
  new_banner->description = publisher_banner->description;
  new_banner->background = publisher_banner->background;
  new_banner->logo = publisher_banner->logo;
  new_banner->amounts = publisher_banner->amounts;
  new_banner->social = publisher_banner->social;
  new_banner->provider = publisher_banner->provider;
  new_banner->verified = publisher_banner->verified;

  std::move(callback).Run(std::move(new_banner));
}

void RewardsServiceImpl::OnDonate_PublisherInfoSaved(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
}

void RewardsServiceImpl::OnDonate(const std::string& publisher_key, int amount,
  bool recurring, const ledger::PublisherInfo* publisher_info) {
  if (recurring) {
    // TODO(nejczdovc): this needs to be wired through ledger code
    // If caller provided publisher info, save it to `publisher_info` table
    if (publisher_info) {
      auto publisher_copy = std::make_unique<ledger::PublisherInfo>(
        *publisher_info);
      SavePublisherInfo(std::move(publisher_copy),
        std::bind(&RewardsServiceImpl::OnDonate_PublisherInfoSaved,
            this, _1, _2));
    }

    SaveRecurringTip(publisher_key, amount);
    return;
  }

  if (!Connected())
    return;

  ledger::PublisherInfo publisher(publisher_key);

  bat_ledger_->DoDirectDonation(publisher.ToJson(), amount, "BAT");
}

bool SaveContributionInfoOnFileTaskRunner(
    const brave_rewards::ContributionInfo info,
  PublisherInfoDatabase* backend) {
  if (backend && backend->InsertContributionInfo(info))
    return true;

  return false;
}

void RewardsServiceImpl::OnContributionInfoSaved(
    const ledger::REWARDS_CATEGORY category,
    bool success) {
  for (auto& observer : observers_) {
    observer.OnContributionSaved(this, success, category);
  }
}

void RewardsServiceImpl::SaveContributionInfo(const std::string& probi,
  const int month,
  const int year,
  const uint32_t date,
  const std::string& publisher_key,
  const ledger::REWARDS_CATEGORY category) {
  brave_rewards::ContributionInfo info;
  info.probi = probi;
  info.month = month;
  info.year = year;
  info.date = date;
  info.publisher_key = publisher_key;
  info.category = category;

  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&SaveContributionInfoOnFileTaskRunner,
                    info,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnContributionInfoSaved,
                     AsWeakPtr(),
                     category));
}

bool SaveRecurringTipOnFileTaskRunner(
    const brave_rewards::RecurringDonation info,
  PublisherInfoDatabase* backend) {
  if (backend && backend->InsertOrUpdateRecurringTip(info))
    return true;

  return false;
}

void RewardsServiceImpl::OnRecurringTipSaved(bool success) {
  for (auto& observer : observers_) {
    observer.OnRecurringTipSaved(this, success);
  }
}

void RewardsServiceImpl::SaveRecurringTip(
    const std::string& publisher_key, const int amount) {
  brave_rewards::RecurringDonation info;
  info.publisher_key = publisher_key;
  info.amount = amount;
  info.added_date = GetCurrentTimestamp();

  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&SaveRecurringTipOnFileTaskRunner,
                    info,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnRecurringTipSaved,
                     AsWeakPtr()));
}

ledger::PublisherInfoList GetRecurringTipsOnFileTaskRunner(
    PublisherInfoDatabase* backend) {
  ledger::PublisherInfoList list;
  if (!backend) {
    return list;
  }

  backend->GetRecurringTips(&list);

  return list;
}

void RewardsServiceImpl::OnGetRecurringTipsUI(
    GetRecurringTipsCallback callback,
    const std::vector<std::string>& json_list) {
    std::unique_ptr<brave_rewards::ContentSiteList> new_list(
      new brave_rewards::ContentSiteList);

  for (auto &json_publisher : json_list) {
    ledger::PublisherInfo publisher;
    publisher.loadFromJson(json_publisher);
    brave_rewards::ContentSite site = PublisherInfoToContentSite(publisher);
    site.percentage = publisher.weight;
    new_list->push_back(site);
  }

  std::move(callback).Run(std::move(new_list));
}

void RewardsServiceImpl::GetRecurringTipsUI(
    GetRecurringTipsCallback callback) {
  bat_ledger_->GetRecurringTips(
      base::BindOnce(&RewardsServiceImpl::OnGetRecurringTipsUI,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetRecurringTips(
    const ledger::PublisherInfoListCallback callback,
    const ledger::PublisherInfoList list) {
  if (!Connected()) {
    return;
  }

  callback(list, 0);
}

void RewardsServiceImpl::GetRecurringTips(
    ledger::PublisherInfoListCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&GetRecurringTipsOnFileTaskRunner,
                 publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnGetRecurringTips,
                 AsWeakPtr(),
                 callback));
}

void RewardsServiceImpl::OnGetOneTimeTipsUI(
    GetRecurringTipsCallback callback,
    const std::vector<std::string>& json_list) {
    std::unique_ptr<brave_rewards::ContentSiteList> new_list(
      new brave_rewards::ContentSiteList);

  for (auto &json_publisher : json_list) {
    ledger::PublisherInfo publisher;
    publisher.loadFromJson(json_publisher);
    brave_rewards::ContentSite site = PublisherInfoToContentSite(publisher);
    site.percentage = publisher.weight;
    new_list->push_back(site);
  }

  std::move(callback).Run(std::move(new_list));
}

void RewardsServiceImpl::GetOneTimeTipsUI(GetOneTimeTipsCallback callback) {
  bat_ledger_->GetOneTimeTips(
      base::BindOnce(&RewardsServiceImpl::OnGetOneTimeTipsUI,
                     AsWeakPtr(),
                     std::move(callback)));
}

ledger::PublisherInfoList GetOneTimeTipsOnFileTaskRunner(
    PublisherInfoDatabase* backend) {
  ledger::PublisherInfoList list;
  if (!backend) {
    return list;
  }

  auto now = base::Time::Now();
  backend->GetOneTimeTips(&list, GetPublisherMonth(now), GetPublisherYear(now));

  return list;
}

void RewardsServiceImpl::GetOneTimeTips(
    ledger::PublisherInfoListCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&GetOneTimeTipsOnFileTaskRunner,
                 publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnGetOneTimeTips,
                 AsWeakPtr(),
                 callback));
}

void RewardsServiceImpl::OnGetOneTimeTips(
    ledger::PublisherInfoListCallback callback,
    const ledger::PublisherInfoList list) {
  if (!Connected()) {
    return;
  }

  callback(list, 0);
}

void RewardsServiceImpl::RemoveRecurringTip(const std::string& publisher_key) {
  if (!Connected())
    return;

  bat_ledger_->RemoveRecurringTip(publisher_key);
}

bool RemoveRecurringTipOnFileTaskRunner(
    const std::string publisher_key, PublisherInfoDatabase* backend) {
  if (!backend) {
    return false;
  }

  return backend->RemoveRecurringTip(publisher_key);
}

void RewardsServiceImpl::OnRemovedRecurringTip(
    ledger::RecurringRemoveCallback callback, bool success) {
  if (!Connected()) {
    callback(success ?
        ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
  }

  for (auto& observer : observers_) {
    observer.OnRecurringTipRemoved(this, success);
  }
}

void RewardsServiceImpl::OnRemoveRecurring(const std::string& publisher_key,
    ledger::RecurringRemoveCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&RemoveRecurringTipOnFileTaskRunner,
                    publisher_key,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnRemovedRecurringTip,
                     AsWeakPtr(), callback));
}

void RewardsServiceImpl::TriggerOnGetCurrentBalanceReport(
    const ledger::BalanceReportInfo& report) {
  for (auto& observer : private_observers_) {
    brave_rewards::BalanceReport balance_report;
    balance_report.opening_balance = report.opening_balance_;
    balance_report.closing_balance = report.closing_balance_;
    balance_report.grants = report.grants_;
    balance_report.earning_from_ads = report.earning_from_ads_;
    balance_report.auto_contribute = report.auto_contribute_;
    balance_report.recurring_donation = report.recurring_donation_;
    balance_report.one_time_donation = report.one_time_donation_;
    observer.OnGetCurrentBalanceReport(this, balance_report);
  }
}

void RewardsServiceImpl::SetContributionAutoInclude(
    const std::string& publisher_key,
    bool excluded) {
  if (!Connected())
    return;

  ledger::PUBLISHER_EXCLUDE exclude =
      excluded
      ? ledger::PUBLISHER_EXCLUDE::EXCLUDED
      : ledger::PUBLISHER_EXCLUDE::INCLUDED;

  bat_ledger_->SetPublisherExclude(publisher_key, exclude);
}

RewardsNotificationService* RewardsServiceImpl::GetNotificationService() const {
  return notification_service_.get();
}

void RewardsServiceImpl::StartNotificationTimers(bool main_enabled) {
  if (!main_enabled) return;

  // Startup timer, begins after 30-second delay.
  PrefService* pref_service = profile_->GetPrefs();
  notification_startup_timer_ = std::make_unique<base::OneShotTimer>();
  notification_startup_timer_->Start(
      FROM_HERE,
      pref_service->GetTimeDelta(
        prefs::kRewardsNotificationStartupDelay),
      this,
      &RewardsServiceImpl::OnNotificationTimerFired);
  DCHECK(notification_startup_timer_->IsRunning());

  // Periodic timer, runs once per day by default.
  base::TimeDelta periodic_timer_interval =
      pref_service->GetTimeDelta(prefs::kRewardsNotificationTimerInterval);
  notification_periodic_timer_ = std::make_unique<base::RepeatingTimer>();
  notification_periodic_timer_->Start(
      FROM_HERE, periodic_timer_interval, this,
      &RewardsServiceImpl::OnNotificationTimerFired);
  DCHECK(notification_periodic_timer_->IsRunning());
}

void RewardsServiceImpl::StopNotificationTimers() {
  notification_startup_timer_.reset();
  notification_periodic_timer_.reset();
}

void RewardsServiceImpl::OnNotificationTimerFired() {
  if (!Connected())
    return;

  bat_ledger_->GetBootStamp(
      base::BindOnce(&RewardsServiceImpl::MaybeShowBackupNotification,
        AsWeakPtr()));
  GetReconcileStamp(
      base::Bind(&RewardsServiceImpl::MaybeShowAddFundsNotification,
        AsWeakPtr()));
  FetchGrants(std::string(), std::string());
}

void RewardsServiceImpl::MaybeShowNotificationAddFunds() {
  bat_ledger_->HasSufficientBalanceToReconcile(
      base::BindOnce(&RewardsServiceImpl::ShowNotificationAddFunds,
        AsWeakPtr()));
}

bool RewardsServiceImpl::ShouldShowNotificationAddFunds() const {
  base::Time next_time =
      profile_->GetPrefs()->GetTime(prefs::kRewardsAddFundsNotification);
  return (next_time.is_null() || base::Time::Now() > next_time);
}

void RewardsServiceImpl::ShowNotificationAddFunds(bool sufficient) {
  if (sufficient) return;

  base::Time next_time = base::Time::Now() + base::TimeDelta::FromDays(3);
  profile_->GetPrefs()->SetTime(prefs::kRewardsAddFundsNotification, next_time);
  RewardsNotificationService::RewardsNotificationArgs args;
  notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS, args,
      "rewards_notification_insufficient_funds");
}

void RewardsServiceImpl::MaybeShowNotificationTipsPaid() {
  GetAutoContribute(base::BindOnce(
      &RewardsServiceImpl::ShowNotificationTipsPaid,
      AsWeakPtr()));
}

void RewardsServiceImpl::ShowNotificationTipsPaid(bool ac_enabled) {
  if (ac_enabled)
    return;

  RewardsNotificationService::RewardsNotificationArgs args;
  notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_TIPS_PROCESSED, args,
      "rewards_notification_tips_processed");
}

std::unique_ptr<ledger::LogStream> RewardsServiceImpl::Log(
    const char* file,
    int line,
    const ledger::LogLevel log_level) const {
  return std::make_unique<LogStreamImpl>(file, line, log_level);
}

std::unique_ptr<ledger::LogStream> RewardsServiceImpl::VerboseLog(
                     const char* file,
                     int line,
                     int log_level) const {
  return std::make_unique<LogStreamImpl>(file, line, log_level);
}

// static
void RewardsServiceImpl::HandleFlags(const std::string& options) {
  std::vector<std::string> flags = base::SplitString(
      options, ",", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (const auto& flag : flags) {
    if (flag.empty()) {
      continue;
    }

    std::vector<std::string> values = base::SplitString(
      flag, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    if (values.size() != 2) {
      continue;
    }

    std::string name = base::ToLowerASCII(values[0]);
    std::string value = values[1];

    if (value.empty()) {
      continue;
    }

    if (name == "staging") {
      bool is_production;
      std::string lower = base::ToLowerASCII(value);

      if (lower == "true" || lower == "1") {
        is_production = false;
      } else {
        is_production = true;
      }

      SetProduction(is_production);
      continue;
    }

    if (name == "debug") {
      bool is_debug;
      std::string lower = base::ToLowerASCII(value);

      if (lower == "true" || lower == "1") {
        is_debug = true;
      } else {
        is_debug = false;
      }

      SetDebug(is_debug);
      continue;
    }

    if (name == "reconcile-interval") {
      int reconcile_int;
      bool success = base::StringToInt(value, &reconcile_int);

      if (success && reconcile_int > 0) {
        SetReconcileTime(reconcile_int);
      }

      continue;
    }

    if (name == "short-retries") {
      std::string lower = base::ToLowerASCII(value);
      bool short_retries;

      if (lower == "true" || lower == "1") {
        short_retries = true;
      } else {
        short_retries = false;
      }

      SetShortRetries(short_retries);
    }
  }
}

bool RewardsServiceImpl::CheckImported() {
  PrefService* prefs = profile_->GetOriginalProfile()->GetPrefs();
  const int pinned_item_count = prefs->GetInteger(
      kBravePaymentsPinnedItemCount);
  if (pinned_item_count > 0) {
    prefs->SetInteger(kBravePaymentsPinnedItemCount, 0);
  }

  return pinned_item_count > 0;
}

void RewardsServiceImpl::SetBackupCompleted() {
  profile_->GetPrefs()->SetBoolean(prefs::kRewardsBackupSucceeded, true);
}

void RewardsServiceImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  bat_ledger_->GetRewardsInternalsInfo(
      base::BindOnce(&RewardsServiceImpl::OnGetRewardsInternalsInfo,
                     AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::OnDonate(
    const std::string& publisher_key,
    int amount,
    bool recurring,
    std::unique_ptr<brave_rewards::ContentSite> site) {

  if (!site) {
    return;
  }

  ledger::PublisherInfo info;
  info.id = publisher_key;
  info.verified = site->verified;
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.name = site->name;
  info.url = site->url;
  info.provider = site->provider;
  info.favicon_url = site->favicon_url;

  OnDonate(publisher_key, amount, recurring, &info);
}

bool RewardsServiceImpl::Connected() const {
  return bat_ledger_.is_bound();
}

void RewardsServiceImpl::SetLedgerEnvForTesting() {
  bat_ledger_service_->SetTesting();

  // this is needed because we are using braveledger_bat_helper::buildURL
  // directly in BraveRewardsBrowserTest
  #if defined(OFFICIAL_BUILD)
  ledger::is_production = true;
  #else
  ledger::is_production = false;
  #endif
}

void RewardsServiceImpl::GetProduction(const GetProductionCallback& callback) {
  bat_ledger_service_->GetProduction(callback);
}

void RewardsServiceImpl::GetDebug(const GetDebugCallback& callback) {
  bat_ledger_service_->GetDebug(callback);
}

void RewardsServiceImpl::GetReconcileTime(
    const GetReconcileTimeCallback& callback) {
  bat_ledger_service_->GetReconcileTime(callback);
}

void RewardsServiceImpl::GetShortRetries(
    const GetShortRetriesCallback& callback) {
  bat_ledger_service_->GetShortRetries(callback);
}

void RewardsServiceImpl::SetProduction(bool production) {
  bat_ledger_service_->SetProduction(production);
}

void RewardsServiceImpl::SetDebug(bool debug) {
  bat_ledger_service_->SetDebug(debug);
}

void RewardsServiceImpl::SetReconcileTime(int32_t time) {
  bat_ledger_service_->SetReconcileTime(time);
}

void RewardsServiceImpl::SetShortRetries(bool short_retries) {
  bat_ledger_service_->SetShortRetries(short_retries);
}

ledger::Result SavePendingContributionOnFileTaskRunner(
    PublisherInfoDatabase* backend,
    const ledger::PendingContributionList& list) {
  if (!backend) {
    return ledger::Result::LEDGER_ERROR;
  }

  bool result = backend->InsertPendingContribution(list);

  return result ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR;
}

void RewardsServiceImpl::OnSavePendingContribution(ledger::Result result) {
  for (auto& observer : observers_)
    observer.OnPendingContributionSaved(this, result);
}

void RewardsServiceImpl::SavePendingContribution(
      const ledger::PendingContributionList& list) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::Bind(&SavePendingContributionOnFileTaskRunner,
                 publisher_info_backend_.get(),
                 list),
      base::Bind(&RewardsServiceImpl::OnSavePendingContribution,
                 AsWeakPtr()));
}

double PendingContributionsTotalOnFileTaskRunner(
    PublisherInfoDatabase* backend) {
  if (!backend) {
    return 0;
  }

  return backend->GetReservedAmount();
}

void RewardsServiceImpl::GetPendingContributionsTotal(
    const GetPendingContributionsTotalCallback& callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::Bind(&PendingContributionsTotalOnFileTaskRunner,
                 publisher_info_backend_.get()),
      callback);
}

bool RestorePublisherOnFileTaskRunner(PublisherInfoDatabase* backend) {
  if (!backend) {
    return false;
  }

  return backend->RestorePublishers();
}

void RewardsServiceImpl::OnRestorePublishers(
    ledger::OnRestoreCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::Bind(&RestorePublisherOnFileTaskRunner,
                 publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnRestorePublishersInternal,
                 AsWeakPtr(),
                 callback));
}

void RewardsServiceImpl::OnRestorePublishersInternal(
    ledger::OnRestoreCallback callback,
    bool result) {
  callback(result);
}

std::unique_ptr<ledger::PublisherInfoList>
SaveNormalizedPublisherListOnFileTaskRunner(PublisherInfoDatabase* backend,
    const ledger::PublisherInfoList& list) {
  if (!backend) {
    return nullptr;
  }

  bool success = backend->InsertOrUpdateActivityInfos(list);

  if (!success) {
    return nullptr;
  }

  std::unique_ptr<ledger::PublisherInfoList> new_list(
      new ledger::PublisherInfoList);

  for (auto& publisher : list) {
    new_list->push_back(publisher);
  }

  return new_list;
}

void RewardsServiceImpl::SaveNormalizedPublisherList(
      const ledger::PublisherInfoListStruct& list) {
  if (list.list.size() == 0) {
    std::unique_ptr<ledger::PublisherInfoList> empty_list(
        new ledger::PublisherInfoList);
    OnPublisherListNormalizedSaved(std::move(empty_list));
    return;
  }

  base::PostTaskAndReplyWithResult(
    file_task_runner_.get(),
    FROM_HERE,
    base::Bind(&SaveNormalizedPublisherListOnFileTaskRunner,
               publisher_info_backend_.get(),
               list.list),
    base::Bind(&RewardsServiceImpl::OnPublisherListNormalizedSaved,
               AsWeakPtr()));
}

void RewardsServiceImpl::OnPublisherListNormalizedSaved(
    std::unique_ptr<ledger::PublisherInfoList> list) {
  if (!list) {
    LOG(ERROR) << "Problem saving normalized publishers "
                  "in SaveNormalizedPublisherList";
    return;
  }

  ContentSiteList site_list;
  for (auto& publisher : *list) {
    site_list.push_back(PublisherInfoToContentSite(publisher));
  }

  sort(site_list.begin(), site_list.end());

  for (auto& observer : observers_) {
    observer.OnPublisherListNormalized(this, site_list);
  }
}

void RewardsServiceImpl::GetAddressesForPaymentId(
    const GetAddressesCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAddressesForPaymentId(
      base::BindOnce(&RewardsServiceImpl::OnGetAddresses,
                     AsWeakPtr(),
                     callback));
}

int GetExcludedPublishersNumberOnFileTaskRunner(
    PublisherInfoDatabase* backend) {
  if (!backend) {
    return 0;
  }

  return backend->GetExcludedPublishersCount();
}

void RewardsServiceImpl::OnGetExcludedPublishersNumberDB(
    ledger::GetExcludedPublishersNumberDBCallback callback,
    int number) {
  if (!Connected()) {
    callback(0);
    return;
  }

  callback(number);
}

void RewardsServiceImpl::GetExcludedPublishersNumberDB(
      ledger::GetExcludedPublishersNumberDBCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::BindOnce(&GetExcludedPublishersNumberOnFileTaskRunner,
                     publisher_info_backend_.get()),
      base::BindOnce(&RewardsServiceImpl::OnGetExcludedPublishersNumberDB,
                     AsWeakPtr(),
                     callback));
}

bool DeleteActivityInfoOnFileTaskRunner(PublisherInfoDatabase* backend,
                                        const std::string& publisher_key,
                                        uint64_t reconcile_stamp) {
  if (backend &&
      backend->DeleteActivityInfo(publisher_key, reconcile_stamp)) {
    return true;
  }

  return false;
}

void RewardsServiceImpl::DeleteActivityInfo(const std::string& publisher_key) {
  GetReconcileStamp(
      base::Bind(&RewardsServiceImpl::OnDeleteActivityInfoStamp,
                 AsWeakPtr(),
                 publisher_key));
}

void RewardsServiceImpl::OnDeleteActivityInfoStamp(
    const std::string& publisher_key,
    uint64_t reconcile_stamp) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::Bind(&DeleteActivityInfoOnFileTaskRunner,
                 publisher_info_backend_.get(),
                 publisher_key,
                 reconcile_stamp),
      base::Bind(&RewardsServiceImpl::OnDeleteActivityInfo,
                 AsWeakPtr(),
                 publisher_key));
}

void RewardsServiceImpl::OnDeleteActivityInfo(
    const std::string& publisher_key,
    bool result) {
  if (!result) {
    LOG(ERROR) << "Problem deleting activity info for "
               << publisher_key;
  }
}

void RewardsServiceImpl::RefreshPublisher(
    const std::string& publisher_key,
    RefreshPublisherCallback callback) {
  if (!Connected()) {
    std::move(callback).Run(false, std::string());
    return;
  }
  bat_ledger_->RefreshPublisher(
      publisher_key,
      base::BindOnce(&RewardsServiceImpl::OnRefreshPublisher,
        AsWeakPtr(), std::move(callback), publisher_key));
}

void RewardsServiceImpl::OnRefreshPublisher(
    RefreshPublisherCallback callback,
    const std::string& publisher_key,
    bool verified) {
  std::move(callback).Run(verified, publisher_key);
}

}  // namespace brave_rewards
