/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/wallet_info.h"
#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/one_shot_event.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/contribution_info.h"
#include "ui/gfx/image/image.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/components/brave_rewards/browser/extension_rewards_service_observer.h"
#endif

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace ledger {
class Ledger;
class LedgerCallbackHandler;
struct LedgerMediaPublisherInfo;
}  // namespace ledger

namespace leveldb {
class DB;
}  // namespace leveldb

namespace net {
class URLFetcher;
}  // namespace net

class Profile;

namespace brave_rewards {

class PublisherInfoDatabase;
class RewardsNotificationService;

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
  void FetchWalletProperties() override;
  void FetchGrant(const std::string& lang, const std::string& paymentId) override;
  void GetGrantCaptcha() override;
  void SolveGrantCaptcha(const std::string& solution) const override;
  std::string GetWalletPassphrase() const override;
  unsigned int GetNumExcludedSites() const override;
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
  void OnXHRLoad(SessionID tab_id,
                 const GURL& url,
                 const GURL& first_party_url,
                 const GURL& referrer) override;
  void OnPostData(SessionID tab_id,
                  const GURL& url,
                  const GURL& first_party_url,
                  const GURL& referrer,
                  const std::string& post_data) override;
  std::string URIEncode(const std::string& value) override;
  uint64_t GetReconcileStamp() const override;
  std::map<std::string, std::string> GetAddresses() const override;
  bool GetAutoContribute() const override;
  uint64_t GetPublisherMinVisitTime() const override;
  unsigned int GetPublisherMinVisits() const override;
  bool GetPublisherAllowNonVerified() const override;
  bool GetPublisherAllowVideos() const override;
  void LoadMediaPublisherInfo(
      const std::string& media_key,
      ledger::PublisherInfoCallback callback) override;
  void SaveMediaPublisherInfo(const std::string& media_key, const std::string& publisher_id) override;
  void ExcludePublisher(const std::string publisherKey) const override;
  void RestorePublishers() override;
  std::map<std::string, brave_rewards::BalanceReport> GetAllBalanceReports() override;
  void GetCurrentBalanceReport() override;
  bool IsWalletCreated() override;
  void GetPublisherActivityFromUrl(uint64_t windowId, const std::string& url, const std::string& favicon_url) override;
  double GetContributionAmount() override;
  void GetPublisherBanner(const std::string& publisher_id) override;
  void OnPublisherBanner(std::unique_ptr<ledger::PublisherBanner> banner);
  void RemoveRecurring(const std::string& publisher_key) override;
  void UpdateRecurringDonationsList() override;
  void UpdateTipsList() override;
  void SetContributionAutoInclude(
    std::string publisher_key, bool excluded, uint64_t windowId) override;
  RewardsNotificationService* GetNotificationService() const override;

  static void HandleFlags(const std::string& options);
  void OnWalletProperties(ledger::Result result,
                          std::unique_ptr<ledger::WalletInfo> info) override;

 private:
  friend void RunIOTaskCallback(
      base::WeakPtr<RewardsServiceImpl>,
      std::function<void(void)>);
  typedef base::Callback<void(int, const std::string&, const std::map<std::string, std::string>& headers)> FetchCallback;

  const extensions::OneShotEvent& ready() const { return ready_; }
  void OnLedgerStateSaved(ledger::LedgerCallbackHandler* handler,
                          bool success);
  void OnLedgerStateLoaded(ledger::LedgerCallbackHandler* handler,
                              const std::string& data);
  void LoadNicewareList(ledger::GetNicewareListCallback callback) override;
  void OnPublisherStateSaved(ledger::LedgerCallbackHandler* handler,
                             bool success);
  void OnPublisherStateLoaded(ledger::LedgerCallbackHandler* handler,
                              const std::string& data);
  void TriggerOnWalletInitialized(int error_code);
  void TriggerOnWalletProperties(int error_code,
                                 std::unique_ptr<ledger::WalletInfo> result);
  void TriggerOnGrant(ledger::Result result, const ledger::Grant& grant);
  void TriggerOnGrantCaptcha(const std::string& image, const std::string& hint);
  void TriggerOnRecoverWallet(ledger::Result result,
                              double balance,
                              const std::vector<ledger::Grant>& grants);
  void TriggerOnGrantFinish(ledger::Result result, const ledger::Grant& grant);
  void OnPublisherInfoSaved(ledger::PublisherInfoCallback callback,
                            std::unique_ptr<ledger::PublisherInfo> info,
                            bool success);
  void OnPublisherInfoLoaded(ledger::PublisherInfoCallback callback,
                             const ledger::PublisherInfoList list);
  void OnMediaPublisherInfoSaved(bool success);
  void OnMediaPublisherInfoLoaded(ledger::PublisherInfoCallback callback,
                             std::unique_ptr<ledger::PublisherInfo> info);
  void OnPublisherInfoListLoaded(uint32_t start,
                                 uint32_t limit,
                                 ledger::PublisherInfoListCallback callback,
                                 const ledger::PublisherInfoList& list);
  void OnPublishersListSaved(ledger::LedgerCallbackHandler* handler,
                             bool success);
  void OnTimer(uint32_t timer_id);
  void TriggerOnContentSiteUpdated();
  void OnPublisherListLoaded(ledger::LedgerCallbackHandler* handler,
                             const std::string& data);
  void OnDonate(const std::string& publisher_key, int amount, bool recurring) override;
  void OnContributionInfoSaved(const ledger::PUBLISHER_CATEGORY category, bool success);
  void OnRecurringDonationSaved(bool success);
  void SaveRecurringDonation(const std::string& publisher_key, const int amount);
  void OnRecurringDonationsData(const ledger::PublisherInfoListCallback callback,
                                const ledger::PublisherInfoList list);
  void OnRecurringDonationUpdated(const ledger::PublisherInfoList& list);
  void OnTipsUpdatedData(const ledger::PublisherInfoList list);
  void TipsUpdated();
  void OnRemovedRecurring(ledger::RecurringRemoveCallback callback, bool success);
  void OnRemoveRecurring(const std::string& publisher_key, ledger::RecurringRemoveCallback callback) override;
  void TriggerOnGetCurrentBalanceReport(const ledger::BalanceReportInfo& report);
  void TriggerOnGetPublisherActivityFromUrl(
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info,
      uint64_t windowId);

  // ledger::LedgerClient
  std::string GenerateGUID() const override;
  void OnWalletInitialized(ledger::Result result) override;
  void OnGrant(ledger::Result result, const ledger::Grant& grant) override;
  void OnGrantCaptcha(const std::string& image, const std::string& hint) override;
  void OnRecoverWallet(ledger::Result result,
                      double balance,
                      const std::vector<ledger::Grant>& grants) override;
  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id,
                           ledger::PUBLISHER_CATEGORY category,
                           const std::string& probi) override;
  void OnGrantFinish(ledger::Result result,
                     const ledger::Grant& grant) override;
  void LoadLedgerState(ledger::LedgerCallbackHandler* handler) override;
  void LoadPublisherState(ledger::LedgerCallbackHandler* handler) override;
  void SaveLedgerState(const std::string& ledger_state,
                       ledger::LedgerCallbackHandler* handler) override;
  void SavePublisherState(const std::string& publisher_state,
                          ledger::LedgerCallbackHandler* handler) override;

  void SavePublisherInfo(std::unique_ptr<ledger::PublisherInfo> publisher_info,
                         ledger::PublisherInfoCallback callback) override;
  void LoadPublisherInfo(ledger::PublisherInfoFilter filter,
                         ledger::PublisherInfoCallback callback) override;
  void LoadPublisherInfoList(
      uint32_t start,
      uint32_t limit,
      ledger::PublisherInfoFilter filter,
      ledger::PublisherInfoListCallback callback) override;
  void LoadCurrentPublisherInfoList(
      uint32_t start,
      uint32_t limit,
      ledger::PublisherInfoFilter filter,
      ledger::PublisherInfoListCallback callback) override;
  void SavePublishersList(const std::string& publishers_list,
                          ledger::LedgerCallbackHandler* handler) override;
  void SetTimer(uint64_t time_offset, uint32_t& timer_id) override;
  void LoadPublisherList(ledger::LedgerCallbackHandler* handler) override;

  std::unique_ptr<ledger::LedgerURLLoader> LoadURL(const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::URL_METHOD& method,
      ledger::LedgerCallbackHandler* handler) override;

  void RunIOTask(std::unique_ptr<ledger::LedgerTaskRunner> task) override;
  void SetRewardsMainEnabled(bool enabled) const override;
  void SetPublisherMinVisitTime(uint64_t duration_in_seconds) const override;
  void SetPublisherMinVisits(unsigned int visits) const override;
  void SetPublisherAllowNonVerified(bool allow) const override;
  void SetPublisherAllowVideos(bool allow) const override;
  void SetContributionAmount(double amount) const override;
  void SetUserChangedContribution() const override;
  void SetAutoContribute(bool enabled) const override;
  void OnExcludedSitesChanged() override;
  void OnPublisherActivity(ledger::Result result,
                          std::unique_ptr<ledger::PublisherInfo> info,
                          uint64_t windowId) override;
  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::FetchIconCallback callback) override;
  void OnFetchFavIconCompleted(ledger::FetchIconCallback callback,
                          const std::string& favicon_key,
                          const GURL& url,
                          const BitmapFetcherService::RequestId& request_id,
                          const SkBitmap& image);
  void OnSetOnDemandFaviconComplete(const std::string& favicon_url,
                                    ledger::FetchIconCallback callback,
                                    bool success);
  void SaveContributionInfo(const std::string& probi,
                            const int month,
                            const int year,
                            const uint32_t date,
                            const std::string& publisher_key,
                            const ledger::PUBLISHER_CATEGORY category) override;
  void GetRecurringDonations(ledger::PublisherInfoListCallback callback) override;
  std::unique_ptr<ledger::LogStream> Log(
                     const char* file,
                     int line,
                     const ledger::LogLevel log_level) const override;

  void OnIOTaskComplete(std::function<void(void)> callback);

  // URLFetcherDelegate impl
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  Profile* profile_;  // NOT OWNED
  std::unique_ptr<ledger::Ledger> ledger_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<ExtensionRewardsServiceObserver>
      extension_rewards_service_observer_;
#endif
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  const base::FilePath ledger_state_path_;
  const base::FilePath publisher_state_path_;
  const base::FilePath publisher_info_db_path_;
  const base::FilePath publisher_list_path_;
  std::unique_ptr<PublisherInfoDatabase> publisher_info_backend_;
  std::unique_ptr<RewardsNotificationService> notification_service_;
  base::ObserverList<RewardsServicePrivateObserver> private_observers_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<ExtensionRewardsServiceObserver> private_observer_;
#endif

  extensions::OneShotEvent ready_;
  std::map<const net::URLFetcher*, FetchCallback> fetchers_;
  std::map<uint32_t, std::unique_ptr<base::OneShotTimer>> timers_;
  std::vector<std::string> current_media_fetchers_;
  std::vector<BitmapFetcherService::RequestId> request_ids_;

  uint32_t next_timer_id_;

  DISALLOW_COPY_AND_ASSIGN(RewardsServiceImpl);
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_IMPL_
