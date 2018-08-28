/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_

#include <memory>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/wallet_info.h"
#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/one_shot_event.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace ledger {
class Ledger;
class LedgerCallbackHandler;
}  // namespace ledger

namespace leveldb {
class DB;
}  // namespace leveldb

namespace net {
class URLFetcher;
}  // namespace net

class Profile;

namespace brave_rewards {

class PublisherInfoBackend;

class RewardsServiceImpl : public RewardsService,
                            public ledger::LedgerClient,
                            public net::URLFetcherDelegate,
                            public base::SupportsWeakPtr<RewardsServiceImpl> {
 public:
  RewardsServiceImpl(Profile* profile);
  ~RewardsServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  void Init();
  void CreateWallet() override;
  void GetWalletProperties() override;
  void GetGrant(const std::string& lang, const std::string& paymentId) override;
  void GetGrantCaptcha() override;
  void SolveGrantCaptcha(const std::string& solution) const override;
  std::string GetWalletPassphrase() const override;
  void RecoverWallet(const std::string passPhrase) const override;
  void GetContentSiteList(uint32_t start,
                          uint32_t limit,
     const GetContentSiteListCallback& callback) override;
  void OnLoad(SessionID tab_id, const GURL& url) override;
  void OnUnload(SessionID tab_id) override;
  void OnShow(SessionID tab_id) override;
  void OnHide(SessionID tab_id) override;
  void OnForeground(SessionID tab_id) override;
  void OnBackground(SessionID tab_id) override;
  void OnMediaStart(SessionID tab_id) override;
  void OnMediaStop(SessionID tab_id) override;
  void OnXHRLoad(SessionID tab_id, const GURL& url) override;
  std::string URIEncode(const std::string& value) override;
  uint64_t GetReconcileStamp() const override;
  std::map<std::string, std::string> GetAddresses() const override;

 private:
  typedef base::Callback<void(int, const std::string&)> FetchCallback;

  const extensions::OneShotEvent& ready() const { return ready_; }
  void OnLedgerStateSaved(ledger::LedgerCallbackHandler* handler,
                          bool success);
  void OnLedgerStateLoaded(ledger::LedgerCallbackHandler* handler,
                              const std::string& data);
  void OnPublisherStateSaved(ledger::LedgerCallbackHandler* handler,
                             bool success);
  void OnPublisherStateLoaded(ledger::LedgerCallbackHandler* handler,
                              const std::string& data);
  void TriggerOnWalletInitialized(int error_code);
  void TriggerOnWalletProperties(int error_code,
                                 std::unique_ptr<ledger::WalletInfo> result);
  void TriggerOnGrant(ledger::Result result, const ledger::Grant& grant);
  void TriggerOnGrantCaptcha(const std::string& image);
  void TriggerOnRecoverWallet(ledger::Result result, double balance, const std::vector<ledger::Grant>& grants);
  void TriggerOnGrantFinish(ledger::Result result, const ledger::Grant& grant);
  void OnPublisherInfoSaved(ledger::PublisherInfoCallback callback,
                            std::unique_ptr<ledger::PublisherInfo> info,
                            bool success);
  void OnPublisherInfoLoaded(ledger::PublisherInfoCallback callback,
                             std::unique_ptr<ledger::PublisherInfo> info);
  void OnPublisherInfoListLoaded(uint32_t start,
                                 uint32_t limit,
                                 ledger::GetPublisherInfoListCallback callback,
                                 const ledger::PublisherInfoList& list);

  // ledger::LedgerClient
  std::string GenerateGUID() const override;
  void OnWalletInitialized(ledger::Result result) override;
  void OnWalletProperties(ledger::Result result,
                          std::unique_ptr<ledger::WalletInfo> info) override;
  void OnGrant(ledger::Result result, const ledger::Grant& grant) override;
  void OnGrantCaptcha(const std::string& image) override;
  void OnRecoverWallet(ledger::Result result, double balance, const std::vector<ledger::Grant>& grants) override;
  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id) override;
  void OnGrantFinish(ledger::Result result, const ledger::Grant& grant) override;
  void LoadLedgerState(ledger::LedgerCallbackHandler* handler) override;
  void LoadPublisherState(ledger::LedgerCallbackHandler* handler) override;
  void SaveLedgerState(const std::string& ledger_state,
                       ledger::LedgerCallbackHandler* handler) override;
  void SavePublisherState(const std::string& publisher_state,
                          ledger::LedgerCallbackHandler* handler) override;

  void SavePublisherInfo(std::unique_ptr<ledger::PublisherInfo> publisher_info,
                         ledger::PublisherInfoCallback callback) override;
  void LoadPublisherInfo(const ledger::PublisherInfo::id_type& publisher_id,
                         ledger::PublisherInfoCallback callback) override;
  void LoadPublisherInfoList(
      uint32_t start,
      uint32_t limit,
      ledger::PublisherInfoFilter filter,
      const std::vector<std::string>& prefix,
      ledger::GetPublisherInfoListCallback callback) override;

  std::unique_ptr<ledger::LedgerURLLoader> LoadURL(const std::string& url,
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
  const base::FilePath publisher_info_db_path_;
  std::unique_ptr<PublisherInfoBackend> publisher_info_backend_;

  extensions::OneShotEvent ready_;
  std::map<const net::URLFetcher*, FetchCallback> fetchers_;

  DISALLOW_COPY_AND_ASSIGN(RewardsServiceImpl);
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_
