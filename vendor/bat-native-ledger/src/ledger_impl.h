/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_IMPL_H_
#define BAT_LEDGER_LEDGER_IMPL_H_

#include <memory>
#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/ledger_url_loader.h"
#include "bat_helper.h"
#include "ledger_task_runner_impl.h"
#include "url_request_handler.h"

namespace braveledger_bat_client {
class BatClient;
}

namespace braveledger_bat_get_media {
class BatGetMedia;
}

namespace braveledger_bat_publishers {
class BatPublishers;
}

namespace braveledger_bat_state {
class BatState;
}

namespace braveledger_bat_contribution {
class BatContribution;
}

namespace bat_ledger {

class LedgerImpl : public ledger::Ledger,
                   public ledger::LedgerCallbackHandler {
 public:
  typedef std::map<uint32_t, ledger::VisitData>::const_iterator visit_data_iter;

  LedgerImpl(ledger::LedgerClient* client);
  ~LedgerImpl() override;

  // Not copyable, not assignable
  LedgerImpl(const LedgerImpl&) = delete;
  LedgerImpl& operator=(const LedgerImpl&) = delete;

  std::string GenerateGUID() const;
  void Initialize() override;
  bool CreateWallet() override;

  void SetPublisherInfo(std::unique_ptr<ledger::PublisherInfo> publisher_info,
                        ledger::PublisherInfoCallback callback) override;
  void GetPublisherInfo(const ledger::PublisherInfoFilter& filter,
                        ledger::PublisherInfoCallback callback) override;
  void GetMediaPublisherInfo(const std::string& media_key,
                                ledger::PublisherInfoCallback callback) override;
  void SetMediaPublisherInfo(const std::string& media_key,
                            const std::string& publisher_id) override;
  std::vector<ledger::ContributionInfo> GetRecurringDonationPublisherInfo() override;
  void GetPublisherInfoList(uint32_t start, uint32_t limit,
                            const ledger::PublisherInfoFilter& filter,
                            ledger::PublisherInfoListCallback callback) override;
  void GetCurrentPublisherInfoList(uint32_t start, uint32_t limit,
                            const ledger::PublisherInfoFilter& filter,
                            ledger::PublisherInfoListCallback callback) override;

  void DoDirectDonation(const ledger::PublisherInfo& publisher, const int amount, const std::string& currency) override;

  void SetRewardsMainEnabled(bool enabled) override;
  void SetPublisherMinVisitTime(uint64_t duration_in_seconds) override;
  void SetPublisherMinVisits(unsigned int visits) override;
  void SetPublisherAllowNonVerified(bool allow) override;
  void SetPublisherAllowVideos(bool allow) override;
  void SetContributionAmount(double amount) override;
  void SetUserChangedContribution() override;
  bool GetUserChangedContribution();
  void SetAutoContribute(bool enabled) override;
  void SetBalanceReport(ledger::PUBLISHER_MONTH month,
                        int year,
                        const ledger::BalanceReportInfo& report_info) override;

  const std::string& GetBATAddress() const override;
  const std::string& GetBTCAddress() const override;
  const std::string& GetETHAddress() const override;
  const std::string& GetLTCAddress() const override;
  uint64_t GetReconcileStamp() const override;
  bool GetRewardsMainEnabled() const override;
  uint64_t GetPublisherMinVisitTime() const override; // In milliseconds
  unsigned int GetPublisherMinVisits() const override;
  unsigned int GetNumExcludedSites() const override;
  bool GetPublisherAllowNonVerified() const override;
  bool GetPublisherAllowVideos() const override;
  double GetContributionAmount() const override;
  bool GetAutoContribute() const override;
  bool GetBalanceReport(ledger::PUBLISHER_MONTH month,
                        int year,
                        ledger::BalanceReportInfo* report_info) const override;
  std::map<std::string, ledger::BalanceReportInfo> GetAllBalanceReports() const override;

  void SaveLedgerState(const std::string& data);
  void SavePublisherState(const std::string& data,
                          ledger::LedgerCallbackHandler* handler);
  void SavePublishersList(const std::string& data);
  void LoadNicewareList(ledger::GetNicewareListCallback callback);

  void LoadLedgerState(ledger::LedgerCallbackHandler* handler);
  void LoadPublisherState(ledger::LedgerCallbackHandler* handler);
  void LoadPublisherList(ledger::LedgerCallbackHandler* handler);

  void OnWalletInitialized(ledger::Result result);

  void OnWalletProperties(ledger::Result result,
                          const braveledger_bat_helper::WALLET_PROPERTIES_ST&);
  void FetchWalletProperties() const override;

  void FetchGrant(const std::string& lang, const std::string& paymentId) const override;
  void OnGrant(ledger::Result result, const braveledger_bat_helper::GRANT& grant);

  void GetGrantCaptcha() const override;
  void OnGrantCaptcha(const std::string& image, const std::string& hint);

  void SolveGrantCaptcha(const std::string& solution) const override;
  void OnGrantFinish(ledger::Result result, const braveledger_bat_helper::GRANT& grant);

  std::string GetWalletPassphrase() const override;
  void RecoverWallet(const std::string& passPhrase) const override;
  void OnRecoverWallet(ledger::Result result, double balance, const std::vector<braveledger_bat_helper::GRANT>& grants);

  void LoadPublishersListCallback(bool result, const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnPublishersListSaved(ledger::Result result) override;

  std::unique_ptr<ledger::LedgerURLLoader> LoadURL(const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::URL_METHOD& method,
      ledger::LedgerCallbackHandler* handler);
  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id,
                           const std::string& probi = "0");
  void RunIOTask(LedgerTaskRunnerImpl::Task task);
  std::string URIEncode(const std::string& value) override;
  void SaveMediaVisit(const std::string& publisher_id,
                      const ledger::VisitData& visit_data,
                      const uint64_t& duration,
                      const uint64_t window_id) override;
  void SetPublisherExclude(const std::string& publisher_id, const ledger::PUBLISHER_EXCLUDE& exclude) override;
  void SetPublisherPanelExclude(const std::string& publisher_id,
    const ledger::PUBLISHER_EXCLUDE& exclude, uint64_t windowId) override;

  void RestorePublishers() override;
  bool IsWalletCreated() const override;
  void GetPublisherActivityFromUrl(uint64_t windowId, const ledger::VisitData& visit_data) override;
  void GetMediaActivityFromUrl(uint64_t windowId,
                               const ledger::VisitData& visit_data,
                               const std::string& providerType);
  void OnPublisherActivity(ledger::Result result,
                           std::unique_ptr<ledger::PublisherInfo> info,
                           uint64_t windowId);
  void OnExcludedSitesChanged();
  void SetBalanceReportItem(ledger::PUBLISHER_MONTH month,
                            int year,
                            ledger::ReportType type,
                            const std::string& probi) override;

  braveledger_bat_helper::CURRENT_RECONCILE GetReconcileById(const std::string& viewingId);
  void RemoveReconcileById(const std::string& viewingId);
  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::FetchIconCallback callback);
  void GetPublisherBanner(const std::string& publisher_id,
                          ledger::PublisherBannerCallback callback) override;
  double GetBalance() override;
  void OnReconcileCompleteSuccess(const std::string& viewing_id,
                                  const ledger::PUBLISHER_CATEGORY category,
                                  const std::string& probi,
                                  const ledger::PUBLISHER_MONTH month,
                                  const int year,
                                  const uint32_t date) override;
  void GetRecurringDonations(ledger::PublisherInfoListCallback callback);
  void RemoveRecurring(const std::string& publisher_key) override;
  ledger::PublisherInfoFilter CreatePublisherFilter(const std::string& publisher_id,
      ledger::PUBLISHER_CATEGORY category,
      ledger::PUBLISHER_MONTH month,
      int year,
      ledger::PUBLISHER_EXCLUDE_FILTER excluded,
      bool min_duration,
      const uint64_t& currentReconcileStamp);
  void Log(const std::string& func_name,
           const ledger::LogLevel log_level,
           std::vector<std::string> data);
  void LogResponse(const std::string& func_name,
                   bool result,
                   const std::string& response,
                   const std::map<std::string,
                   std::string>& headers);
  void ResetReconcileStamp();
  bool UpdateReconcile(
    const braveledger_bat_helper::CURRENT_RECONCILE& reconcile);
  void AddReconcile(
      const std::string& viewing_id,
      const braveledger_bat_helper::CURRENT_RECONCILE& reconcile);

  const std::string& GetPaymentId() const;
  void SetPaymentId(const std::string& payment_id);
  const braveledger_bat_helper::GRANT& GetGrant() const;
  void SetGrant(braveledger_bat_helper::GRANT grant);
  const std::string& GetPersonaId() const;
  void SetPersonaId(const std::string& persona_id);
  const std::string& GetUserId() const;
  void SetUserId(const std::string& user_id);
  const std::string& GetRegistrarVK() const;
  void SetRegistrarVK(const std::string& registrar_vk);
  const std::string& GetPreFlight() const;
  void SetPreFlight(const std::string& pre_flight);
  const braveledger_bat_helper::WALLET_INFO_ST& GetWalletInfo() const;
  void SetWalletInfo(const braveledger_bat_helper::WALLET_INFO_ST& info);

  const braveledger_bat_helper::WALLET_PROPERTIES_ST&
  GetWalletProperties() const;

  void SetWalletProperties(
      braveledger_bat_helper::WALLET_PROPERTIES_ST& properties);
  unsigned int GetDays() const;
  void SetDays(unsigned int days);

  const braveledger_bat_helper::Transactions& GetTransactions() const;
  void SetTransactions(
      const braveledger_bat_helper::Transactions& transactions);

  const braveledger_bat_helper::Ballots& GetBallots() const;
  void SetBallots(
      const braveledger_bat_helper::Ballots& ballots);

  const braveledger_bat_helper::BatchVotes& GetBatch() const;
  void SetBatch(
      const braveledger_bat_helper::BatchVotes& votes);

  const std::string& GetCurrency() const;
  void SetCurrency(const std::string& currency);

  void SetLastGrantLoadTimestamp(uint64_t stamp);

  void SetBootStamp(uint64_t stamp);

  const std::string& GetMasterUserToken() const;

  void SetMasterUserToken(const std::string& token);

  bool ReconcileExists(const std::string& viewingId);

  void SaveContributionInfo(const std::string& probi,
                            const int month,
                            const int year,
                            const uint32_t date,
                            const std::string& publisher_key,
                            const ledger::PUBLISHER_CATEGORY category);

  void NormalizeContributeWinners(
      ledger::PublisherInfoList* newList,
      bool saveData,
      const braveledger_bat_helper::PublisherList& list,
      uint32_t /* next_record */);

  void SetTimer(uint64_t time_offset, uint32_t& timer_id) const;

  bool AddReconcileStep(const std::string& viewing_id,
                        braveledger_bat_helper::ContributionRetry step,
                        int level = -1);

  const braveledger_bat_helper::CurrentReconciles& GetCurrentReconciles() const;
  double GetDefaultContributionAmount() override;

 private:
  void MakePayment(const ledger::PaymentData& payment_data) override;
  void AddRecurringPayment(const std::string& publisher_id, const double& value) override;
  void OnLoad(const ledger::VisitData& visit_data, const uint64_t& current_time) override;
  void OnUnload(uint32_t tab_id, const uint64_t& current_time) override;
  void OnShow(uint32_t tab_id, const uint64_t& current_time) override;
  void OnHide(uint32_t tab_id, const uint64_t& current_time) override;
  void OnForeground(uint32_t tab_id, const uint64_t& current_time) override;
  void OnBackground(uint32_t tab_id, const uint64_t& current_time) override;
  void OnMediaStart(uint32_t tab_id, const uint64_t& current_time) override;
  void OnMediaStop(uint32_t tab_id, const uint64_t& current_time) override;
  void OnXHRLoad(
      uint32_t tab_id,
      const std::string& url,
      const std::map<std::string, std::string>& parts,
      const std::string& first_party_url,
      const std::string& referrer,
      const ledger::VisitData& visit_data) override;
  void OnPostData(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      const ledger::VisitData& visit_data) override;

  void OnTimer(uint32_t timer_id) override;

  void OnSetPublisherInfo(ledger::PublisherInfoCallback callback,
                          ledger::Result result,
                          std::unique_ptr<ledger::PublisherInfo> info);

  void saveVisitCallback(const std::string& publisher,
                         uint64_t verifiedTimestamp);

  void OnRemovedRecurring(ledger::Result result);

  // ledger::LedgerCallbacHandler implementation
  void OnPublisherStateLoaded(ledger::Result result,
                              const std::string& data) override;
  void OnLedgerStateLoaded(ledger::Result result,
                           const std::string& data) override;

  void RefreshPublishersList(bool retryAfterError);
  void RefreshGrant(bool retryAfterError);

  void OnPublisherListLoaded(ledger::Result result,
                             const std::string& data) override;
  uint64_t retryRequestSetup(uint64_t min_time, uint64_t max_time);

  ledger::LedgerClient* ledger_client_;
  std::unique_ptr<braveledger_bat_client::BatClient> bat_client_;
  std::unique_ptr<braveledger_bat_publishers::BatPublishers> bat_publishers_;
  std::unique_ptr<braveledger_bat_get_media::BatGetMedia> bat_get_media_;
  std::unique_ptr<braveledger_bat_state::BatState> bat_state_;
  std::unique_ptr<braveledger_bat_contribution::BatContribution> bat_contribution_;
  bool initialized_;
  bool initializing_;

  URLRequestHandler handler_;

  //ledger::VisitData current_visit_data_;
  std::map<uint32_t, ledger::VisitData> current_pages_;
  uint64_t last_tab_active_time_;
  uint32_t last_shown_tab_id_;
  uint32_t last_pub_load_timer_id_;
  uint32_t last_grant_check_timer_id_;
 };
}  // namespace bat_ledger

#endif  // BAT_LEDGER_LEDGER_IMPL_H_
