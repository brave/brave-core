/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/browser/balance_report.h"

#include <functional>
#include <limits.h>

#include "base/bind.h"
#include "base/guid.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/i18n/time_formatting.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/media_publisher_info.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/wallet_info.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/publisher_info_database.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "net/base/escape.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "url/gurl.h"
#include "url/url_canon_stdstring.h"

#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "brave/common/extensions/api/brave_rewards.h"
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

RewardsServiceImpl::RewardsServiceImpl(Profile* profile) :
    profile_(profile),
    ledger_(ledger::Ledger::CreateInstance(this)),
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    ledger_state_path_(profile_->GetPath().Append("ledger_state")),
    publisher_state_path_(profile_->GetPath().Append("publisher_state")),
    publisher_info_db_path_(profile->GetPath().Append("publisher_info_db")),
    publisher_list_path_(profile->GetPath().Append("publishers_list")),
    publisher_info_backend_(new PublisherInfoDatabase(publisher_info_db_path_)),
    next_timer_id_(0) {
// TODO(bridiver) - production/verbose should
// also be controllable by command line flags
#if defined(IS_OFFICIAL_BUILD)
ledger::is_production = true;
#else
ledger::is_production = false;
#endif

#if defined(NDEBUG)
ledger::is_verbose = false;
#else
ledger::is_verbose = true;
#endif
}

RewardsServiceImpl::~RewardsServiceImpl() {
  file_task_runner_->DeleteSoon(FROM_HERE, publisher_info_backend_.release());
}

void RewardsServiceImpl::Init() {
  ledger_->Initialize();
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
  filter.order_by.push_back(std::pair<std::string, bool>("percent", false));

  ledger_->GetPublisherInfoList(start, limit,
      filter,
      std::bind(&GetContentSiteListInternal,
                start,
                limit,
                callback, _1, _2));
}

void RewardsServiceImpl::OnLoad(SessionID tab_id, const GURL& url) {
  auto origin = url.GetOrigin();
  const std::string tld =
      GetDomainAndRegistry(origin.host(), INCLUDE_PRIVATE_REGISTRIES);

  if (tld == "")
    return;

  auto now = base::Time::Now();
  ledger::VisitData data(tld,
                         origin.host(),
                         url.path(),
                         tab_id.id(),
                         GetPublisherMonth(now),
                         GetPublisherYear(now),
                         tld,
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
    VLOG(1) << "Error in OnMediaPublisherInfoSaved";
  }
}

std::string RewardsServiceImpl::URIEncode(const std::string& value) {
  return net::EscapeQueryParamValue(value, false);
}

std::string RewardsServiceImpl::GenerateGUID() const {
  return base::GenerateGUID();
}

void RewardsServiceImpl::Shutdown() {
  fetchers_.clear();
  ledger_.reset();
  RewardsService::Shutdown();
}

void RewardsServiceImpl::OnWalletInitialized(ledger::Result result) {
  if (!ready_.is_signaled())
    ready_.Signal();
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
  ledger_->GetBalanceReport(GetPublisherMonth(now), GetPublisherYear(now), &report_info);
  report_info.grants_ += 10.0; // TODO NZ convert probi to
  ledger_->SetBalanceReport(GetPublisherMonth(now), GetPublisherYear(now), report_info);
  TriggerOnGrantFinish(result, grant);
}

void RewardsServiceImpl::OnReconcileComplete(ledger::Result result,
                                              const std::string& viewing_id) {
  LOG(ERROR) << "reconcile complete " << viewing_id;
  // TODO - TriggerOnReconcileComplete
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

void RewardsServiceImpl::SavePublisherInfo(
    std::unique_ptr<ledger::PublisherInfo> publisher_info,
    ledger::PublisherInfoCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&SavePublisherInfoOnFileTaskRunner,
                    *publisher_info,
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

  if (VLOG_IS_ON(2)) {
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
    VLOG(2) << "[ REQUEST ]";
    VLOG(2) << "> url: " << url;
    VLOG(2) << "> method: " << printMethod;
    VLOG(2) << "> content: " << content;
    VLOG(2) << "> contentType: " << contentType;
    for (size_t i = 0; i < headers.size(); i++) {
      VLOG(2) << "> headers: " << headers[i];
    }
    VLOG(2) << "[ END REQUEST ]";
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

void RewardsServiceImpl::RunIOTask(
    std::unique_ptr<ledger::LedgerTaskRunner> task) {
  file_task_runner_->PostTask(FROM_HERE,
      base::BindOnce(&ledger::LedgerTaskRunner::Run, std::move(task)));
}

void RewardsServiceImpl::RunTask(
      std::unique_ptr<ledger::LedgerTaskRunner> task) {
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&ledger::LedgerTaskRunner::Run,
                     std::move(task)));
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

  for (auto& observer : observers_)
    observer.OnWalletProperties(this, error_code, std::move(wallet_properties));
}

void RewardsServiceImpl::GetWalletProperties() {
  if (ready().is_signaled()) {
    ledger_->GetWalletProperties();
  } else {
    ready().Post(FROM_HERE,
        base::Bind(&brave_rewards::RewardsService::GetWalletProperties,
            base::Unretained(this)));
  }
}

void RewardsServiceImpl::GetGrant(const std::string& lang,
    const std::string& payment_id) {
  ledger_->GetGrant(lang, payment_id);
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
  return ledger_->SetContributionAmount(amount);
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

bool RewardsServiceImpl::IsWalletCreated() {
  return ledger_->IsWalletCreated();
}

}  // namespace brave_rewards
