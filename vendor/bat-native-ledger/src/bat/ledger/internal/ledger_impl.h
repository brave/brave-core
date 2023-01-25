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

class LedgerImpl : public Ledger {
 public:
  explicit LedgerImpl(LedgerClient* client);
  ~LedgerImpl() override;

  LedgerImpl(const LedgerImpl&) = delete;
  LedgerImpl& operator=(const LedgerImpl&) = delete;

  LedgerClient* ledger_client() const;

  state::State* state() const;

  virtual promotion::Promotion* promotion() const;

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

  virtual void LoadURL(mojom::UrlRequestPtr request,
                       client::LegacyLoadURLCallback callback);

  virtual void LoadURL(mojom::UrlRequestPtr request,
                       client::LoadURLCallback callback);

  virtual void RunDBTransaction(
      mojom::DBTransactionPtr transaction,
      client::LegacyRunDBTransactionCallback callback);

  virtual void RunDBTransaction(mojom::DBTransactionPtr transaction,
                                client::RunDBTransactionCallback callback);

  bool IsShuttingDown() const;

  // Ledger Implementation

  void Initialize(bool execute_create_script,
                  LegacyResultCallback callback) override;

  void CreateRewardsWallet(const std::string& country,
                           CreateRewardsWalletCallback callback) override;

  void OneTimeTip(const std::string& publisher_key,
                  double amount,
                  LegacyResultCallback callback) override;

  void OnLoad(mojom::VisitDataPtr visit_data, uint64_t current_time) override;

  void OnUnload(uint32_t tab_id, uint64_t current_time) override;

  void OnShow(uint32_t tab_id, uint64_t current_time) override;

  void OnHide(uint32_t tab_id, uint64_t current_time) override;

  void OnForeground(uint32_t tab_id, uint64_t current_time) override;

  void OnBackground(uint32_t tab_id, uint64_t current_time) override;

  void OnXHRLoad(uint32_t tab_id,
                 const std::string& url,
                 const base::flat_map<std::string, std::string>& parts,
                 const std::string& first_party_url,
                 const std::string& referrer,
                 mojom::VisitDataPtr visit_data) override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           mojom::ActivityInfoFilterPtr filter,
                           PublisherInfoListCallback callback) override;

  void GetPublishersVisitedCount(
      base::OnceCallback<void(int)> callback) override;

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

  void GetBalanceReport(mojom::ActivityMonth month,
                        int year,
                        GetBalanceReportCallback callback) override;

  void GetAllBalanceReports(GetBalanceReportListCallback callback) override;

  mojom::AutoContributePropertiesPtr GetAutoContributeProperties() override;

  void SetPublisherExclude(const std::string& publisher_id,
                           mojom::PublisherExclude exclude,
                           ResultCallback callback) override;

  void RestorePublishers(ResultCallback callback) override;

  void GetPublisherActivityFromUrl(uint64_t window_id,
                                   mojom::VisitDataPtr visit_data,
                                   const std::string& publisher_blob) override;

  void GetPublisherBanner(const std::string& publisher_id,
                          PublisherBannerCallback callback) override;

  void RemoveRecurringTip(const std::string& publisher_key,
                          LegacyResultCallback callback) override;

  uint64_t GetCreationStamp() override;

  void GetRewardsInternalsInfo(RewardsInternalsInfoCallback callback) override;

  void SaveRecurringTip(mojom::RecurringTipPtr info,
                        LegacyResultCallback callback) override;

  void GetRecurringTips(PublisherInfoListCallback callback) override;

  void GetOneTimeTips(PublisherInfoListCallback callback) override;

  void RefreshPublisher(const std::string& publisher_key,
                        OnRefreshPublisherCallback callback) override;

  void StartMonthlyContribution() override;

  void UpdateMediaDuration(uint64_t window_id,
                           const std::string& publisher_key,
                           uint64_t duration,
                           bool first_visit) override;

  void IsPublisherRegistered(const std::string& publisher_id,
                             std::function<void(bool)> callback) override;

  void GetPublisherInfo(const std::string& publisher_key,
                        PublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(const std::string& publisher_key,
                             PublisherInfoCallback callback) override;

  void SavePublisherInfo(uint64_t window_id,
                         mojom::PublisherInfoPtr publisher_info,
                         LegacyResultCallback callback) override;

  void SetInlineTippingPlatformEnabled(mojom::InlineTipsPlatforms platform,
                                       bool enabled) override;

  bool GetInlineTippingPlatformEnabled(
      mojom::InlineTipsPlatforms platform) override;

  std::string GetShareURL(
      const base::flat_map<std::string, std::string>& args) override;

  void GetPendingContributions(
      PendingContributionInfoListCallback callback) override;

  void RemovePendingContribution(uint64_t id,
                                 LegacyResultCallback callback) override;

  void RemoveAllPendingContributions(LegacyResultCallback callback) override;

  void GetPendingContributionsTotal(
      PendingContributionsTotalCallback callback) override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         GetExternalWalletCallback) override;

  void ConnectExternalWallet(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ConnectExternalWalletCallback) override;

  void GetAllPromotions(GetAllPromotionsCallback callback) override;

  void GetTransactionReport(mojom::ActivityMonth month,
                            int year,
                            GetTransactionReportCallback callback) override;

  void GetContributionReport(mojom::ActivityMonth month,
                             int year,
                             GetContributionReportCallback callback) override;

  void GetAllContributions(ContributionInfoListCallback callback) override;

  void SavePublisherInfoForTip(mojom::PublisherInfoPtr info,
                               LegacyResultCallback callback) override;

  void GetMonthlyReport(mojom::ActivityMonth month,
                        int year,
                        GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(GetAllMonthlyReportIdsCallback callback) override;

  void ProcessSKU(const std::vector<mojom::SKUOrderItem>& items,
                  const std::string& wallet_type,
                  SKUOrderCallback callback) override;

  void Shutdown(LegacyResultCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;

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

  void OnInitialized(mojom::Result result, LegacyResultCallback callback);

  void StartServices();

  void OnStateInitialized(mojom::Result result, LegacyResultCallback callback);

  void InitializeDatabase(bool execute_create_script,
                          LegacyResultCallback callback);

  void OnDatabaseInitialized(mojom::Result result,
                             LegacyResultCallback callback);

  void OnAllDone(mojom::Result result, LegacyResultCallback callback);

  template <typename T>
  void WhenReady(T callback);

  template <typename LoadURLCallback>
  void LoadURLImpl(mojom::UrlRequestPtr request, LoadURLCallback callback);

  template <typename RunDBTransactionCallback>
  void RunDBTransactionImpl(mojom::DBTransactionPtr transaction,
                            RunDBTransactionCallback callback);

  LedgerClient* ledger_client_;

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

  std::map<uint32_t, mojom::VisitData> current_pages_;
  uint64_t last_tab_active_time_ = 0;
  uint32_t last_shown_tab_id_ = -1;
  std::queue<std::function<void()>> ready_callbacks_;
  ReadyState ready_state_ = ReadyState::kUninitialized;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_H_
