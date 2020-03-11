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
#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/internal/properties/wallet_info_properties.h"
#include "bat/ledger/internal/wallet/wallet.h"
#include "bat/ledger/ledger_client.h"
#include "bat/ledger/ledger.h"

namespace base {
class SequencedTaskRunner;
}

namespace braveledger_promotion {
class Promotion;
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

namespace braveledger_database {
class Database;
}

namespace braveledger_report {
class Report;
}

namespace braveledger_sku {
class SKU;
}

namespace confirmations {
class Confirmations;
}

namespace bat_ledger {

class LedgerImpl : public ledger::Ledger {
 public:
  typedef std::map<uint32_t,
      ledger::VisitData>::const_iterator visit_data_iter;

  explicit LedgerImpl(ledger::LedgerClient* client);
  ~LedgerImpl() override;

  // Not copyable, not assignable
  LedgerImpl(const LedgerImpl&) = delete;
  LedgerImpl& operator=(const LedgerImpl&) = delete;

  void Initialize(
      const bool execute_create_script,
      ledger::ResultCallback callback) override;

  void CreateWallet(ledger::ResultCallback callback) override;

  void SavePublisherInfo(
      ledger::PublisherInfoPtr publisher_info,
      ledger::ResultCallback callback) override;

  void SaveActivityInfo(
      ledger::PublisherInfoPtr publisher_info,
      ledger::ResultCallback callback);

  void GetPublisherInfo(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback);

  void GetActivityInfo(ledger::ActivityInfoFilterPtr filter,
                       ledger::PublisherInfoCallback callback);

  void GetPanelPublisherInfo(ledger::ActivityInfoFilterPtr filter,
                             ledger::PublisherInfoCallback callback);

  void GetMediaPublisherInfo(
      const std::string& media_key,
      ledger::PublisherInfoCallback callback);

  void SaveMediaPublisherInfo(
      const std::string& media_key,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           ledger::ActivityInfoFilterPtr filter,
                           ledger::PublisherInfoListCallback callback) override;

  void GetExcludedList(ledger::PublisherInfoListCallback callback) override;

  void OneTimeTip(
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback) override;

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

  void SavePendingContribution(
      ledger::PendingContributionList list,
      ledger::ResultCallback callback);

  void PendingContributionSaved(const ledger::Result result);

  uint64_t GetReconcileStamp() const override;

  bool GetRewardsMainEnabled() const override;

  uint64_t GetPublisherMinVisitTime() const override;  // In milliseconds

  unsigned int GetPublisherMinVisits() const override;

  bool GetPublisherAllowNonVerified() const override;

  bool GetPublisherAllowVideos() const override;

  double GetContributionAmount() const override;

  bool GetAutoContribute() const override;

  void GetBalanceReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetBalanceReportCallback callback) const override;

  std::map<std::string, ledger::BalanceReportInfoPtr>
  GetAllBalanceReports() const override;

  ledger::AutoContributePropsPtr GetAutoContributeProps() override;

  void SaveLedgerState(
      const std::string& data,
      ledger::ResultCallback callback);

  void SavePublisherState(
      const std::string& data,
      ledger::ResultCallback callback);

  void LoadNicewareList(ledger::GetNicewareListCallback callback);

  void SetConfirmationsWalletInfo(
      const ledger::WalletInfoProperties& wallet_info_properties);

  void LoadLedgerState(ledger::OnLoadCallback callback);

  void LoadPublisherState(ledger::OnLoadCallback callback);

  void OnWalletInitializedInternal(ledger::Result result,
                                   ledger::ResultCallback callback);

  void OnWalletProperties(ledger::Result result,
                          const ledger::WalletProperties&);

  void FetchWalletProperties(
      ledger::OnWalletPropertiesCallback callback) const override;

  void FetchPromotions(ledger::FetchPromotionCallback callback) const override;

  void ClaimPromotion(
      const std::string& payload,
      ledger::ClaimPromotionCallback callback) const override;

  void AttestPromotion(
      const std::string& promotion_id,
      const std::string& solution,
      ledger::AttestPromotionCallback callback) const override;

  std::string GetWalletPassphrase() const override;

  void RecoverWallet(
      const std::string& pass_phrase,
      ledger::RecoverWalletCallback callback) override;

  void OnRecoverWallet(
      const ledger::Result result,
      double balance,
      ledger::RecoverWalletCallback callback);

  virtual void LoadURL(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ledger::UrlMethod method,
      ledger::LoadURLCallback callback);

  virtual void ContributionCompleted(
      const ledger::Result result,
      const double amount,
      const std::string& contribution_id,
      const ledger::RewardsType type);

  std::string URIEncode(const std::string& value) override;

  void SaveMediaVisit(const std::string& publisher_id,
                      const ledger::VisitData& visit_data,
                      const uint64_t& duration,
                      const uint64_t window_id,
                      const ledger::PublisherInfoCallback callback);

  void SetPublisherExclude(
      const std::string& publisher_id,
      const ledger::PublisherExclude& exclude,
      ledger::ResultCallback callback) override;

  void RestorePublishers(ledger::ResultCallback callback) override;

  void LogRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ledger::UrlMethod method);

  void OnRestorePublishers(
    const ledger::Result result,
    ledger::ResultCallback callback);

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

  void SetBalanceReportItem(
      const ledger::ActivityMonth month,
      const int year,
      const ledger::ReportType type,
      const double amount);

  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::FetchIconCallback callback);

  void GetPublisherBanner(const std::string& publisher_id,
                          ledger::PublisherBannerCallback callback) override;

  void SaveRecurringTip(
      ledger::RecurringTipPtr info,
      ledger::ResultCallback callback) override;

  void GetRecurringTips(ledger::PublisherInfoListCallback callback) override;

  void GetOneTimeTips(ledger::PublisherInfoListCallback callback) override;

  void RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::ResultCallback callback) override;

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

  virtual const std::string& GetPaymentId() const;

  const std::string& GetPersonaId() const;

  void SetPersonaId(const std::string& persona_id);

  const std::string& GetUserId() const;

  void SetUserId(const std::string& user_id);

  const std::string& GetRegistrarVK() const;

  void SetRegistrarVK(const std::string& registrar_vk);

  const std::string& GetPreFlight() const;

  void SetPreFlight(const std::string& pre_flight);

  const ledger::WalletInfoProperties& GetWalletInfo() const;

  void SetWalletInfo(const ledger::WalletInfoProperties& info);

  const ledger::WalletProperties& GetWalletProperties() const;

  void SetWalletProperties(
      ledger::WalletProperties* properties);

  uint64_t GetBootStamp() const override;

  void SetBootStamp(uint64_t stamp);

  void SaveContributionInfo(
      ledger::ContributionInfoPtr info,
      ledger::ResultCallback callback);

  void NormalizeContributeWinners(
      ledger::PublisherInfoList* newList,
      const ledger::PublisherInfoList* list,
      uint32_t /* next_record */);

  void SetTimer(uint64_t time_offset, uint32_t* timer_id) const;

  double GetDefaultContributionAmount() override;

  void HasSufficientBalanceToReconcile(
      ledger::HasSufficientBalanceToReconcileCallback callback) override;

  void SaveNormalizedPublisherList(ledger::PublisherInfoList list);

  void SetCatalogIssuers(
      const std::string& info) override;

  void ConfirmAd(
      const std::string& json,
      const std::string& confirmation_type) override;
  void ConfirmAction(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const std::string& confirmation_type) override;

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
      const uint64_t id,
      ledger::ResultCallback callback) override;

  void RemoveAllPendingContributions(
      ledger::ResultCallback callback) override;

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
      ledger::ResultCallback callback) override;

  void TransferAnonToExternalWallet(
      ledger::ExternalWalletPtr wallet,
      ledger::ResultCallback callback,
      const bool allow_zero_balance = false);

  void ShowNotification(
      const std::string& type,
      ledger::ResultCallback callback,
      const std::vector<std::string>& args = {});

  void DeleteActivityInfo(
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void ClearServerPublisherList(ledger::ResultCallback callback);

  void InsertServerPublisherList(
      const std::vector<ledger::ServerPublisherPartial>& list,
      ledger::ResultCallback callback);

  void InsertPublisherBannerList(
      const std::vector<ledger::PublisherBanner>& list,
      ledger::ResultCallback callback);

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

  bool GetBooleanOption(const std::string& name) const;

  int GetIntegerOption(const std::string& name) const;

  double GetDoubleOption(const std::string& name) const;

  std::string GetStringOption(const std::string& name) const;

  int64_t GetInt64Option(const std::string& name) const;

  uint64_t GetUint64Option(const std::string& name) const;

  void SetTransferFee(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee);

  ledger::TransferFeeList GetTransferFees(const std::string& wallet_type) const;

  void RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id);

  void SaveContributionQueue(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback);

  void DeleteContributionQueue(
    const uint64_t id,
    ledger::ResultCallback callback);

  void GetFirstContributionQueue(
    ledger::GetFirstContributionQueueCallback callback);

  virtual void SavePromotion(
    ledger::PromotionPtr info,
    ledger::ResultCallback callback);

  void GetPromotion(
    const std::string& id,
    ledger::GetPromotionCallback callback);

  void GetAllPromotions(
    ledger::GetAllPromotionsCallback callback) override;

  void DeletePromotionList(
      const std::vector<std::string>& id_list,
      ledger::ResultCallback callback);

  void SaveUnblindedTokenList(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback);

  virtual void GetAllUnblindedTokens(
      ledger::GetUnblindedTokenListCallback callback);

  virtual void DeleteUnblindedTokens(
      const std::vector<std::string>& id_list,
      ledger::ResultCallback callback);

  void GetUnblindedTokensByTriggerIds(
      const std::vector<std::string>& trigger_ids,
      ledger::GetUnblindedTokenListCallback callback);

  ledger::ClientInfoPtr GetClientInfo();

  void UnblindedTokensReady();

  void GetAnonWalletStatus(ledger::ResultCallback callback) override;

  void GetTransactionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback) override;

  void GetContributionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback) override;

  void GetIncompleteContributions(
      const ledger::ContributionProcessor processor,
      ledger::ContributionInfoListCallback callback);

  virtual void GetContributionInfo(
      const std::string& contribution_id,
      ledger::GetContributionInfoCallback callback);

  virtual void UpdateContributionInfoStepAndCount(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count,
      ledger::ResultCallback callback);

  void UpdateContributionInfoContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  virtual void RunDBTransaction(
      ledger::DBTransactionPtr transaction,
      ledger::RunDBTransactionCallback callback);

  void GetCreateScript(ledger::GetCreateScriptCallback callback);

  void GetAllContributions(
      ledger::ContributionInfoListCallback callback) override;

  void GetMonthlyReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(
      ledger::GetAllMonthlyReportIdsCallback callback) override;

  void TransferTokens(
      ledger::ExternalWalletPtr wallet,
      ledger::ResultCallback callback);

  void SaveCredsBatch(
      ledger::CredsBatchPtr info,
      ledger::ResultCallback callback);

  void SavePromotionClaimId(
      const std::string& promotion_id,
      const std::string& claim_id,
      ledger::ResultCallback callback);

  void GetCredsBatchByTrigger(
      const std::string& trigger_id,
      const ledger::CredsBatchType trigger_type,
      ledger::GetCredsBatchCallback callback);

  void SaveSignedCreds(
      ledger::CredsBatchPtr info,
      ledger::ResultCallback callback);

  void UpdatePromotionStatus(
      const std::string& promotion_id,
      const ledger::PromotionStatus status,
      ledger::ResultCallback callback);

  void PromotionCredentialCompleted(
      const std::string& promotion_id,
      ledger::ResultCallback callback);

  void GetAllCredsBatches(ledger::GetAllCredsBatchCallback callback);

  void GetPromotionList(
      const std::vector<std::string>& ids,
      ledger::GetPromotionListCallback callback);

  void GetPromotionListByType(
      const std::vector<ledger::PromotionType>& types,
      ledger::GetPromotionListCallback callback);

  void CheckUnblindedTokensExpiration(ledger::ResultCallback callback);

  void UpdateCredsBatchStatus(
      const std::string& trigger_id,
      const ledger::CredsBatchType trigger_type,
      const ledger::CredsBatchStatus status,
      ledger::ResultCallback callback);

  void SaveSKUOrder(ledger::SKUOrderPtr order, ledger::ResultCallback callback);

  void SaveSKUTransaction(
      ledger::SKUTransactionPtr transaction,
      ledger::ResultCallback callback);

  void SaveSKUExternalTransaction(
      const std::string& transaction_id,
      const std::string& external_transaction_id,
      ledger::ResultCallback callback);

  void UpdateSKUOrderStatus(
      const std::string& order_id,
      const ledger::SKUOrderStatus status,
      ledger::ResultCallback callback);

  void TransferFunds(
      const ledger::SKUTransaction& transaction,
      const std::string& destination,
      ledger::ExternalWalletPtr wallet,
      ledger::TransactionCallback callback);

  void GetSKUOrder(
      const std::string& order_id,
      ledger::GetSKUOrderCallback callback);

  void BraveSKU(
      const std::string& destination,
      const std::vector<ledger::SKUOrderItem>& items,
      const std::string& contribution_id,
      ledger::ExternalWalletPtr wallet,
      ledger::SKUOrderCallback callback);

 private:
  void InitializeConfirmations(
      const bool execute_create_script,
      ledger::ResultCallback callback);

  void OnConfirmationsInitialized(
      const bool success,
      const bool execute_create_script,
      ledger::ResultCallback callback);

  void OnLoad(ledger::VisitDataPtr visit_data,
              const uint64_t& current_time) override;

  void OnUnload(uint32_t tab_id, const uint64_t& current_time) override;

  void OnShow(uint32_t tab_id, const uint64_t& current_time) override;

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

  void OnGetPendingContributions(
    const ledger::PendingContributionInfoList& list,
    ledger::PendingContributionInfoListCallback callback);

  void OnDatabaseInitialized(
      const ledger::Result result,
      ledger::ResultCallback callback);

  void OnPublisherStateLoaded(
      ledger::Result result,
      const std::string& data,
      ledger::ResultCallback callback);

  void OnLedgerStateLoaded(
      ledger::Result result,
      const std::string& data,
      ledger::ResultCallback callback);

  void RefreshPromotions(bool retryAfterError);

  void DownloadPublisherList(
      ledger::LoadURLCallback callback);

  void OnRefreshPublisher(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback);

  void OnGetActivityInfo(
      ledger::PublisherInfoList list,
      ledger::PublisherInfoCallback callback,
      const std::string& publisher_key);

  ledger::LedgerClient* ledger_client_;
  std::unique_ptr<braveledger_promotion::Promotion> bat_promotion_;
  std::unique_ptr<braveledger_publisher::Publisher> bat_publisher_;
  std::unique_ptr<braveledger_media::Media> bat_media_;
  std::unique_ptr<braveledger_bat_state::BatState> bat_state_;
  std::unique_ptr<braveledger_contribution::Contribution> bat_contribution_;
  std::unique_ptr<braveledger_wallet::Wallet> bat_wallet_;
  std::unique_ptr<braveledger_database::Database> bat_database_;
  std::unique_ptr<confirmations::Confirmations> bat_confirmations_;
  std::unique_ptr<braveledger_report::Report> bat_report_;
  std::unique_ptr<braveledger_sku::SKU> bat_sku_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool initialized_task_scheduler_;

  bool initialized_;
  bool initializing_;

  std::map<uint32_t, ledger::VisitData> current_pages_;
  uint64_t last_tab_active_time_;
  uint32_t last_shown_tab_id_;
  uint32_t last_pub_load_timer_id_;
};

}  // namespace bat_ledger

#endif  // BAT_LEDGER_LEDGER_IMPL_H_
