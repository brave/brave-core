/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/payments/payments_service_impl.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "bat/ledger/ledger.h"
#include "brave/browser/payments/payments_service_observer.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "net/url_request/url_fetcher.h"
#include "url/gurl.h"

namespace payments {

namespace {

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

// `callback` has a WeakPtr so this won't crash if the file finishes
// writing after PaymentServiceImpl has been destroyed
void PostWriteCallback(
    const base::Callback<void(bool success)>& callback,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    bool write_success) {
  // We can't run |callback| on the current thread. Bounce back to
  // the |reply_task_runner| which is the correct sequenced thread.
  reply_task_runner->PostTask(FROM_HERE,
                              base::Bind(callback, write_success));
}

static uint64_t next_id = 1;

}  // namespace

PaymentsServiceImpl::PaymentsServiceImpl(Profile* profile) :
    profile_(profile),
    ledger_(ledger::Ledger::CreateInstance(this)),
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BACKGROUND,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    ledger_state_path_(profile_->GetPath().Append("ledger_state")),
    publisher_state_path_(profile_->GetPath().Append("publisher_state")) {
}

PaymentsServiceImpl::~PaymentsServiceImpl() {
}

void PaymentsServiceImpl::CreateWallet() {
  ledger_->CreateWallet();
}

std::string PaymentsServiceImpl::GenerateGUID() const {
  return base::GenerateGUID();
}

void PaymentsServiceImpl::Shutdown() {
  fetchers_.clear();
  ledger_.reset();
  PaymentsService::Shutdown();
}

void PaymentsServiceImpl::OnWalletCreated(ledger::Result result) {
  TriggerOnWalletCreated(result);
}

void PaymentsServiceImpl::OnReconcileComplete(ledger::Result result,
                                              const std::string& viewing_id) {
  LOG(ERROR) << "reconcile complete " << viewing_id;
}

void PaymentsServiceImpl::LoadLedgerState(
    ledger::LedgerCallbackHandler* handler) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadStateOnFileTaskRunner, ledger_state_path_),
      base::Bind(&PaymentsServiceImpl::OnLedgerStateLoaded,
                     AsWeakPtr(),
                     base::Unretained(handler)));
}

void PaymentsServiceImpl::OnLedgerStateLoaded(
    ledger::LedgerCallbackHandler* handler,
    const std::string& data) {
  handler->OnLedgerStateLoaded(data.empty() ? ledger::Result::ERROR
                                            : ledger::Result::OK,
                               data);
}

void PaymentsServiceImpl::LoadPublisherState(
    ledger::LedgerCallbackHandler* handler) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadStateOnFileTaskRunner, publisher_state_path_),
      base::Bind(&PaymentsServiceImpl::OnPublisherStateLoaded,
                     AsWeakPtr(),
                     base::Unretained(handler)));
}

void PaymentsServiceImpl::OnPublisherStateLoaded(
    ledger::LedgerCallbackHandler* handler,
    const std::string& data) {
  handler->OnPublisherStateLoaded(data.empty() ? ledger::Result::ERROR
                                               : ledger::Result::OK,
                                  data);
}

void PaymentsServiceImpl::SaveLedgerState(const std::string& ledger_state,
                                      ledger::LedgerCallbackHandler* handler) {
  base::ImportantFileWriter writer(
      ledger_state_path_, file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
      base::Closure(),
      base::Bind(
        &PostWriteCallback,
        base::Bind(&PaymentsServiceImpl::OnLedgerStateSaved, AsWeakPtr(),
            base::Unretained(handler)),
        base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(ledger_state));
}

void PaymentsServiceImpl::OnLedgerStateSaved(
    ledger::LedgerCallbackHandler* handler,
    bool success) {
  handler->OnLedgerStateSaved(success ? ledger::Result::OK
                                      : ledger::Result::ERROR);
}

void PaymentsServiceImpl::SavePublisherState(const std::string& publisher_state,
                                      ledger::LedgerCallbackHandler* handler) {
  base::ImportantFileWriter writer(publisher_state_path_, file_task_runner_);

  writer.RegisterOnNextWriteCallbacks(
      base::Closure(),
      base::Bind(
        &PostWriteCallback,
        base::Bind(&PaymentsServiceImpl::OnPublisherStateSaved, AsWeakPtr(),
            base::Unretained(handler)),
        base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(std::make_unique<std::string>(publisher_state));
}

void PaymentsServiceImpl::OnPublisherStateSaved(
    ledger::LedgerCallbackHandler* handler,
    bool success) {
  handler->OnPublisherStateSaved(success ? ledger::Result::OK
                                         : ledger::Result::ERROR);
}

uint64_t PaymentsServiceImpl::LoadURL(const std::string& url,
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

  fetcher->Start();
  FetchCallback callback = base::Bind(
      &ledger::LedgerCallbackHandler::OnURLRequestResponse,
      base::Unretained(handler),
      next_id);
  fetchers_[fetcher] = callback;

  return next_id++;
}

void PaymentsServiceImpl::OnURLFetchComplete(
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

void PaymentsServiceImpl::RunIOTask(
    std::unique_ptr<ledger::LedgerTaskRunner> task) {
  file_task_runner_->PostTask(FROM_HERE,
      base::BindOnce(&ledger::LedgerTaskRunner::Run, std::move(task)));
}

void PaymentsServiceImpl::RunTask(
      std::unique_ptr<ledger::LedgerTaskRunner> task) {
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&ledger::LedgerTaskRunner::Run,
                     std::move(task)));
}

void PaymentsServiceImpl::TriggerOnWalletCreated(int error_code) {
  for (auto& observer : observers_)
    observer.OnWalletCreated(this, error_code);
}

}  // namespace payments
