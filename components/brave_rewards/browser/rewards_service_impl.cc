/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service_impl.h"

#include <functional>
#include <limits.h>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/guid.h"
#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/media_publisher_info.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/wallet_info.h"
#include "brave/browser/ui/webui/brave_rewards_source.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/publisher_info_database.h"
#include "brave/components/brave_rewards/browser/rewards_fetcher_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/switches.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/url_data_source.h"
#include "content_site.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/escape.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "publisher_banner.h"
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

using namespace net::registry_controlled_domains;
using namespace std::placeholders;

namespace brave_rewards {

class LogStreamImpl : public ledger::LogStream {
 public:
  LogStreamImpl(const char* file,
                int line,
                const ledger::LogLevel log_level) {
    switch(log_level) {
      case ledger::LogLevel::LOG_INFO:
        log_message_ = std::make_unique<logging::LogMessage>(file, line, logging::LOG_INFO);
        break;
      case ledger::LogLevel::LOG_WARNING:
        log_message_ = std::make_unique<logging::LogMessage>(file, line, logging::LOG_WARNING);
        break;
      case ledger::LogLevel::LOG_ERROR:
        log_message_ = std::make_unique<logging::LogMessage>(file, line, logging::LOG_ERROR);
        break;
      default:
        log_message_ = std::make_unique<logging::LogMessage>(file, line, logging::LOG_VERBOSE);
        break;
    }
  }

  std::ostream& stream() override {
    return log_message_->stream();
  }

 private:
  std::unique_ptr<logging::LogMessage> log_message_;
  DISALLOW_COPY_AND_ASSIGN(LogStreamImpl);
};

namespace {

ledger::PUBLISHER_MONTH GetPublisherMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return (ledger::PUBLISHER_MONTH)exploded.month;
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
  content_site.reconcile_stamp = publisher_info.reconcile_stamp;
  return content_site;
}

net::URLFetcher::RequestType URLMethodToRequestType(ledger::URL_METHOD method) {
  switch(method) {
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
  if (backend && backend->InsertOrUpdateMediaPublisherInfo(media_key, publisher_id))
    return true;

  return false;
}

std::unique_ptr<ledger::PublisherInfo>
LoadMediaPublisherInfoListOnFileTaskRunner(
    const std::string media_key,
    PublisherInfoDatabase* backend) {
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!backend)
    return info;

  info = backend->GetMediaPublisherInfo(media_key);
  return info;
}

bool SavePublisherInfoOnFileTaskRunner(
    const ledger::PublisherInfo publisher_info,
    PublisherInfoDatabase* backend) {
  if (backend && backend->InsertOrUpdatePublisherInfo(publisher_info))
    return true;

  return false;
}

ledger::PublisherInfoList LoadPublisherInfoListOnFileTaskRunner(
    uint32_t start,
    uint32_t limit,
    ledger::PublisherInfoFilter filter,
    PublisherInfoDatabase* backend) {
  ledger::PublisherInfoList list;
  if (!backend)
    return list;

  ignore_result(backend->Find(start, limit, filter, &list));
  return list;
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

void GetContentSiteListInternal(
    uint32_t start,
    uint32_t limit,
    const GetCurrentContributeListCallback& callback,
    const ledger::PublisherInfoList& publisher_list,
    uint32_t next_record) {
  std::unique_ptr<ContentSiteList> site_list(new ContentSiteList);
  for (ledger::PublisherInfoList::const_iterator it =
      publisher_list.begin(); it != publisher_list.end(); ++it) {
    site_list->push_back(PublisherInfoToContentSite(*it));
  }
  callback.Run(std::move(site_list), next_record);
}

time_t GetCurrentTimestamp() {
  return base::Time::NowFromSystemTime().ToTimeT();
}

}  // namespace

bool IsMediaLink(const GURL& url,
                 const GURL& first_party_url,
                 const GURL& referrer) {
  return ledger::Ledger::IsMediaLink(url.spec(),
                                     first_party_url.spec(),
                                     referrer.spec());
}


//read comment about file pathes at src\base\files\file_path.h
#if defined(OS_WIN)
const base::FilePath::StringType kLedger_state(L"ledger_state");
const base::FilePath::StringType kPublisher_state(L"publisher_state");
const base::FilePath::StringType kPublisher_info_db(L"publisher_info_db");
const base::FilePath::StringType kPublishers_list(L"publishers_list");
#else
const base::FilePath::StringType kLedger_state("ledger_state");
const base::FilePath::StringType kPublisher_state("publisher_state");
const base::FilePath::StringType kPublisher_info_db("publisher_info_db");
const base::FilePath::StringType kPublishers_list("publishers_list");
#endif

RewardsServiceImpl::RewardsServiceImpl(Profile* profile)
    : profile_(profile),
      ledger_(ledger::Ledger::CreateInstance(this)),
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
      publisher_info_backend_(
          new PublisherInfoDatabase(publisher_info_db_path_)),
      notification_service_(new RewardsNotificationServiceImpl(profile)),
#if BUILDFLAG(ENABLE_EXTENSIONS)
      private_observer_(
          std::make_unique<ExtensionRewardsServiceObserver>(profile_)),
#endif
      next_timer_id_(0) {
  // Environment
  #if defined(OFFICIAL_BUILD)
    ledger::is_production = true;
  #else
    ledger::is_production = false;
  #endif

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kRewards)) {
    std::string options = command_line.GetSwitchValueASCII(switches::kRewards);

    if (!options.empty()) {
      HandleFlags(options);
    }
  }

  // Set up the rewards data source
  content::URLDataSource::Add(profile_,
                              std::make_unique<BraveRewardsSource>(profile_));
}

RewardsServiceImpl::~RewardsServiceImpl() {
  file_task_runner_->DeleteSoon(FROM_HERE, publisher_info_backend_.release());
  StopNotificationTimers();
}

void RewardsServiceImpl::Init() {
  AddObserver(notification_service_.get());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  AddObserver(extension_rewards_service_observer_.get());
  private_observers_.AddObserver(private_observer_.get());
#endif
  ledger_->Initialize();
}

void RewardsServiceImpl::MaybeShowBackupNotification() {
  PrefService* pref_service = profile_->GetPrefs();
  bool user_has_funded = pref_service->GetBoolean(kRewardsUserHasFunded);
  bool backup_succeeded = pref_service->GetBoolean(kRewardsBackupSucceeded);
  if (user_has_funded && !backup_succeeded) {
    base::Time now = base::Time::Now();
    base::Time boot_timestamp =
        base::Time::FromDoubleT(ledger_->GetBootStamp());
    base::TimeDelta backup_notification_frequency =
        pref_service->GetTimeDelta(kRewardsBackupNotificationFrequency);
    base::TimeDelta backup_notification_interval =
        pref_service->GetTimeDelta(kRewardsBackupNotificationInterval);
    base::TimeDelta elapsed = now - boot_timestamp;
    if (elapsed > backup_notification_interval) {
      base::TimeDelta next_backup_notification_interval =
          backup_notification_interval + backup_notification_frequency;
      pref_service->SetTimeDelta(kRewardsBackupNotificationInterval,
                                 next_backup_notification_interval);
      RewardsNotificationService::RewardsNotificationArgs args;
      notification_service_->AddNotification(
          RewardsNotificationService::REWARDS_NOTIFICATION_BACKUP_WALLET, args,
          "rewards_notification_backup_wallet");
    }
  }
}

void RewardsServiceImpl::MaybeShowAddFundsNotification() {
  // Show add funds notification if reconciliation will occur in the
  // next 3 days and balance is too low.
  base::Time now = base::Time::Now();
  if (ledger_->GetReconcileStamp() - now.ToDoubleT() <
      3 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour) {
    if (!HasSufficientBalanceToReconcile() &&
        ShouldShowNotificationAddFunds()) {
      ShowNotificationAddFunds();
    }
  }
}

void RewardsServiceImpl::CreateWallet() {
  if (ready().is_signaled()) {
    ledger_->CreateWallet();
  } else {
    ready().Post(FROM_HERE,
        base::Bind(&brave_rewards::RewardsService::CreateWallet,
            base::Unretained(this)));
  }
}

void RewardsServiceImpl::GetCurrentContributeList(
    uint32_t start,
    uint32_t limit,
    const GetCurrentContributeListCallback& callback) {
  ledger::PublisherInfoFilter filter;
  filter.category = ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE;
  filter.month = ledger::PUBLISHER_MONTH::ANY;
  filter.year = -1;
  filter.min_duration = ledger_->GetPublisherMinVisitTime();
  filter.order_by.push_back(std::pair<std::string, bool>("ai.percent", false));
  filter.reconcile_stamp = ledger_->GetReconcileStamp();
  filter.excluded =
    ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED;
  filter.percent = 1;
  filter.non_verified = ledger_->GetPublisherAllowNonVerified();

  ledger_->GetPublisherInfoList(
      start,
      limit,
      filter,
      std::bind(&GetContentSiteListInternal,
                start,
                limit,
                callback, _1, _2));
}

void RewardsServiceImpl::OnLoad(SessionID tab_id, const GURL& url) {
  auto origin = url.GetOrigin();
  const std::string baseDomain =
      GetDomainAndRegistry(origin.host(), INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "")
    return;

  const std::string publisher_url = origin.scheme() + "://" + baseDomain + "/";

  auto now = base::Time::Now();
  ledger::VisitData data(baseDomain,
                         origin.host(),
                         url.path(),
                         tab_id.id(),
                         GetPublisherMonth(now),
                         GetPublisherYear(now),
                         baseDomain,
                         publisher_url,
                         "",
                         "");
  ledger_->OnLoad(data, GetCurrentTimestamp());
}

void RewardsServiceImpl::OnUnload(SessionID tab_id) {

  ledger_->OnUnload(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnShow(SessionID tab_id) {
  ledger_->OnShow(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnHide(SessionID tab_id) {
  ledger_->OnHide(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnForeground(SessionID tab_id) {
  ledger_->OnForeground(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnBackground(SessionID tab_id) {
  ledger_->OnBackground(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnMediaStart(SessionID tab_id) {
  ledger_->OnMediaStart(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnMediaStop(SessionID tab_id) {
  ledger_->OnMediaStop(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnPostData(SessionID tab_id,
                                    const GURL& url,
                                    const GURL& first_party_url,
                                    const GURL& referrer,
                                    const std::string& post_data) {
  std::string output;
  url::RawCanonOutputW<1024> canonOutput;
  url::DecodeURLEscapeSequences(post_data.c_str(),
                                post_data.length(),
                                &canonOutput);
  output = base::UTF16ToUTF8(base::StringPiece16(canonOutput.data(),
                                                 canonOutput.length()));

  if (output.empty())
    return;

  auto now = base::Time::Now();
  ledger::VisitData visit_data(
      "",
      "",
      url.spec(),
      tab_id.id(),
      GetPublisherMonth(now),
      GetPublisherYear(now),
      "",
      "",
      "",
      "");

  ledger_->OnPostData(url.spec(),
                      first_party_url.spec(),
                      referrer.spec(),
                      output,
                      visit_data);
}

void RewardsServiceImpl::OnXHRLoad(SessionID tab_id,
                                   const GURL& url,
                                   const GURL& first_party_url,
                                   const GURL& referrer) {
  std::map<std::string, std::string> parts;

  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    parts[it.GetKey()] = it.GetUnescapedValue();
  }

  auto now = base::Time::Now();
  ledger::VisitData data("", "", url.spec(), tab_id.id(),
                         GetPublisherMonth(now), GetPublisherYear(now),
                         "", "", "", "");

  ledger_->OnXHRLoad(tab_id.id(),
                     url.spec(),
                     parts,
                     first_party_url.spec(),
                     referrer.spec(),
                     data);
}

void RewardsServiceImpl::LoadMediaPublisherInfo(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadMediaPublisherInfoListOnFileTaskRunner,
          media_key, publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnMediaPublisherInfoLoaded,
                     AsWeakPtr(),
                     callback));
}

void RewardsServiceImpl::OnMediaPublisherInfoLoaded(
    ledger::PublisherInfoCallback callback,
    std::unique_ptr<ledger::PublisherInfo> info) {
  if (!info) {
    callback(ledger::Result::NOT_FOUND, std::move(info));
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

void RewardsServiceImpl::ExcludePublisher(const std::string publisherKey) const {
  ledger_->SetPublisherExclude(publisherKey, ledger::PUBLISHER_EXCLUDE::EXCLUDED);
}

void RewardsServiceImpl::RestorePublishers() {
  ledger_->RestorePublishers();
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

  ledger_.reset();
  RewardsService::Shutdown();
}

void RewardsServiceImpl::OnWalletInitialized(ledger::Result result) {
  if (!ready_.is_signaled())
    ready_.Signal();

  if (result == ledger::Result::WALLET_CREATED) {
    SetRewardsMainEnabled(true);
    SetAutoContribute(true);
    StartNotificationTimers();
    result = ledger::Result::LEDGER_OK;
  }

  TriggerOnWalletInitialized(result);
}

void RewardsServiceImpl::OnWalletProperties(ledger::Result result,
    std::unique_ptr<ledger::WalletInfo> wallet_info) {
  TriggerOnWalletProperties(result, std::move(wallet_info));
}

void RewardsServiceImpl::OnGrant(ledger::Result result,
                                 const ledger::Grant& grant) {
  TriggerOnGrant(result, grant);
}

void RewardsServiceImpl::OnGrantCaptcha(const std::string& image, const std::string& hint) {
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
    ledger_->SetBalanceReportItem(GetPublisherMonth(now),
                                  GetPublisherYear(now),
                                  ledger::ReportType::GRANT,
                                  grant.probi);
  }

  GetCurrentBalanceReport();
  TriggerOnGrantFinish(result, grant);
}

void RewardsServiceImpl::OnReconcileComplete(ledger::Result result,
  const std::string& viewing_id,
  ledger::PUBLISHER_CATEGORY category,
  const std::string& probi) {
  if (result == ledger::Result::LEDGER_OK) {
    auto now = base::Time::Now();
    FetchWalletProperties();
    ledger_->OnReconcileCompleteSuccess(viewing_id,
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
  handler->OnLedgerStateLoaded(data.empty() ? ledger::Result::LEDGER_ERROR
                                            : ledger::Result::LEDGER_OK,
                               data);
  if (ledger_->GetRewardsMainEnabled()) {
    StartNotificationTimers();
  }
}

void RewardsServiceImpl::LoadPublisherState(
    ledger::LedgerCallbackHandler* handler) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadStateOnFileTaskRunner, publisher_state_path_),
      base::Bind(&RewardsServiceImpl::OnPublisherStateLoaded,
                     AsWeakPtr(),
                     base::Unretained(handler)));
}

void RewardsServiceImpl::OnPublisherStateLoaded(
    ledger::LedgerCallbackHandler* handler,
    const std::string& data) {
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
  handler->OnPublisherStateSaved(success ? ledger::Result::LEDGER_OK
                                         : ledger::Result::LEDGER_ERROR);
}

void RewardsServiceImpl::LoadNicewareList(
  ledger::GetNicewareListCallback callback) {
  std::string data = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
      IDR_BRAVE_REWARDS_NICEWARE_LIST).as_string();

  if (data.empty()) {
    LOG(ERROR) << "Failed to read in niceware list";
  }
  callback(data.empty() ? ledger::Result::LEDGER_ERROR
                                             : ledger::Result::LEDGER_OK,
                                data);
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
  callback(success ? ledger::Result::LEDGER_OK
                   : ledger::Result::LEDGER_ERROR, std::move(info));

  TriggerOnContentSiteUpdated();
}

void RewardsServiceImpl::LoadPublisherInfo(
    ledger::PublisherInfoFilter filter,
    ledger::PublisherInfoCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadPublisherInfoListOnFileTaskRunner,
          // set limit to 2 to make sure there is
          // only 1 valid result for the filter
          0, 2, filter, publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnPublisherInfoLoaded,
                     AsWeakPtr(),
                     callback));
}

void RewardsServiceImpl::OnPublisherInfoLoaded(
    ledger::PublisherInfoCallback callback,
    const ledger::PublisherInfoList list) {
  if (list.size() == 0) {
    callback(ledger::Result::NOT_FOUND,
        std::unique_ptr<ledger::PublisherInfo>());
    return;
  } else if (list.size() > 1) {
    callback(ledger::Result::TOO_MANY_RESULTS,
        std::unique_ptr<ledger::PublisherInfo>());
    return;
  }

  callback(ledger::Result::LEDGER_OK,
      std::make_unique<ledger::PublisherInfo>(list[0]));
}

void RewardsServiceImpl::LoadPublisherInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::PublisherInfoFilter filter,
    ledger::PublisherInfoListCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadPublisherInfoListOnFileTaskRunner,
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
    const ledger::URL_METHOD& method,
    ledger::LoadURLCallback callback) {
  net::URLFetcher::RequestType request_type = URLMethodToRequestType(method);

  net::URLFetcher* fetcher = net::URLFetcher::Create(
      GURL(url), request_type, this).release();
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
    for(auto const& header: headers) {
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
  scoped_refptr<net::HttpResponseHeaders> headersList = source->GetResponseHeaders();

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

  callback(response_code == 200, body, headers);
}

void RunIOTaskCallback(
    base::WeakPtr<RewardsServiceImpl> rewards_service,
    std::function<void(void)> callback) {
  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&RewardsServiceImpl::OnIOTaskComplete,
                      rewards_service,
                      callback));
}

void RewardsServiceImpl::OnIOTaskComplete(std::function<void(void)> callback) {
  callback();
}

void RewardsServiceImpl::RunIOTask(
    std::unique_ptr<ledger::LedgerTaskRunner> task) {
  ledger::LedgerTaskRunner::CallerThreadCallback callback =
      std::bind(&RunIOTaskCallback, AsWeakPtr(), _1);

  file_task_runner_->PostTask(FROM_HERE,
      base::BindOnce(&ledger::LedgerTaskRunner::Run,
          std::move(task), std::move(callback)));
}

void RewardsServiceImpl::TriggerOnWalletInitialized(int error_code) {
  for (auto& observer : observers_)
    observer.OnWalletInitialized(this, error_code);
}

void RewardsServiceImpl::TriggerOnWalletProperties(int error_code,
    std::unique_ptr<ledger::WalletInfo> wallet_info) {
  if (wallet_info && wallet_info->balance_ > 0)
    profile_->GetPrefs()->SetBoolean(kRewardsUserHasFunded, true);

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
    observer.OnWalletProperties(
            this, error_code, std::move(wallet_properties));
  }
}

void RewardsServiceImpl::FetchWalletProperties() {
  if (ready().is_signaled()) {
    ledger_->FetchWalletProperties();
  } else {
    ready().Post(FROM_HERE,
        base::Bind(&brave_rewards::RewardsService::FetchWalletProperties,
            base::Unretained(this)));
  }
}

void RewardsServiceImpl::FetchGrant(const std::string& lang,
    const std::string& payment_id) {
  ledger_->FetchGrant(lang, payment_id);
}

void RewardsServiceImpl::TriggerOnGrant(ledger::Result result,
                                        const ledger::Grant& grant) {
  brave_rewards::Grant properties;

  properties.promotionId = grant.promotionId;
  properties.altcurrency = grant.altcurrency;
  properties.probi = grant.probi;
  properties.expiryTime = grant.expiryTime;

  for (auto& observer : observers_)
    observer.OnGrant(this, result, properties);
}

void RewardsServiceImpl::GetGrantCaptcha() {
  ledger_->GetGrantCaptcha();
}

void RewardsServiceImpl::TriggerOnGrantCaptcha(const std::string& image, const std::string& hint) {
  for (auto& observer : observers_)
    observer.OnGrantCaptcha(this, image, hint);
}

std::string RewardsServiceImpl::GetWalletPassphrase() const {
  return ledger_->GetWalletPassphrase();
}

unsigned int RewardsServiceImpl::GetNumExcludedSites() const {
  return ledger_->GetNumExcludedSites();
}

void RewardsServiceImpl::RecoverWallet(const std::string passPhrase) const {
  return ledger_->RecoverWallet(passPhrase);
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

void RewardsServiceImpl::SolveGrantCaptcha(const std::string& solution) const {
  return ledger_->SolveGrantCaptcha(solution);
}

void RewardsServiceImpl::TriggerOnGrantFinish(ledger::Result result,
                                              const ledger::Grant& grant) {
  brave_rewards::Grant properties;

  properties.promotionId = grant.promotionId;
  properties.altcurrency = grant.altcurrency;
  properties.probi = grant.probi;
  properties.expiryTime = grant.expiryTime;

  for (auto& observer : observers_)
    observer.OnGrantFinish(this, result, properties);
}

uint64_t RewardsServiceImpl::GetReconcileStamp() const {
  return ledger_->GetReconcileStamp();
}

std::map<std::string, std::string> RewardsServiceImpl::GetAddresses() const {
  std::map<std::string, std::string> addresses;
  addresses.emplace("BAT", ledger_->GetBATAddress());
  addresses.emplace("BTC", ledger_->GetBTCAddress());
  addresses.emplace("ETH", ledger_->GetETHAddress());
  addresses.emplace("LTC", ledger_->GetLTCAddress());
  return addresses;
}

void RewardsServiceImpl::SetRewardsMainEnabled(bool enabled) const {
  return ledger_->SetRewardsMainEnabled(enabled);
}

uint64_t RewardsServiceImpl::GetPublisherMinVisitTime() const {
  return ledger_->GetPublisherMinVisitTime();
}

void RewardsServiceImpl::SetPublisherMinVisitTime(
    uint64_t duration_in_seconds) const {
  return ledger_->SetPublisherMinVisitTime(duration_in_seconds);
}

unsigned int RewardsServiceImpl::GetPublisherMinVisits() const {
  return ledger_->GetPublisherMinVisits();
}

void RewardsServiceImpl::SetPublisherMinVisits(unsigned int visits) const {
  return ledger_->SetPublisherMinVisits(visits);
}

bool RewardsServiceImpl::GetPublisherAllowNonVerified() const {
  return ledger_->GetPublisherAllowNonVerified();
}

void RewardsServiceImpl::SetPublisherAllowNonVerified(bool allow) const {
  return ledger_->SetPublisherAllowNonVerified(allow);
}

bool RewardsServiceImpl::GetPublisherAllowVideos() const {
  return ledger_->GetPublisherAllowVideos();
}

void RewardsServiceImpl::SetPublisherAllowVideos(bool allow) const {
  return ledger_->SetPublisherAllowVideos(allow);
}

void RewardsServiceImpl::SetContributionAmount(double amount) const {
  ledger_->SetUserChangedContribution();
  ledger_->SetContributionAmount(amount);
}

// TODO: remove me (and pure virtual definition)
// see https://github.com/brave/brave-core/commit/c4ef62c954a64fca18ae83ff8ffd611137323420#diff-aa3505dbf36b5d03d8ba0751e0c99904R385
// and https://github.com/brave-intl/bat-native-ledger/commit/27f3ceb471d61c84052737ff201fe18cb9a6af32#diff-e303122e010480b2226895b9470891a3R135
void RewardsServiceImpl::SetUserChangedContribution() const {
  ledger_->SetUserChangedContribution();
}

bool RewardsServiceImpl::GetAutoContribute() const {
  return ledger_->GetAutoContribute();
}

void RewardsServiceImpl::SetAutoContribute(bool enabled) const {
  return ledger_->SetAutoContribute(enabled);
}

void RewardsServiceImpl::TriggerOnContentSiteUpdated() {
  for (auto& observer : observers_)
    observer.OnContentSiteUpdated(this);
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
  handler->OnPublishersListSaved(success ? ledger::Result::LEDGER_OK
                                         : ledger::Result::LEDGER_ERROR);
}

void RewardsServiceImpl::SetTimer(uint64_t time_offset,
                                  uint32_t& timer_id) {
  if (next_timer_id_ == std::numeric_limits<uint32_t>::max())
    next_timer_id_ = 1;
  else
    ++next_timer_id_;

  timer_id = next_timer_id_;

  timers_[next_timer_id_] = std::make_unique<base::OneShotTimer>();
  timers_[next_timer_id_]->Start(FROM_HERE,
      base::TimeDelta::FromSeconds(time_offset),
      base::BindOnce(
          &RewardsServiceImpl::OnTimer, AsWeakPtr(), next_timer_id_));
}

void RewardsServiceImpl::OnTimer(uint32_t timer_id) {
  ledger_->OnTimer(timer_id);
  timers_.erase(timer_id);
}

void RewardsServiceImpl::LoadPublisherList(
    ledger::LedgerCallbackHandler* handler) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
                                   base::Bind(&LoadStateOnFileTaskRunner, publisher_list_path_),
                                   base::Bind(&RewardsServiceImpl::OnPublisherListLoaded,
                                              AsWeakPtr(),
                                              base::Unretained(handler)));
}

void RewardsServiceImpl::OnPublisherListLoaded(
    ledger::LedgerCallbackHandler* handler,
    const std::string& data) {
  handler->OnPublisherListLoaded(
      data.empty() ? ledger::Result::NO_PUBLISHER_LIST
                   : ledger::Result::LEDGER_OK,
      data);
}

std::map<std::string, brave_rewards::BalanceReport> RewardsServiceImpl::GetAllBalanceReports() {
  std::map<std::string, ledger::BalanceReportInfo> reports = ledger_->GetAllBalanceReports();

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

  return newReports;
}

void RewardsServiceImpl::GetCurrentBalanceReport() {
  ledger::BalanceReportInfo report;
  auto now = base::Time::Now();
  bool success = ledger_->GetBalanceReport(GetPublisherMonth(now),
                                           GetPublisherYear(now), &report);
  if (success) {
    TriggerOnGetCurrentBalanceReport(report);
  }
}

bool RewardsServiceImpl::IsWalletCreated() {
  return ledger_->IsWalletCreated();
}

void RewardsServiceImpl::GetPublisherActivityFromUrl(uint64_t windowId,
                                                     const std::string& url,
                                                     const std::string& favicon_url) {
  GURL parsedUrl(url);

  if (!parsedUrl.is_valid()) {
    return;
  }

  auto now = base::Time::Now();
  auto origin = parsedUrl.GetOrigin();
  std::string baseDomain =
      GetDomainAndRegistry(origin.host(), INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "") {
    std::unique_ptr<ledger::PublisherInfo> info;
    OnPublisherActivity(ledger::Result::NOT_FOUND, std::move(info), windowId);
    return;
  }

  ledger::VisitData visitData;
  visitData.domain = baseDomain;
  visitData.path = parsedUrl.PathForRequest();
  visitData.local_month = GetPublisherMonth(now);
  visitData.local_year = GetPublisherYear(now);
  visitData.name = baseDomain;
  visitData.url = origin.spec();
  visitData.favicon_url = favicon_url;

  ledger_->GetPublisherActivityFromUrl(windowId, visitData);
}

void RewardsServiceImpl::OnExcludedSitesChanged(const std::string& publisher_id) {
  for (auto& observer : observers_)
    observer.OnExcludedSitesChanged(this, publisher_id);
}

void RewardsServiceImpl::OnPublisherActivity(ledger::Result result,
                                             std::unique_ptr<ledger::PublisherInfo> info,
                                             uint64_t windowId) {
  if (result != ledger::Result::LEDGER_OK &&
      result != ledger::Result::NOT_FOUND) {
    return;
  }
  TriggerOnGetPublisherActivityFromUrl(result, std::move(info), windowId);
}

double RewardsServiceImpl::GetContributionAmount() {
  return ledger_->GetContributionAmount();
}

void RewardsServiceImpl::FetchFavIcon(const std::string& url,
                                      const std::string& favicon_key,
                                      ledger::FetchIconCallback callback) {
  GURL parsedUrl(url);

  if (!parsedUrl.is_valid()) {
    return;
  }

  std::vector<std::string>::iterator it;
  it = find (current_media_fetchers_.begin(), current_media_fetchers_.end(), url);
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
              base::Bind(&RewardsServiceImpl::OnFetchFavIconCompleted, base::Unretained(this), callback)),
          traffic_annotation));
  }
}

void RewardsServiceImpl::OnFetchFavIconCompleted(ledger::FetchIconCallback callback,
                                                 const std::string& favicon_key,
                                                 const GURL& url,
                                                 const BitmapFetcherService::RequestId& request_id,
                                                 const SkBitmap& image) {
  GURL favicon_url(favicon_key);
  gfx::Image gfx_image = gfx::Image::CreateFrom1xBitmap(image);
  favicon::FaviconService* favicon_service =
          FaviconServiceFactory::GetForProfile(profile_, ServiceAccessType::EXPLICIT_ACCESS);
  favicon_service->SetOnDemandFavicons(
      favicon_url,
      url,
      favicon_base::IconType::kFavicon,
      gfx_image,
      base::BindOnce(&RewardsServiceImpl::OnSetOnDemandFaviconComplete, AsWeakPtr(), favicon_url.spec(), callback));

  std::vector<std::string>::iterator it_url;
  it_url = find(current_media_fetchers_.begin(), current_media_fetchers_.end(), url.spec());
  if (it_url != current_media_fetchers_.end()) {
    current_media_fetchers_.erase(it_url);
  }

  std::vector<BitmapFetcherService::RequestId>::iterator it_ids;
  it_ids = find(request_ids_.begin(), request_ids_.end(), request_id);
  if (it_ids != request_ids_.end()) {
    request_ids_.erase(it_ids);
  }
}

void RewardsServiceImpl::OnSetOnDemandFaviconComplete(const std::string& favicon_url,
                                                      ledger::FetchIconCallback callback,
                                                      bool success) {
  callback(success, favicon_url);
}

void RewardsServiceImpl::GetPublisherBanner(const std::string& publisher_id) {
  ledger_->GetPublisherBanner(publisher_id,
      std::bind(&RewardsServiceImpl::OnPublisherBanner, this, _1));
}

void RewardsServiceImpl::OnPublisherBanner(std::unique_ptr<ledger::PublisherBanner> banner) {
  brave_rewards::PublisherBanner new_banner;

  if (!banner) {
    return;
  }

  new_banner.publisher_key = banner->publisher_key;
  new_banner.title = banner->title;
  new_banner.name = banner->name;
  new_banner.description = banner->description;
  new_banner.background = banner->background;
  new_banner.logo = banner->logo;
  new_banner.amounts = banner->amounts;
  new_banner.social = banner->social;
  new_banner.provider = banner->provider;
  new_banner.verified = banner->verified;

  for (auto& observer : observers_)
    observer.OnPublisherBanner(this, new_banner);
}

void RewardsServiceImpl::OnDonate_PublisherInfoSaved(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
}

void RewardsServiceImpl::OnDonate(const std::string& publisher_key, int amount,
  bool recurring, const ledger::PublisherInfo* publisher_info) {
  if (recurring) {
    // If caller provided publisher info, save it to `publisher_info` table
    if (publisher_info) {
      auto publisher_copy = std::make_unique<ledger::PublisherInfo>(
        *publisher_info);
      SavePublisherInfo(std::move(publisher_copy),
        std::bind(&RewardsServiceImpl::OnDonate_PublisherInfoSaved, this, _1, _2));
    }

    SaveRecurringDonation(publisher_key, amount);
    return;
  }

  ledger::PublisherInfo publisher(
    publisher_key,
    ledger::PUBLISHER_MONTH::ANY,
    -1);

  ledger_->DoDirectDonation(publisher, amount, "BAT");
}

bool SaveContributionInfoOnFileTaskRunner(const brave_rewards::ContributionInfo info,
  PublisherInfoDatabase* backend) {
  if (backend && backend->InsertContributionInfo(info))
    return true;

  return false;
}

void RewardsServiceImpl::OnContributionInfoSaved(const ledger::PUBLISHER_CATEGORY category, bool success) {
  if (success && category == ledger::PUBLISHER_CATEGORY::DIRECT_DONATION) {
    TipsUpdated();
  }
}

void RewardsServiceImpl::SaveContributionInfo(const std::string& probi,
  const int month,
  const int year,
  const uint32_t date,
  const std::string& publisher_key,
  const ledger::PUBLISHER_CATEGORY category) {

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

bool SaveRecurringDonationOnFileTaskRunner(const brave_rewards::RecurringDonation info,
  PublisherInfoDatabase* backend) {
  if (backend && backend->InsertOrUpdateRecurringDonation(info))
    return true;

  return false;
}

void RewardsServiceImpl::OnRecurringDonationSaved(bool success) {
  if (success) {
    UpdateRecurringDonationsList();
  }
}

void RewardsServiceImpl::SaveRecurringDonation(const std::string& publisher_key, const int amount) {
  brave_rewards::RecurringDonation info;
  info.publisher_key = publisher_key;
  info.amount = amount;
  info.added_date = GetCurrentTimestamp();

  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&SaveRecurringDonationOnFileTaskRunner,
                    info,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnRecurringDonationSaved,
                     AsWeakPtr()));

}

ledger::PublisherInfoList GetRecurringDonationsOnFileTaskRunner(PublisherInfoDatabase* backend) {
  ledger::PublisherInfoList list;
  if (!backend) {
    return list;
  }

  backend->GetRecurringDonations(&list);

  return list;
}

void RewardsServiceImpl::OnRecurringDonationsData(const ledger::PublisherInfoListCallback callback,
                                                  const ledger::PublisherInfoList list) {
  callback(list, 0);
}

void RewardsServiceImpl::GetRecurringDonations(ledger::PublisherInfoListCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&GetRecurringDonationsOnFileTaskRunner,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnRecurringDonationsData,
                     AsWeakPtr(),
                     callback));

}

void RewardsServiceImpl::UpdateRecurringDonationsList() {
  GetRecurringDonations(std::bind(&RewardsServiceImpl::OnRecurringDonationUpdated, this, _1));
}

void RewardsServiceImpl::UpdateTipsList() {
  TipsUpdated();
}

void RewardsServiceImpl::OnRecurringDonationUpdated(const ledger::PublisherInfoList& list) {
  brave_rewards::ContentSiteList new_list;

  for (auto &publisher : list) {
    brave_rewards::ContentSite site = PublisherInfoToContentSite(publisher);
    site.percentage = publisher.weight;
    new_list.push_back(site);
  }

  for (auto& observer : observers_) {
    observer.OnRecurringDonationUpdated(this, new_list);
  }
}

ledger::PublisherInfoList TipsUpdatedOnFileTaskRunner(PublisherInfoDatabase* backend) {
  ledger::PublisherInfoList list;
  if (!backend) {
    return list;
  }

  auto now = base::Time::Now();
  backend->GetTips(&list, GetPublisherMonth(now), GetPublisherYear(now));

  return list;
}

void RewardsServiceImpl::OnTipsUpdatedData(const ledger::PublisherInfoList list) {
  brave_rewards::ContentSiteList new_list;

  for (auto &publisher : list) {
    brave_rewards::ContentSite site = PublisherInfoToContentSite(publisher);
    site.percentage = publisher.weight;
    new_list.push_back(site);
  }

  for (auto& observer : observers_) {
    observer.OnCurrentTips(this, new_list);
  }
}

void RewardsServiceImpl::RemoveRecurring(const std::string& publisher_key) {
  ledger_->RemoveRecurring(publisher_key);
}

void RewardsServiceImpl::TipsUpdated() {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&TipsUpdatedOnFileTaskRunner,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnTipsUpdatedData,
                     AsWeakPtr()));

}

bool RemoveRecurringOnFileTaskRunner(const std::string publisher_key, PublisherInfoDatabase* backend) {
  if (!backend) {
    return false;
  }

  return backend->RemoveRecurring(publisher_key);
}

void RewardsServiceImpl::OnRemovedRecurring(ledger::RecurringRemoveCallback callback, bool success) {
  callback(success ? ledger::Result::LEDGER_OK : ledger::Result::LEDGER_ERROR);
  UpdateRecurringDonationsList();
}

void RewardsServiceImpl::OnRemoveRecurring(const std::string& publisher_key,
                                           ledger::RecurringRemoveCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&RemoveRecurringOnFileTaskRunner,
                    publisher_key,
                    publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnRemovedRecurring,
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

void RewardsServiceImpl::TriggerOnGetPublisherActivityFromUrl(
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId) {
  for (auto& observer : private_observers_)
    observer.OnGetPublisherActivityFromUrl(this, result, std::move(info),
                                           windowId);
}

void RewardsServiceImpl::SetContributionAutoInclude(std::string publisher_key,
  bool excluded, uint64_t windowId) {
  ledger_->SetPublisherPanelExclude(publisher_key, excluded ?
    ledger::PUBLISHER_EXCLUDE::EXCLUDED : ledger::PUBLISHER_EXCLUDE::INCLUDED,
    windowId);
}

RewardsNotificationService* RewardsServiceImpl::GetNotificationService() const {
  return notification_service_.get();
}

void RewardsServiceImpl::StartNotificationTimers() {
  // Startup timer, begins after 3-second delay.
  notification_startup_timer_ = std::make_unique<base::OneShotTimer>();
  notification_startup_timer_->Start(
      FROM_HERE, base::TimeDelta::FromSeconds(3), this,
      &RewardsServiceImpl::OnNotificationTimerFired);
  DCHECK(notification_startup_timer_->IsRunning());

  // Periodic timer, runs once per day by default.
  PrefService* pref_service = profile_->GetPrefs();
  base::TimeDelta periodic_timer_interval =
      pref_service->GetTimeDelta(kRewardsNotificationTimerInterval);
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
  MaybeShowBackupNotification();
  MaybeShowAddFundsNotification();
}

bool RewardsServiceImpl::HasSufficientBalanceToReconcile() const {
  return (ledger_->GetBalance() >= ledger_->GetContributionAmount());
}

bool RewardsServiceImpl::ShouldShowNotificationAddFunds() const {
  base::Time next_time =
      profile_->GetPrefs()->GetTime(kRewardsAddFundsNotification);
  return (next_time.is_null() || base::Time::Now() > next_time);
}

void RewardsServiceImpl::ShowNotificationAddFunds() {
  base::Time next_time = base::Time::Now() + base::TimeDelta::FromDays(3);
  profile_->GetPrefs()->SetTime(kRewardsAddFundsNotification, next_time);
  RewardsNotificationService::RewardsNotificationArgs args;
  notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS, args,
      "rewards_notification_insufficient_funds");
}

std::unique_ptr<ledger::LogStream> RewardsServiceImpl::Log(
    const char* file,
    int line,
    const ledger::LogLevel log_level) const {
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
      std::string lower = base::ToLowerASCII(value);
      if (lower == "true" || lower == "1") {
        ledger::is_production = false;
      } else {
        ledger::is_production = true;
      }
      continue;
    }

    if (name == "reconcile-interval") {
      int reconcile_int;
      bool success = base::StringToInt(value, &reconcile_int);
      if (success && reconcile_int > 0) {
        ledger::reconcile_time = reconcile_int;
      }
      continue;
    }

    if (name == "short-retries") {
      std::string lower = base::ToLowerASCII(value);
      if (lower == "true" || lower == "1") {
        ledger::short_retries = true;
      } else {
        ledger::short_retries = false;
      }
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
  profile_->GetPrefs()->SetBoolean(kRewardsBackupSucceeded, true);
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
  info.month = ledger::PUBLISHER_MONTH::ANY;
  info.year = -1;
  info.verified = site->verified;
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.name = site->name;
  info.url = site->url;
  info.provider = site->provider;
  info.favicon_url = site->favicon_url;

  OnDonate(publisher_key, amount, recurring, &info);
}

void RewardsServiceImpl::SetLedgerClient(
    std::unique_ptr<ledger::Ledger> new_ledger) {
  ledger_ = std::move(new_ledger);
}

}  // namespace brave_rewards
