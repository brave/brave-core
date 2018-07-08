/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_

#include <memory>

#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger_client.h"
#include "brave/browser/payments/payments_service.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace base {
class SequencedTaskRunner;
}

namespace ledger {
class Ledger;
class LedgerCallbackHandler;
}

namespace net {
class URLFetcher;
}

class Profile;

namespace payments {

class PaymentsServiceImpl : public PaymentsService,
                            public ledger::LedgerClient,
                            public net::URLFetcherDelegate,
                            public base::SupportsWeakPtr<PaymentsServiceImpl> {
 public:
  PaymentsServiceImpl(Profile* profile);
  ~PaymentsServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  void CreateWallet() override;

 private:
  typedef base::Callback<void(int, const std::string&)> FetchCallback;

  void OnLedgerStateSaved(ledger::LedgerCallbackHandler* handler,
                          bool success);
  void OnLedgerStateLoaded(ledger::LedgerCallbackHandler* handler,
                              const std::string& data);
  void OnPublisherStateSaved(ledger::LedgerCallbackHandler* handler,
                             bool success);
  void OnPublisherStateLoaded(ledger::LedgerCallbackHandler* handler,
                              const std::string& data);

  void TriggerOnWalletCreated(int error_code);

  // ledger::LedgerClient
  std::string GenerateGUID() const override;
  void OnWalletCreated(ledger::Result result) override;
  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id) override;
  void LoadLedgerState(ledger::LedgerCallbackHandler* handler) override;
  void LoadPublisherState(ledger::LedgerCallbackHandler* handler) override;
  void SaveLedgerState(const std::string& ledger_state,
                       ledger::LedgerCallbackHandler* handler) override;
  void SavePublisherState(const std::string& publisher_state,
                          ledger::LedgerCallbackHandler* handler) override;
  uint64_t LoadURL(const std::string& url,
                   const std::vector<std::string>& headers,
                   const std::string& content,
                   const std::string& contentType,
                   const ledger::URL_METHOD& method,
                   ledger::LedgerCallbackHandler* handler) override;
  void RunIOTask(std::unique_ptr<ledger::LedgerTaskRunner> task) override;
  void RunTask(std::unique_ptr<ledger::LedgerTaskRunner> task) override;

  // URLFetcherDelegate impl
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  Profile* profile_;  // NOT OWNED
  std::unique_ptr<ledger::Ledger> ledger_;
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  const base::FilePath ledger_state_path_;
  const base::FilePath publisher_state_path_;

  std::map<const net::URLFetcher*, FetchCallback> fetchers_;

  DISALLOW_COPY_AND_ASSIGN(PaymentsServiceImpl);
};

}  // namespace history

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_
