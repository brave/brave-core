/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"

#include <functional>

#include "base/bind.h"
#include "base/guid.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/wallet_info.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/publisher_info_backend.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_fetcher.h"
#include "url/gurl.h"

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

ContentSite PublisherInfoToContentSite(
    const ledger::PublisherInfo& publisher_info) {
  ContentSite content_site(publisher_info.id);
  content_site.score = publisher_info.score;
  content_site.pinned = publisher_info.pinned;
  content_site.percentage = publisher_info.percent;
  content_site.excluded = publisher_info.excluded;
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

bool SavePublisherInfoOnFileTaskRunner(
    const ledger::PublisherInfo publisher_info,
    PublisherInfoBackend* backend) {
  if (backend && backend->Put(publisher_info.id, publisher_info.ToJSON()))
    return true;

  return false;
}

ledger::PublisherInfoList LoadPublisherInfoListOnFileTaskRunner(
    uint32_t start,
    uint32_t limit,
    ledger::PublisherInfoFilter filter,
    PublisherInfoBackend* backend) {
  ledger::PublisherInfoList list;

  std::vector<const std::string> results;
  if (backend && backend->Load(start, limit, results)) {
    for (std::vector<const std::string>::const_iterator it =
        results.begin(); it != results.end(); ++it) {
      list.push_back(ledger::PublisherInfo::FromJSON(*it));
    }
  }

  return list;
}

std::unique_ptr<ledger::PublisherInfo> LoadPublisherInfoOnFileTaskRunner(
    const std::string& id,
    PublisherInfoBackend* backend) {
  std::unique_ptr<ledger::PublisherInfo> info;

  std::string json;
  if (backend && backend->Get(id, &json)) {
    info.reset(
        new ledger::PublisherInfo(ledger::PublisherInfo::FromJSON(json)));
  }

  return info;
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

static uint64_t next_id = 1;

}  // namespace

RewardsServiceImpl::RewardsServiceImpl(Profile* profile) :
    profile_(profile),
    ledger_(ledger::Ledger::CreateInstance(this)),
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    ledger_state_path_(profile_->GetPath().Append("ledger_state")),
    publisher_state_path_(profile_->GetPath().Append("publisher_state")),
    publisher_info_db_path_(profile->GetPath().Append("publisher_info")),
    publisher_info_backend_(new PublisherInfoBackend(publisher_info_db_path_)) {
}

RewardsServiceImpl::~RewardsServiceImpl() {
  file_task_runner_->DeleteSoon(FROM_HERE, publisher_info_backend_.release());
}

void RewardsServiceImpl::CreateWallet() {
  ledger_->CreateWallet();
}

void RewardsServiceImpl::GetContentSiteList(
    uint32_t start, uint32_t limit,
    const GetContentSiteListCallback& callback) {
  ledger_->GetPublisherInfoList(start, limit,
      ledger::PublisherInfoFilter::DEFAULT,
      std::bind(&GetContentSiteListInternal,
                start,
                limit,
                std::cref(callback), _1, _2));
}

void RewardsServiceImpl::OnLoad(SessionID tab_id, const GURL& url) {
  auto origin = url.GetOrigin();
  const std::string tld =
      GetDomainAndRegistry(origin.host(), INCLUDE_PRIVATE_REGISTRIES);

  if (tld == "")
    return;

  // TODO(bridiver) - add query parts
  ledger::VisitData data(tld, origin.host(), url.path(), tab_id.id());
  ledger_->OnLoad(data);
}

void RewardsServiceImpl::OnUnload(SessionID tab_id) {
  ledger_->OnUnload(tab_id.id());
}

void RewardsServiceImpl::OnShow(SessionID tab_id) {
  ledger_->OnShow(tab_id.id());
}

void RewardsServiceImpl::OnHide(SessionID tab_id) {
  ledger_->OnHide(tab_id.id());
}

void RewardsServiceImpl::OnForeground(SessionID tab_id) {
  ledger_->OnForeground(tab_id.id());
}

void RewardsServiceImpl::OnBackground(SessionID tab_id) {
  ledger_->OnBackground(tab_id.id());
}

void RewardsServiceImpl::OnMediaStart(SessionID tab_id) {
  ledger_->OnMediaStart(tab_id.id());
}

void RewardsServiceImpl::OnMediaStop(SessionID tab_id) {
  ledger_->OnMediaStop(tab_id.id());
}

void RewardsServiceImpl::OnXHRLoad(SessionID tab_id, const GURL& url) {
  // TODO(bridiver) - add query parts
  ledger_->OnXHRLoad(tab_id.id(), url.spec());
}

std::string RewardsServiceImpl::GenerateGUID() const {
  return base::GenerateGUID();
}

void RewardsServiceImpl::Shutdown() {
  fetchers_.clear();
  ledger_.reset();
  RewardsService::Shutdown();
}

void RewardsServiceImpl::OnWalletCreated(ledger::Result result) {
  TriggerOnWalletCreated(result);
}

void RewardsServiceImpl::OnWalletProperties(ledger::Result result,
    std::unique_ptr<ledger::WalletInfo> wallet_info) {
  TriggerOnWalletProperties(result, std::move(wallet_info));
}

void RewardsServiceImpl::OnPromotion(ledger::Promo result) {
  TriggerOnPromotion(result);
}

void RewardsServiceImpl::OnPromotionCaptcha(const std::string& image) {
  TriggerOnPromotionCaptcha(image);
}

void RewardsServiceImpl::OnRecoverWallet(ledger::Result result, double balance) {
  TriggerOnRecoverWallet(result, balance);
}

void RewardsServiceImpl::OnPromotionFinish(ledger::Result result, unsigned int statusCode, uint64_t expirationDate) {
  TriggerOnPromotionFinish(result, statusCode, expirationDate);
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
  handler->OnLedgerStateLoaded(data.empty() ? ledger::Result::ERROR
                                            : ledger::Result::OK,
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
  handler->OnPublisherStateLoaded(data.empty() ? ledger::Result::ERROR
                                               : ledger::Result::OK,
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
  handler->OnLedgerStateSaved(success ? ledger::Result::OK
                                      : ledger::Result::ERROR);
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
  handler->OnPublisherStateSaved(success ? ledger::Result::OK
                                         : ledger::Result::ERROR);
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
  callback(success ? ledger::Result::OK
                   : ledger::Result::ERROR, std::move(info));
}

void RewardsServiceImpl::LoadPublisherInfo(
    const ledger::PublisherInfo::id_type& publisher_id,
    ledger::PublisherInfoCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadPublisherInfoOnFileTaskRunner,
          publisher_id, publisher_info_backend_.get()),
      base::Bind(&RewardsServiceImpl::OnPublisherInfoLoaded,
                     AsWeakPtr(),
                     callback));
}

void RewardsServiceImpl::OnPublisherInfoLoaded(
    ledger::PublisherInfoCallback callback,
    std::unique_ptr<ledger::PublisherInfo> info) {
  callback(ledger::Result::OK, std::move(info));
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

  FetchCallback callback = base::Bind(
      &ledger::LedgerCallbackHandler::OnURLRequestResponse,
      base::Unretained(handler),
      next_id);
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
  if (response_code != net::URLFetcher::ResponseCode::RESPONSE_CODE_INVALID &&
      source->GetStatus().is_success()) {
    source->GetResponseAsString(&body);
  }

  callback.Run(response_code, body);
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

void RewardsServiceImpl::TriggerOnWalletCreated(int error_code) {
  for (auto& observer : observers_)
    observer.OnWalletCreated(this, error_code);
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
  ledger_->GetWalletProperties();
}

void RewardsServiceImpl::GetPromotion(const std::string& lang,
    const std::string& payment_id) {
  ledger_->GetPromotion(lang, payment_id);
}

void RewardsServiceImpl::TriggerOnPromotion(const ledger::Promo result) {
  brave_rewards::Promotion properties;

  properties.promotionId = result.promotionId;
  properties.amount = result.amount;

  for (auto& observer : observers_)
    observer.OnPromotion(this, properties);
}

void RewardsServiceImpl::GetPromotionCaptcha() {
  ledger_->GetPromotionCaptcha();
}

void RewardsServiceImpl::TriggerOnPromotionCaptcha(const std::string& image) {
  for (auto& observer : observers_)
    observer.OnPromotionCaptcha(this, image);
}

std::string RewardsServiceImpl::GetWalletPassphrase() const {
  return ledger_->GetWalletPassphrase();
}

void RewardsServiceImpl::RecoverWallet(const std::string passPhrase) const {
  return ledger_->RecoverWallet(passPhrase);
}

void RewardsServiceImpl::TriggerOnRecoverWallet(ledger::Result result, double balance) {
  for (auto& observer : observers_)
    observer.OnRecoverWallet(this, result, balance);
}

void RewardsServiceImpl::SolvePromotionCaptcha(const std::string& solution) const {
  return ledger_->SolvePromotionCaptcha(solution);
}

void RewardsServiceImpl::TriggerOnPromotionFinish(ledger::Result result, unsigned int statusCode, uint64_t expirationDate) {
  for (auto& observer : observers_)
    observer.OnPromotionFinish(this, result, statusCode, expirationDate);
}


}  // namespace brave_rewards
