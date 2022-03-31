/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "bat/ledger/internal/api/api.h"
#include "bat/ledger/internal/bitflyer/bitflyer.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/gemini/gemini.h"
#include "bat/ledger/internal/legacy/media/media.h"
#include "bat/ledger/internal/logging/logging.h"
#include "bat/ledger/internal/promotion/promotion.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/recovery/recovery.h"
#include "bat/ledger/internal/report/report.h"
#include "bat/ledger/internal/sku/sku.h"
#include "bat/ledger/internal/state/state.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/wallet/wallet.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_client.h"

namespace base {
class SequencedTaskRunner;
}

namespace ledger {

class BATLedgerContext;

class LedgerImpl : public Ledger {
 public:
  explicit LedgerImpl(LedgerClient* client);
  ~LedgerImpl() override;

  LedgerImpl(const LedgerImpl&) = delete;
  LedgerImpl& operator=(const LedgerImpl&) = delete;

  BATLedgerContext* context() const;

  LedgerClient* ledger_client() const;

  state::State* state() const;

  promotion::Promotion* promotion() const;

  publisher::Publisher* publisher() const;

  braveledger_media::Media* media() const;

  contribution::Contribution* contribution() const;

  wallet::Wallet* wallet() const;

  report::Report* report() const;

  sku::SKU* sku() const;

  api::API* api() const;

  bitflyer::Bitflyer* bitflyer() const;

  gemini::Gemini* gemini() const;

  uphold::Uphold* uphold() const;

  virtual database::Database* database() const;

  virtual void LoadURL(type::UrlRequestPtr request,
                       client::LoadURLCallback callback);

  bool IsShuttingDown() const;

  // Ledger Implementation

  void Initialize(bool execute_create_script, ResultCallback callback) override;

  void CreateWallet(ResultCallback callback) override;

  void OneTimeTip(const std::string& publisher_key,
                  double amount,
                  ResultCallback callback) override;

  void OnLoad(type::VisitDataPtr visit_data, uint64_t current_time) override;

  void OnUnload(uint32_t tab_id, uint64_t current_time) override;

  void OnShow(uint32_t tab_id, uint64_t current_time) override;

  void OnHide(uint32_t tab_id, uint64_t current_time) override;

  void OnForeground(uint32_t tab_id, uint64_t current_time) override;

  void OnBackground(uint32_t tab_id, uint64_t current_time) override;

  void OnXHRLoad(
      uint32_t tab_id,
      const std::string& url,
      const base::flat_map<std::string, std::string>& parts,
      const std::string& first_party_url,
      const std::string& referrer,
      type::VisitDataPtr visit_data) override;


  void OnPostData(
      const std::string& url,
      const std::string& first_party_url,
      const std::string& referrer,
      const std::string& post_data,
      type::VisitDataPtr visit_data) override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           type::ActivityInfoFilterPtr filter,
                           PublisherInfoListCallback callback) override;

  void GetExcludedList(PublisherInfoListCallback callback) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) override;

  void SetPublisherMinVisits(int visits) override;

  void SetPublisherAllowNonVerified(bool allow) override;

  void SetPublisherAllowVideos(bool allow) override;

  void SetAutoContributionAmount(double amount) override;

  void SetAutoContributeEnabled(bool enabled) override;

  uint64_t GetReconcileStamp() override;

  int GetPublisherMinVisitTime() override;

  int GetPublisherMinVisits() override;

  bool GetPublisherAllowNonVerified() override;

  bool GetPublisherAllowVideos() override;

  double GetAutoContributionAmount() override;

  bool GetAutoContributeEnabled() override;

  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void FetchPromotions(FetchPromotionCallback callback) override;

  void ClaimPromotion(const std::string& promotion_id,
                      const std::string& payload,
                      ClaimPromotionCallback callback) override;

  void AttestPromotion(const std::string& promotion_id,
                       const std::string& solution,
                       AttestPromotionCallback callback) override;

  void GetBalanceReport(type::ActivityMonth month,
                        int year,
                        GetBalanceReportCallback callback) override;

  void GetAllBalanceReports(GetBalanceReportListCallback callback) override;

  type::AutoContributePropertiesPtr GetAutoContributeProperties() override;

  void RecoverWallet(const std::string& pass_phrase,
                     ResultCallback callback) override;

  void SetPublisherExclude(const std::string& publisher_id,
                           type::PublisherExclude exclude,
                           ResultCallback callback) override;

  void RestorePublishers(ResultCallback callback) override;

  void GetPublisherActivityFromUrl(uint64_t window_id,
                                   type::VisitDataPtr visit_data,
                                   const std::string& publisher_blob) override;

  void GetPublisherBanner(const std::string& publisher_id,
                          PublisherBannerCallback callback) override;

  void RemoveRecurringTip(const std::string& publisher_key,
                          ResultCallback callback) override;

  uint64_t GetCreationStamp() override;

  void HasSufficientBalanceToReconcile(
      HasSufficientBalanceToReconcileCallback callback) override;

  void GetRewardsInternalsInfo(RewardsInternalsInfoCallback callback) override;

  void SaveRecurringTip(type::RecurringTipPtr info,
                        ResultCallback callback) override;

  void GetRecurringTips(PublisherInfoListCallback callback) override;

  void GetOneTimeTips(PublisherInfoListCallback callback) override;

  void RefreshPublisher(const std::string& publisher_key,
                        OnRefreshPublisherCallback callback) override;

  void StartMonthlyContribution() override;

  void SaveMediaInfo(const std::string& type,
                     const base::flat_map<std::string, std::string>& data,
                     PublisherInfoCallback callback) override;

  void UpdateMediaDuration(uint64_t window_id,
                           const std::string& publisher_key,
                           uint64_t duration,
                           bool first_visit) override;

  void GetPublisherInfo(const std::string& publisher_key,
                        PublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(const std::string& publisher_key,
                             PublisherInfoCallback callback) override;

  void SavePublisherInfo(uint64_t window_id,
                         type::PublisherInfoPtr publisher_info,
                         ResultCallback callback) override;

  void SetInlineTippingPlatformEnabled(type::InlineTipsPlatforms platform,
                                       bool enabled) override;

  bool GetInlineTippingPlatformEnabled(
      type::InlineTipsPlatforms platform) override;

  std::string GetShareURL(
      const base::flat_map<std::string, std::string>& args) override;

  void GetPendingContributions(
      PendingContributionInfoListCallback callback) override;

  void RemovePendingContribution(uint64_t id, ResultCallback callback) override;

  void RemoveAllPendingContributions(ResultCallback callback) override;

  void GetPendingContributionsTotal(
      PendingContributionsTotalCallback callback) override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         ExternalWalletCallback callback) override;

  void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ExternalWalletAuthorizationCallback callback) override;

  void DisconnectWallet(const std::string& wallet_type,
                        ResultCallback callback) override;

  void GetAllPromotions(GetAllPromotionsCallback callback) override;

  void GetAnonWalletStatus(ResultCallback callback) override;

  void GetTransactionReport(type::ActivityMonth month,
                            int year,
                            GetTransactionReportCallback callback) override;

  void GetContributionReport(type::ActivityMonth month,
                             int year,
                             GetContributionReportCallback callback) override;

  void GetAllContributions(ContributionInfoListCallback callback) override;

  void SavePublisherInfoForTip(type::PublisherInfoPtr info,
                               ResultCallback callback) override;

  void GetMonthlyReport(type::ActivityMonth month,
                        int year,
                        GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(GetAllMonthlyReportIdsCallback callback) override;

  void ProcessSKU(const std::vector<type::SKUOrderItem>& items,
                  const std::string& wallet_type,
                  SKUOrderCallback callback) override;

  void Shutdown(ResultCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void GetBraveWallet(GetBraveWalletCallback callback) override;

  std::string GetWalletPassphrase() override;

  void LinkBraveWallet(const std::string& destination_payment_id,
                       PostSuggestionsClaimCallback callback) override;

  void GetTransferableAmount(GetTransferableAmountCallback callback) override;

  void GetDrainStatus(const std::string& drain_id,
                      GetDrainCallback callback) override;

  void SetInitializedForTesting();

 private:
  enum class ReadyState {
    kUninitialized,
    kInitializing,
    kReady,
    kShuttingDown
  };

  bool IsReady() const;

  void OnInitialized(type::Result result, ResultCallback callback);

  void StartServices();

  void OnStateInitialized(type::Result result, ResultCallback callback);

  void InitializeDatabase(bool execute_create_script, ResultCallback callback);

  void OnDatabaseInitialized(type::Result result, ResultCallback callback);

  void OnAllDone(type::Result result, ResultCallback callback);

  template <typename T>
  void WhenReady(T callback);

  LedgerClient* ledger_client_;

  std::unique_ptr<BATLedgerContext> context_;
  std::unique_ptr<promotion::Promotion> promotion_;
  std::unique_ptr<publisher::Publisher> publisher_;
  std::unique_ptr<braveledger_media::Media> media_;
  std::unique_ptr<contribution::Contribution> contribution_;
  std::unique_ptr<wallet::Wallet> wallet_;
  std::unique_ptr<database::Database> database_;
  std::unique_ptr<report::Report> report_;
  std::unique_ptr<sku::SKU> sku_;
  std::unique_ptr<state::State> state_;
  std::unique_ptr<api::API> api_;
  std::unique_ptr<recovery::Recovery> recovery_;
  std::unique_ptr<bitflyer::Bitflyer> bitflyer_;
  std::unique_ptr<gemini::Gemini> gemini_;
  std::unique_ptr<uphold::Uphold> uphold_;

  std::map<uint32_t, type::VisitData> current_pages_;
  uint64_t last_tab_active_time_ = 0;
  uint32_t last_shown_tab_id_ = -1;
  std::queue<std::function<void()>> ready_callbacks_;
  ReadyState ready_state_ = ReadyState::kUninitialized;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_H_
