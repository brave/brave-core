/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service_impl.h"

#include <functional>
#include <limits.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/guid.h"
#include "base/i18n/time_formatting.h"
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
#include "brave/common/brave_switches.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/publisher_info_database.h"
#include "brave/components/brave_rewards/browser/rewards_fetcher_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/browser_task_traits.h"
#include "content_site.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "net/base/escape.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "publisher_banner.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"
#include "url/url_canon_stdstring.h"

using extensions::Event;
using extensions::EventRouter;

using namespace net::registry_controlled_domains;
using namespace std::placeholders;

namespace brave_rewards {

namespace {

class LedgerURLLoaderImpl : public ledger::LedgerURLLoader {
 public:
  LedgerURLLoaderImpl(uint64_t request_id, net::URLFetcher* fetcher) :
    request_id_(request_id),
    fetcher_(fetcher) {}
  ~LedgerURLLoaderImpl() override = default;

  void Start() override {
    fetcher_->Start();
  }

  uint64_t request_id() override {
    return request_id_;
  }

 private:
  uint64_t request_id_;
  net::URLFetcher* fetcher_;  // NOT OWNED
};

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
    const GetContentSiteListCallback& callback,
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

static uint64_t next_id = 1;

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
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ledger_state_path_(profile_->GetPath().Append(kLedger_state)),
      publisher_state_path_(profile_->GetPath().Append(kPublisher_state)),
      publisher_info_db_path_(profile->GetPath().Append(kPublisher_info_db)),
      publisher_list_path_(profile->GetPath().Append(kPublishers_list)),
      publisher_info_backend_(
          new PublisherInfoDatabase(publisher_info_db_path_)),
      next_timer_id_(0) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  // Environment
  #if defined(OFFICIAL_BUILD)
    ledger::is_production = true;
  #else
    ledger::is_production = false;
  #endif

  if (command_line.HasSwitch(switches::kRewardsEnv)) {
    std::string defined_env = command_line.GetSwitchValueASCII(switches::kRewardsEnv);
    std::string defined_env_lower = base::ToLowerASCII(defined_env);

    if (defined_env_lower == "stag") {
      ledger::is_production = false;
    } else if (defined_env_lower == "prod") {
      ledger::is_production = true;
    }
  }

  // Reconcile interval
  if (command_line.HasSwitch(switches::kRewardsReconcileInterval)) {
    std::string defined_reconcile = command_line.GetSwitchValueASCII(switches::kRewardsReconcileInterval);
    int defined_reconcile_int;
    bool success = base::StringToInt(defined_reconcile, &defined_reconcile_int);
    if (success && defined_reconcile_int > 0) {
      ledger::reconcile_time = defined_reconcile_int;
    }
  }
}

RewardsServiceImpl::~RewardsServiceImpl() {
  file_task_runner_->DeleteSoon(FROM_HERE, publisher_info_backend_.release());
}

RewardsNotificationService* RewardsServiceImpl::notification_service() {
  return static_cast<RewardsService*>(this)->notification_service(profile_);
}

void RewardsServiceImpl::Init() {
  ledger_->Initialize();
  notification_service()->ReadRewardsNotifications();
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

void RewardsServiceImpl::GetContentSiteList(
    uint32_t start, uint32_t limit,
    const GetContentSiteListCallback& callback) {
  auto now = base::Time::Now();
  ledger::PublisherInfoFilter filter;
  filter.category = ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE;
  filter.month = GetPublisherMonth(now);
  filter.year = GetPublisherYear(now);
  filter.min_duration = ledger_->GetPublisherMinVisitTime();
  filter.order_by.push_back(std::pair<std::string, bool>("ai.percent", false));
  filter.reconcile_stamp = ledger_->GetReconcileStamp();
  filter.excluded =
    ledger::PUBLISHER_EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED;

  ledger_->GetPublisherInfoList(start, limit,
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

  auto now = base::Time::Now();
  ledger::VisitData data(baseDomain,
                         origin.host(),
                         url.path(),
                         tab_id.id(),
                         GetPublisherMonth(now),
                         GetPublisherYear(now),
                         baseDomain,
                         origin.spec(),
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
  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    for (auto request_id : request_ids_) {
      image_service->CancelRequest(request_id);
    }
  }

  fetchers_.clear();
  ledger_.reset();
  notification_service()->StoreRewardsNotifications();
  RewardsService::Shutdown();
}

void RewardsServiceImpl::OnWalletInitialized(ledger::Result result) {
  if (!ready_.is_signaled())
    ready_.Signal();

  if (result == ledger::Result::WALLET_CREATED) {
    SetRewardsMainEnabled(true);
    SetAutoContribute(true);
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

  if (result == ledger::Result::LEDGER_OK) {
    RewardsNotificationService::RewardsNotificationArgs args;
    notification_service()->AddNotification(
        RewardsNotificationService::REWARDS_NOTIFICATION_GRANT, args,
        "rewards_notification_grant");
  }
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

  TriggerOnGrantFinish(result, grant);
  notification_service()->DeleteNotification("rewards_notification_grant");
}

void RewardsServiceImpl::OnReconcileComplete(ledger::Result result,
  const std::string& viewing_id,
  ledger::PUBLISHER_CATEGORY category,
  const std::string& probi) {
  if (result == ledger::Result::LEDGER_OK) {
    // TODO add notification service when implemented
    auto now = base::Time::Now();
    FetchWalletProperties();
    ledger_->OnReconcileCompleteSuccess(viewing_id,
        category,
        probi,
        GetPublisherMonth(now),
        GetPublisherYear(now),
        GetCurrentTimestamp());
  }

  for (auto& observer : observers_)
    observer.OnReconcileComplete(this, result, viewing_id, probi);
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
    ledger::GetPublisherInfoListCallback callback) {
  auto now = base::Time::Now();
  filter.month = GetPublisherMonth(now);
  filter.year = GetPublisherYear(now);
  filter.reconcile_stamp = ledger_->GetReconcileStamp();

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

void RewardsServiceImpl::LoadCurrentPublisherInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::PublisherInfoFilter filter,
    ledger::GetPublisherInfoListCallback callback) {
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
    ledger::GetPublisherInfoListCallback callback,
    const ledger::PublisherInfoList& list) {
  uint32_t next_record = 0;
  if (list.size() == limit)
    next_record = start + limit + 1;

  callback(std::cref(list), next_record);
}

std::unique_ptr<ledger::LedgerURLLoader> RewardsServiceImpl::LoadURL(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    const ledger::URL_METHOD& method,
    ledger::LedgerCallbackHandler* handler) {
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
    VLOG(ledger::LogLevel::LOG_REQUEST) << "[ REQUEST ]";
    VLOG(ledger::LogLevel::LOG_REQUEST) << "> url: " << url;
    VLOG(ledger::LogLevel::LOG_REQUEST) << "> method: " << printMethod;
    VLOG(ledger::LogLevel::LOG_REQUEST) << "> content: " << content;
    VLOG(ledger::LogLevel::LOG_REQUEST) << "> contentType: " << contentType;
    for (size_t i = 0; i < headers.size(); i++) {
      VLOG(ledger::LogLevel::LOG_REQUEST) << "> headers: " << headers[i];
    }
    VLOG(ledger::LogLevel::LOG_REQUEST) << "[ END REQUEST ]";
  }

  FetchCallback callback = base::Bind(
      &ledger::LedgerCallbackHandler::OnURLRequestResponse,
      base::Unretained(handler),
      next_id,
      url);
  fetchers_[fetcher] = callback;

  std::unique_ptr<ledger::LedgerURLLoader> loader(
      new LedgerURLLoaderImpl(next_id++, fetcher));

  return loader;
}

void RewardsServiceImpl::OnURLFetchComplete(
    const net::URLFetcher* source) {
  if (fetchers_.find(source) == fetchers_.end())
    return;

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

  callback.Run(response_code, body, headers);
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
  // webui
  for (auto& observer : observers_)
    observer.OnWalletInitialized(this, error_code);

  // extension
  EventRouter* event_router = EventRouter::Get(profile_);
  if (event_router) {
    std::unique_ptr<base::ListValue> args(new base::ListValue());
    std::unique_ptr<Event> event(
        new Event(extensions::events::BRAVE_WALLET_CREATED,
          extensions::api::brave_rewards::OnWalletCreated::kEventName,
          std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void RewardsServiceImpl::TriggerOnWalletProperties(int error_code,
    std::unique_ptr<ledger::WalletInfo> wallet_info) {
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

      for (size_t i = 0; i < wallet_info->grants_.size(); i ++) {
        brave_rewards::Grant grant;

        grant.altcurrency = wallet_info->grants_[i].altcurrency;
        grant.probi = wallet_info->grants_[i].probi;
        grant.expiryTime = wallet_info->grants_[i].expiryTime;

        wallet_properties->grants.push_back(grant);
      }
    }

    // webui
    observer.OnWalletProperties(this, error_code, std::move(wallet_properties));
  }
  // extension
  EventRouter* event_router = EventRouter::Get(profile_);
  if (event_router && wallet_info) {
    extensions::api::brave_rewards::OnWalletProperties::Properties properties;

    properties.probi = wallet_info->probi_;
    properties.balance = wallet_info->balance_;
    properties.rates.btc = wallet_info->rates_["BTC"];
    properties.rates.eth = wallet_info->rates_["ETH"];
    properties.rates.usd = wallet_info->rates_["USD"];
    properties.rates.eur = wallet_info->rates_["EUR"];

    for (size_t i = 0; i < wallet_info->grants_.size(); i ++) {
      properties.grants.push_back(extensions::api::brave_rewards::OnWalletProperties::Properties::GrantsType());
      auto& grant = properties.grants[properties.grants.size() -1];

      grant.altcurrency = wallet_info->grants_[i].altcurrency;
      grant.probi = wallet_info->grants_[i].probi;
      grant.expiry_time = wallet_info->grants_[i].expiryTime;
    }

    std::unique_ptr<base::ListValue> args(
        extensions::api::brave_rewards::OnWalletProperties::Create(properties)
            .release());

    std::unique_ptr<Event> event(
        new Event(extensions::events::BRAVE_ON_WALLET_PROPERTIES,
                  extensions::api::brave_rewards::OnWalletProperties::kEventName,
                  std::move(args)));
    event_router->BroadcastEvent(std::move(event));
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

void RewardsServiceImpl::SetPublisherMinVisitTime(
    uint64_t duration_in_seconds) const {
  return ledger_->SetPublisherMinVisitTime(duration_in_seconds);
}

void RewardsServiceImpl::SetPublisherMinVisits(unsigned int visits) const {
  return ledger_->SetPublisherMinVisits(visits);
}

void RewardsServiceImpl::SetPublisherAllowNonVerified(bool allow) const {
  return ledger_->SetPublisherAllowNonVerified(allow);
}

void RewardsServiceImpl::SetPublisherAllowVideos(bool allow) const {
  return ledger_->SetPublisherAllowVideos(allow);
}

void RewardsServiceImpl::SetContributionAmount(double amount) const {
  ledger_->SetContributionAmount(amount);
}

void RewardsServiceImpl::SetUserChangedContribution() const {
  ledger_->SetUserChangedContribution();
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
  bool success = ledger_->GetBalanceReport(
      GetPublisherMonth(now),
      GetPublisherYear(now),
      &report);

  if (success) {
    EventRouter* event_router = EventRouter::Get(profile_);
    if (event_router) {
      extensions::api::brave_rewards::OnCurrentReport::Properties properties;

      properties.ads = report.earning_from_ads_;
      properties.closing = report.closing_balance_;
      properties.contribute = report.auto_contribute_;
      properties.deposit = report.deposits_;
      properties.grant = report.grants_;
      properties.tips = report.one_time_donation_;
      properties.opening = report.opening_balance_;
      properties.total = report.total_;
      properties.recurring = report.recurring_donation_;

      std::unique_ptr<base::ListValue> args(
          extensions::api::brave_rewards::OnCurrentReport::Create(properties)
              .release());
      std::unique_ptr<Event> event(
          new Event(extensions::events::BRAVE_ON_CURRENT_REPORT,
                    extensions::api::brave_rewards::OnCurrentReport::kEventName,
                    std::move(args)));
      event_router->BroadcastEvent(std::move(event));
    }
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

void RewardsServiceImpl::OnExcludedSitesChanged() {
  for (auto& observer : observers_)
    observer.OnExcludedSitesChanged(this);
}

void RewardsServiceImpl::OnPublisherActivity(ledger::Result result,
                                             std::unique_ptr<ledger::PublisherInfo> info,
                                             uint64_t windowId) {
  if (result == ledger::Result::LEDGER_OK || result == ledger::Result::NOT_FOUND) {
    EventRouter* event_router = EventRouter::Get(profile_);
    if (!event_router) {
      return;
    }

    extensions::api::brave_rewards::OnPublisherData::Publisher publisher;

    if (!info.get()) {
      info.reset(new ledger::PublisherInfo());
      info->id = "";
    }

    publisher.percentage = info->percent;
    publisher.verified = info->verified;
    publisher.excluded = info->excluded == ledger::PUBLISHER_EXCLUDE::EXCLUDED;
    publisher.name = info->name;
    publisher.url = info->url;
    publisher.provider = info->provider;
    publisher.favicon_url = info->favicon_url;
    publisher.publisher_key = info->id;
    std::unique_ptr<base::ListValue> args(
        extensions::api::brave_rewards::OnPublisherData::Create(windowId, publisher)
          .release());

    std::unique_ptr<Event> event(
        new Event(extensions::events::BRAVE_ON_PUBLISHER_DATA,
          extensions::api::brave_rewards::OnPublisherData::kEventName,
          std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
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

  for (auto& observer : observers_)
    observer.OnPublisherBanner(this, new_banner);
}

void RewardsServiceImpl::OnDonate(const std::string& publisher_key, int amount, bool recurring) {
  if (recurring) {
    SaveRecurringDonation(publisher_key, amount);
    return;
  }

  ledger::PublisherInfo publisher_info(
    publisher_key,
    ledger::PUBLISHER_MONTH::ANY,
    -1);

  ledger_->DoDirectDonation(publisher_info, amount, "BAT");
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

void RewardsServiceImpl::OnRecurringDonationsData(const ledger::RecurringDonationCallback callback,
                                                  const ledger::PublisherInfoList list) {
  callback(list);
}

void RewardsServiceImpl::GetRecurringDonations(ledger::RecurringDonationCallback callback) {
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

void RewardsServiceImpl::SetContributionAutoInclude(std::string publisher_key,
  bool excluded, uint64_t windowId) {
  ledger_->SetPublisherPanelExclude(publisher_key, excluded ?
    ledger::PUBLISHER_EXCLUDE::EXCLUDED : ledger::PUBLISHER_EXCLUDE::INCLUDED,
    windowId);
}

void RewardsServiceImpl::Log(ledger::LogLevel level, const std::string& text) {
  if (level == ledger::LogLevel::LOG_ERROR) {
    LOG(ERROR) << text;
    return;
  }

  if (level == ledger::LogLevel::LOG_WARNING) {
    LOG(WARNING) << text;
    return;
  }

  if (level == ledger::LogLevel::LOG_INFO) {
    LOG(INFO) << text;
    return;
  }

  VLOG(level) << text;
}

}  // namespace brave_rewards
