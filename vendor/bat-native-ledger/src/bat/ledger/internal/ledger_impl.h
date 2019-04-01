/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_IMPL_H_
#define BAT_LEDGER_LEDGER_IMPL_H_

#include <memory>
#include <map>
#include <string>
#include <fstream>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/ledger_client.h"

namespace base {
class SequencedTaskRunner;
}

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

namespace confirmations {
class Confirmations;
}

namespace bat_ledger {

class LedgerImpl : public ledger::Ledger,
                   public ledger::LedgerCallbackHandler {
 public:
  typedef std::map<uint32_t, ledger::VisitData>::const_iterator visit_data_iter;

  explicit LedgerImpl(ledger::LedgerClient* client);
  ~LedgerImpl() override;

  // Not copyable, not assignable
  LedgerImpl(const LedgerImpl&) = delete;
  LedgerImpl& operator=(const LedgerImpl&) = delete;

  std::string GenerateGUID() const;
  void Initialize() override;
  bool CreateWallet() override;

  void SetPublisherInfo(
      std::unique_ptr<ledger::PublisherInfo> publisher_info) override;

  void SetActivityInfo(
      std::unique_ptr<ledger::PublisherInfo> publisher_info) override;

  void GetPublisherInfo(const std::string& publisher_key,
                        ledger::PublisherInfoCallback callback) override;

  void GetActivityInfo(const ledger::ActivityInfoFilter& filter,
                       ledger::PublisherInfoCallback callback) override;

  void GetPanelPublisherInfo(const ledger::ActivityInfoFilter& filter,
                             ledger::PublisherInfoCallback callback);

  void GetMediaPublisherInfo(const std::string& media_key,
                             ledger::PublisherInfoCallback callback) override;

  void SetMediaPublisherInfo(const std::string& media_key,
                             const std::string& publisher_id) override;

  std::vector<ledger::ContributionInfo>
  GetRecurringDonationPublisherInfo() override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           const ledger::ActivityInfoFilter& filter,
                           ledger::PublisherInfoListCallback callback) override;

  void DoDirectDonation(const ledger::PublisherInfo& publisher,
                        int amount,
                        const std::string& currency) override;

  void SetRewardsMainEnabled(bool enabled) override;

  void SetPublisherMinVisitTime(uint64_t duration_in_seconds) override;

  void SetPublisherMinVisits(unsigned int visits) override;

  void SetPublisherAllowNonVerified(bool allow) override;

  void SetPublisherAllowVideos(bool allow) override;

  void SetContributionAmount(double amount) override;

  void SetUserChangedContribution() override;

  bool GetUserChangedContribution();

  void SetAutoContribute(bool enabled) override;

  void SetBalanceReport(ledger::ACTIVITY_MONTH month,
                        int year,
                        const ledger::BalanceReportInfo& report_info) override;

  void SaveUnverifiedContribution(const ledger::PendingContributionList& list);

  std::map<std::string, std::string> GetAddresses() override;

  const std::string& GetBATAddress() const override;

  const std::string& GetBTCAddress() const override;

  const std::string& GetETHAddress() const override;

  const std::string& GetLTCAddress() const override;

  uint64_t GetReconcileStamp() const override;

  bool GetRewardsMainEnabled() const override;

  uint64_t GetPublisherMinVisitTime() const override;  // In milliseconds

  unsigned int GetPublisherMinVisits() const override;

  void GetExcludedPublishersNumber(
      ledger::GetExcludedPublishersNumberDBCallback callback) const override;

  bool GetPublisherAllowNonVerified() const override;

  bool GetPublisherAllowVideos() const override;

  double GetContributionAmount() const override;

  bool GetAutoContribute() const override;

  bool GetBalanceReport(ledger::ACTIVITY_MONTH month,
                        int year,
                        ledger::BalanceReportInfo* report_info) const override;

  std::map<std::string, ledger::BalanceReportInfo>
  GetAllBalanceReports() const override;

  void GetAutoContributeProps(ledger::AutoContributeProps* props) override;

  void SaveLedgerState(const std::string& data);

  void SavePublisherState(const std::string& data,
                          ledger::LedgerCallbackHandler* handler);

  void SavePublishersList(const std::string& data);

  void LoadNicewareList(ledger::GetNicewareListCallback callback);

  void SetConfirmationsWalletInfo(
      const braveledger_bat_helper::WALLET_INFO_ST& wallet_info);

  void LoadLedgerState(ledger::LedgerCallbackHandler* handler);

  void LoadPublisherState(ledger::LedgerCallbackHandler* handler);

  void LoadPublisherList(ledger::LedgerCallbackHandler* handler);

  void OnWalletInitialized(ledger::Result result);

  void OnWalletProperties(ledger::Result result,
                          const braveledger_bat_helper::WALLET_PROPERTIES_ST&);

  void FetchWalletProperties(
      ledger::OnWalletPropertiesCallback callback) const override;

  void FetchGrants(const std::string& lang,
                   const std::string& paymentId) const override;

  void OnGrant(ledger::Result result,
               const braveledger_bat_helper::GRANT& grant);

  void GetGrantCaptcha(
      const std::string& promotion_id,
      const std::string& promotion_type) const override;

  void OnGrantCaptcha(const std::string& image, const std::string& hint);

  void SolveGrantCaptcha(const std::string& solution,
                         const std::string& promotionId) const override;

  void OnGrantFinish(ledger::Result result,
                     const braveledger_bat_helper::GRANT& grant);

  std::string GetWalletPassphrase() const override;

  void RecoverWallet(const std::string& passPhrase) const override;

  void OnRecoverWallet(
      ledger::Result result,
      double balance,
      const std::vector<braveledger_bat_helper::GRANT>& grants);

  void LoadPublishersListCallback(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnPublishersListSaved(ledger::Result result) override;

  void LoadURL(const std::string& url,
               const std::vector<std::string>& headers,
               const std::string& content,
               const std::string& contentType,
               const ledger::URL_METHOD method,
               ledger::LoadURLCallback callback);

  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id,
                           const std::string& probi = "0");

  std::string URIEncode(const std::string& value) override;

  void SaveMediaVisit(const std::string& publisher_id,
                      const ledger::VisitData& visit_data,
                      const uint64_t& duration,
                      const uint64_t window_id) override;

  void SetPublisherExclude(
      const std::string& publisher_id,
      const ledger::PUBLISHER_EXCLUDE& exclude) override;

  void RestorePublishers() override;

  void OnRestorePublishers(ledger::OnRestoreCallback callback);

  bool IsWalletCreated() const override;

  void GetPublisherActivityFromUrl(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& publisher_blob) override;

  void GetMediaActivityFromUrl(
      uint64_t windowId,
      const ledger::VisitData& visit_data,
      const std::string& providerType,
      const std::string& publisher_blob);

  void OnPanelPublisherInfo(ledger::Result result,
                           std::unique_ptr<ledger::PublisherInfo> info,
                           uint64_t windowId);

  void OnExcludedSitesChanged(const std::string& publisher_id,
                              ledger::PUBLISHER_EXCLUDE exclude);

  void SetBalanceReportItem(ledger::ACTIVITY_MONTH month,
                            int year,
                            ledger::ReportType type,
                            const std::string& probi) override;

  braveledger_bat_helper::CURRENT_RECONCILE
  GetReconcileById(const std::string& viewingId);

  void RemoveReconcileById(const std::string& viewingId);

  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::FetchIconCallback callback);

  void GetPublisherBanner(const std::string& publisher_id,
                          ledger::PublisherBannerCallback callback) override;

  double GetBalance() override;

  void OnReconcileCompleteSuccess(const std::string& viewing_id,
                                  const ledger::REWARDS_CATEGORY category,
                                  const std::string& probi,
                                  const ledger::ACTIVITY_MONTH month,
                                  const int year,
                                  const uint32_t date) override;

  void GetRecurringTips(ledger::PublisherInfoListCallback callback) override;

  void RemoveRecurringTip(const std::string& publisher_key) override;

  ledger::ActivityInfoFilter CreateActivityFilter(
      const std::string& publisher_id,
      ledger::EXCLUDE_FILTER excluded,
      bool min_duration,
      const uint64_t& currentReconcileStamp,
      bool non_verified,
      bool min_visits);

  void LogResponse(const std::string& func_name,
                   int response_status_code,
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

  const braveledger_bat_helper::Grants& GetGrants() const;

  void SetGrants(braveledger_bat_helper::Grants grants);
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

  const confirmations::WalletInfo GetConfirmationsWalletInfo(
      const braveledger_bat_helper::WALLET_INFO_ST& info) const;

  const braveledger_bat_helper::WALLET_PROPERTIES_ST&
  GetWalletProperties() const;

  void SetWalletProperties(
      braveledger_bat_helper::WALLET_PROPERTIES_ST* properties);

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

  uint64_t GetBootStamp() const override;

  void SetBootStamp(uint64_t stamp);

  const std::string& GetMasterUserToken() const;

  void SetMasterUserToken(const std::string& token);

  bool ReconcileExists(const std::string& viewingId);

  void SaveContributionInfo(const std::string& probi,
                            const int month,
                            const int year,
                            const uint32_t date,
                            const std::string& publisher_key,
                            const ledger::REWARDS_CATEGORY category);

  void NormalizeContributeWinners(
      ledger::PublisherInfoList* newList,
      const ledger::PublisherInfoList& list,
      uint32_t /* next_record */);

  void SetTimer(uint64_t time_offset, uint32_t* timer_id) const;

  bool AddReconcileStep(const std::string& viewing_id,
                        ledger::ContributionRetry step,
                        int level = -1);

  const braveledger_bat_helper::CurrentReconciles& GetCurrentReconciles() const;

  double GetDefaultContributionAmount() override;

  bool HasSufficientBalanceToReconcile() override;

  void SaveNormalizedPublisherList(
      const ledger::PublisherInfoList& normalized_list);

  void
  GetAddressesForPaymentId(ledger::WalletAddressesCallback callback) override;

  void SetAddresses(std::map<std::string, std::string> addresses);

  void SetCatalogIssuers(const std::string& info) override;
  void ConfirmAd(const std::string& info) override;
  void GetConfirmationsHistory(
      const uint64_t from_timestamp_seconds,
      const uint64_t to_timestamp_seconds,
      ledger::ConfirmationsHistoryCallback callback) override;

  std::unique_ptr<ledger::LogStream> Log(
      const char* file,
      const int line,
      const ledger::LogLevel log_level) const;

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();
  void GetRewardsInternalsInfo(ledger::RewardsInternalsInfo* info) override;

 private:
  void AddRecurringPayment(const std::string& publisher_id,
                           const double& value) override;

  void OnLoad(const ledger::VisitData& visit_data,
              const uint64_t& current_time) override;

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

  void saveVisitCallback(const std::string& publisher,
                         uint64_t verifiedTimestamp);

  void OnRemovedRecurring(ledger::Result result);

  // ledger::LedgerCallbacHandler implementation
  void OnPublisherStateLoaded(ledger::Result result,
                              const std::string& data) override;

  void OnLedgerStateLoaded(ledger::Result result,
                           const std::string& data) override;

  void RefreshPublishersList(bool retryAfterError, bool immediately = false);

  void RefreshGrant(bool retryAfterError);

  void OnPublisherListLoaded(ledger::Result result,
                             const std::string& data) override;

  uint64_t retryRequestSetup(uint64_t min_time, uint64_t max_time);

  void OnPublisherInfoSavedInternal(
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> info);

  ledger::LedgerClient* ledger_client_;
  std::unique_ptr<braveledger_bat_client::BatClient> bat_client_;
  std::unique_ptr<braveledger_bat_publishers::BatPublishers> bat_publishers_;
  std::unique_ptr<braveledger_bat_get_media::BatGetMedia> bat_get_media_;
  std::unique_ptr<braveledger_bat_state::BatState> bat_state_;
  std::unique_ptr<braveledger_bat_contribution::BatContribution>
  bat_contribution_;
  std::unique_ptr<confirmations::Confirmations> bat_confirmations_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool initialized_task_scheduler_;

  bool initialized_;
  bool initializing_;

  std::map<uint32_t, ledger::VisitData> current_pages_;
  uint64_t last_tab_active_time_;
  uint32_t last_shown_tab_id_;
  uint32_t last_pub_load_timer_id_;
  uint32_t last_grant_check_timer_id_;
};

}  // namespace bat_ledger

#endif  // BAT_LEDGER_LEDGER_IMPL_H_
