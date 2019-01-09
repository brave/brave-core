/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_IMPL_H_
#define BAT_LEDGER_LEDGER_IMPL_H_

#include <stdint.h>

#include <memory>
#include <map>
#include <string>
#include <fstream>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/internal/wallet/wallet.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/ledger_client.h"

namespace base {
class SequencedTaskRunner;
}

namespace braveledger_grant {
class Grants;
}

namespace braveledger_media {
class Media;
}

namespace braveledger_publisher {
class Publisher;
}

namespace braveledger_bat_state {
class BatState;
}

namespace braveledger_contribution {
class Contribution;
}

namespace braveledger_wallet {
class Wallet;
}

namespace confirmations {
class Confirmations;
}

namespace bat_ledger {

class LedgerImpl : public ledger::Ledger,
                   public ledger::LedgerCallbackHandler {
 public:
  typedef std::map<uint32_t,
      ledger::VisitData>::const_iterator visit_data_iter;

  explicit LedgerImpl(ledger::LedgerClient* client);
  ~LedgerImpl() override;

  // Not copyable, not assignable
  LedgerImpl(const LedgerImpl&) = delete;
  LedgerImpl& operator=(const LedgerImpl&) = delete;

  std::string GenerateGUID() const;

  void Initialize(ledger::InitializeCallback callback) override;

  void CreateWallet(ledger::CreateWalletCallback callback) override;

  void SetPublisherInfo(
      ledger::PublisherInfoPtr publisher_info);

  void SetActivityInfo(
      ledger::PublisherInfoPtr publisher_info);

  void GetPublisherInfo(const std::string& publisher_key,
                        ledger::PublisherInfoCallback callback) override;

  void GetActivityInfo(ledger::ActivityInfoFilterPtr filter,
                       ledger::PublisherInfoCallback callback);

  void GetPanelPublisherInfo(ledger::ActivityInfoFilterPtr filter,
                             ledger::PublisherInfoCallback callback);

  void GetMediaPublisherInfo(const std::string& media_key,
                             ledger::PublisherInfoCallback callback);

  void SetMediaPublisherInfo(const std::string& media_key,
                             const std::string& publisher_id);

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           ledger::ActivityInfoFilterPtr filter,
                           ledger::PublisherInfoListCallback callback) override;

  void DoDirectTip(const std::string& publisher_key,
                   int amount,
                   const std::string& currency,
                   ledger::DoDirectTipCallback callback) override;

  void SetRewardsMainEnabled(bool enabled) override;

  void SetPublisherMinVisitTime(uint64_t duration_in_seconds) override;

  void SetPublisherMinVisits(unsigned int visits) override;

  void SetPublisherAllowNonVerified(bool allow) override;

  void SetPublisherAllowVideos(bool allow) override;

  void SetContributionAmount(double amount) override;

  void SetUserChangedContribution() override;

  bool GetUserChangedContribution();

  void SetAutoContribute(bool enabled) override;

  void UpdateAdsRewards() override;

  void SaveUnverifiedContribution(
      ledger::PendingContributionList list,
      ledger::SavePendingContributionCallback callback);

  uint64_t GetReconcileStamp() const override;

  bool GetRewardsMainEnabled() const override;

  uint64_t GetPublisherMinVisitTime() const override;  // In milliseconds

  unsigned int GetPublisherMinVisits() const override;

  bool GetPublisherAllowNonVerified() const override;

  bool GetPublisherAllowVideos() const override;

  double GetContributionAmount() const override;

  bool GetAutoContribute() const override;

  void GetBalanceReport(
      ledger::ACTIVITY_MONTH month,
      int year,
      ledger::GetBalanceReportCallback callback) const override;

  std::map<std::string, ledger::BalanceReportInfoPtr>
  GetAllBalanceReports() const override;

  ledger::AutoContributePropsPtr GetAutoContributeProps() override;

  void SaveLedgerState(const std::string& data);

  void SavePublisherState(const std::string& data,
                          ledger::LedgerCallbackHandler* handler);

  void LoadNicewareList(ledger::GetNicewareListCallback callback);

  void SetConfirmationsWalletInfo(
      const braveledger_bat_helper::WALLET_INFO_ST& wallet_info);

  void LoadLedgerState(ledger::OnLoadCallback callback);

  void LoadPublisherState(ledger::OnLoadCallback callback);

  void OnWalletInitializedInternal(ledger::Result result,
                                   ledger::InitializeCallback callback);

  void OnWalletProperties(ledger::Result result,
                          const braveledger_bat_helper::WALLET_PROPERTIES_ST&);

  void FetchWalletProperties(
      ledger::OnWalletPropertiesCallback callback) const override;

  void FetchGrants(const std::string& lang,
                   const std::string& paymentId,
                   const std::string& safetynet_token,
                   ledger::FetchGrantsCallback callback) const override;

  void OnGrants(ledger::Result result,
                const braveledger_bat_helper::Grants& grants,
                ledger::FetchGrantsCallback callback);

  void GetGrantCaptcha(
      const std::vector<std::string>& headers,
      ledger::GetGrantCaptchaCallback callback) const override;

  void SolveGrantCaptcha(const std::string& solution,
                         const std::string& promotionId) const override;

  void ApplySafetynetToken(const std::string& token) const override;

  void OnGrantFinish(ledger::Result result,
                     const braveledger_bat_helper::GRANT& grant);

  void GetGrantViaSafetynetCheck() const override;
  void OnGrantViaSafetynetCheck(const std::string& nonce);

  std::string GetWalletPassphrase() const override;

  void RecoverWallet(
      const std::string& pass_phrase,
      ledger::RecoverWalletCallback callback) override;

  void OnRecoverWallet(
      const ledger::Result result,
      double balance,
      std::vector<ledger::GrantPtr> grants,
      ledger::RecoverWalletCallback callback);

  void LoadURL(const std::string& url,
               const std::vector<std::string>& headers,
               const std::string& content,
               const std::string& contentType,
               const ledger::URL_METHOD method,
               ledger::LoadURLCallback callback);

  void OnReconcileComplete(ledger::Result result,
                           const std::string& viewing_id,
                           const std::string& probi,
                           const ledger::RewardsCategory category);

  std::string URIEncode(const std::string& value) override;

  void SaveMediaVisit(const std::string& publisher_id,
                      const ledger::VisitData& visit_data,
                      const uint64_t& duration,
                      const uint64_t window_id,
                      const ledger::PublisherInfoCallback callback);

  void SetPublisherExclude(
      const std::string& publisher_id,
      const ledger::PUBLISHER_EXCLUDE& exclude,
      ledger::SetPublisherExcludeCallback callback) override;

  void RestorePublishers(ledger::RestorePublishersCallback callback) override;

  void OnRestorePublishers(
    const ledger::Result result,
    ledger::RestorePublishersCallback callback);

  bool IsWalletCreated() const override;

  void GetPublisherActivityFromUrl(
      uint64_t windowId,
      ledger::VisitDataPtr visit_data,
      const std::string& publisher_blob) override;

  void GetMediaActivityFromUrl(
      uint64_t windowId,
      ledger::VisitDataPtr visit_data,
      const std::string& providerType,
      const std::string& publisher_blob);

  void OnPanelPublisherInfo(ledger::Result result,
                           ledger::PublisherInfoPtr info,
                           uint64_t windowId);

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

  void OnReconcileCompleteSuccess(const std::string& viewing_id,
                                  const ledger::RewardsCategory category,
                                  const std::string& probi,
                                  const ledger::ACTIVITY_MONTH month,
                                  const int year,
                                  const uint32_t date) override;

  void SaveRecurringTip(
      ledger::ContributionInfoPtr info,
      ledger::SaveRecurringTipCallback callback) override;

  void GetRecurringTips(ledger::PublisherInfoListCallback callback) override;

  void GetOneTimeTips(ledger::PublisherInfoListCallback callback) override;

  void RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::RemoveRecurringTipCallback callback) override;

  ledger::ActivityInfoFilterPtr CreateActivityFilter(
      const std::string& publisher_id,
      ledger::ExcludeFilter excluded,
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
                            const ledger::RewardsCategory category);

  void NormalizeContributeWinners(
      ledger::PublisherInfoList* newList,
      const ledger::PublisherInfoList* list,
      uint32_t /* next_record */);

  void SetTimer(uint64_t time_offset, uint32_t* timer_id) const;

  bool AddReconcileStep(const std::string& viewing_id,
                        ledger::ContributionRetry step,
                        int level = -1);

  const braveledger_bat_helper::CurrentReconciles& GetCurrentReconciles() const;

  double GetDefaultContributionAmount() override;

  void HasSufficientBalanceToReconcile(
      ledger::HasSufficientBalanceToReconcileCallback callback) override;

  void SaveNormalizedPublisherList(
      ledger::PublisherInfoList normalized_list);

  void SetCatalogIssuers(const std::string& info) override;
  void ConfirmAd(const std::string& info) override;
  void ConfirmAction(const std::string& uuid,
                     const std::string& creative_set_id,
                     const std::string& type) override;
  void GetTransactionHistory(
      ledger::GetTransactionHistoryCallback callback) override;

  std::unique_ptr<ledger::LogStream> Log(
      const char* file,
      const int line,
      const ledger::LogLevel log_level) const;

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();
  void GetRewardsInternalsInfo(
      ledger::RewardsInternalsInfoCallback callback) override;
  void StartMonthlyContribution() override;

  void RefreshPublisher(
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback) override;

  void SaveMediaInfo(const std::string& type,
                     const std::map<std::string, std::string>& data,
                     ledger::PublisherInfoCallback callback) override;

  void SetInlineTipSetting(const std::string& key, bool enabled) override;

  bool GetInlineTipSetting(const std::string& key) override;

  std::string GetShareURL(
      const std::string& type,
      const std::map<std::string, std::string>& args) override;

  void GetPendingContributions(
      ledger::PendingContributionInfoListCallback callback) override;

  void RemovePendingContribution(
      const std::string& publisher_key,
      const std::string& viewing_id,
      uint64_t added_date,
      ledger::RemovePendingContributionCallback callback) override;

  void RemoveAllPendingContributions(
      ledger::RemovePendingContributionCallback callback) override;

  void GetPendingContributionsTotal(
      ledger::PendingContributionsTotalCallback callback) override;

  void ContributeUnverifiedPublishers();

  void OnContributeUnverifiedPublishers(ledger::Result result,
                                        const std::string& publisher_key = "",
                                        const std::string& publisher_name = "");

  void SavePublisherProcessed(const std::string& publisher_key);

  bool WasPublisherAlreadyProcessed(const std::string& publisher_key) const;

  void FetchBalance(ledger::FetchBalanceCallback callback) override;

  void GetExternalWallets(ledger::GetExternalWalletsCallback callback);

  std::string GetCardIdAddress() const;

  void GetExternalWallet(const std::string& wallet_type,
                         ledger::ExternalWalletCallback callback) override;

  void SaveExternalWallet(const std::string& wallet_type,
                          ledger::ExternalWalletPtr wallet);

  void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback) override;

  void DisconnectWallet(
      const std::string& wallet_type,
      ledger::DisconnectWalletCallback callback) override;

  void TransferAnonToExternalWallet(
      ledger::ExternalWalletPtr wallet,
      const bool allow_zero_balance,
      ledger::TransferAnonToExternalWalletCallback callback);

  void ShowNotification(
      const std::string& type,
      ledger::ShowNotificationCallback callback,
      const std::vector<std::string>& args = {});

  void DeleteActivityInfo(
      const std::string& publisher_key,
      ledger::DeleteActivityInfoCallback callback);

  void ClearAndInsertServerPublisherList(
      ledger::ServerPublisherInfoList list,
      ledger::ClearAndInsertServerPublisherListCallback callback);

  void GetServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback);

  bool IsPublisherConnectedOrVerified(const ledger::PublisherStatus status);

  void SetBooleanState(const std::string& name, bool value);

  bool GetBooleanState(const std::string& name) const;

  void SetIntegerState(const std::string& name, int value);

  int GetIntegerState(const std::string& name) const;

  void SetDoubleState(const std::string& name, double value);

  double GetDoubleState(const std::string& name) const;

  void SetStringState(const std::string& name, const std::string& value);

  std::string GetStringState(const std::string& name) const;

  void SetInt64State(const std::string& name, int64_t value);

  int64_t GetInt64State(const std::string& name) const;

  void SetUint64State(const std::string& name, uint64_t value);

  uint64_t GetUint64State(const std::string& name) const;

  void ClearState(const std::string& name);

  void SetTransferFee(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee);

  ledger::TransferFeeList GetTransferFees(const std::string& wallet_type) const;

  void RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id);

 private:
  void OnLoad(ledger::VisitDataPtr visit_data,
              const uint64_t& current_time) override;

  void OnUnload(uint32_t tab_id, const uint64_t& current_time) override;

  void OnShow(uint32_t tab_id, const uint64_t& current_time) override;

  void OnSaveVisit(ledger::Result result,
                   ledger::PublisherInfoPtr info);

  void OnHide(uint32_t tab_id, const uint64_t& current_time) override;

  void OnForeground(uint32_t tab_id, const uint64_t& current_time) override;

  void OnBackground(uint32_t tab_id, const uint64_t& current_time) override;

  void OnXHRLoad(
      uint32_t tab_id,
      const std::string& url,
      const std::map<std::string, std::string>& parts,
      const std::string& first_party_url,
      const std::string& referrer,
      ledger::VisitDataPtr visit_data) override;

  void OnPostData(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      ledger::VisitDataPtr visit_data) override;

  void OnTimer(uint32_t timer_id) override;

  void saveVisitCallback(const std::string& publisher,
                         uint64_t verifiedTimestamp);

  void OnRemoveRecurringTip(
      const ledger::Result result,
      ledger::RemoveRecurringTipCallback callback);

  void OnGetPendingContributions(
    const ledger::PendingContributionInfoList& list,
    ledger::PendingContributionInfoListCallback callback);

  // ledger::LedgerCallbacHandler implementation
  void OnPublisherStateLoaded(ledger::Result result,
                              const std::string& data,
                              ledger::InitializeCallback callback) override;

  void OnLedgerStateLoaded(ledger::Result result,
                           const std::string& data,
                           ledger::InitializeCallback callback) override;

  void RefreshGrant(bool retryAfterError);

  uint64_t retryRequestSetup(uint64_t min_time, uint64_t max_time);

  void OnPublisherInfoSavedInternal(
      ledger::Result result,
      ledger::PublisherInfoPtr info);

  void DownloadPublisherList(
      ledger::LoadURLCallback callback);

  void OnRefreshPublisher(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback);

  ledger::LedgerClient* ledger_client_;
  std::unique_ptr<braveledger_grant::Grants> bat_grants_;
  std::unique_ptr<braveledger_publisher::Publisher> bat_publisher_;
  std::unique_ptr<braveledger_media::Media> bat_media_;
  std::unique_ptr<braveledger_bat_state::BatState> bat_state_;
  std::unique_ptr<braveledger_contribution::Contribution> bat_contribution_;
  std::unique_ptr<braveledger_wallet::Wallet> bat_wallet_;
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
