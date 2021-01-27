/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_H_

#include <stdint.h>

#include <memory>
#include <map>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "bat/ledger/internal/api/api.h"
#include "bat/ledger/internal/bitflyer/bitflyer.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/database/database.h"
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

class LedgerImpl : public ledger::Ledger {
 public:
  typedef std::map<uint32_t,
      type::VisitData>::const_iterator visit_data_iter;

  explicit LedgerImpl(ledger::LedgerClient* client);
  ~LedgerImpl() override;

  // Not copyable, not assignable
  LedgerImpl(const LedgerImpl&) = delete;
  LedgerImpl& operator=(const LedgerImpl&) = delete;

  BATLedgerContext* context() const;

  ledger::LedgerClient* ledger_client() const;

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

  uphold::Uphold* uphold() const;

  virtual database::Database* database() const;

  virtual void LoadURL(
      type::UrlRequestPtr request,
      client::LoadURLCallback callback);

  bool IsShuttingDown() const;

 private:
  void OnInitialized(
      const type::Result result,
      ledger::ResultCallback callback);

  void StartServices();

  void OnStateInitialized(
      const type::Result result,
      ledger::ResultCallback callback);

  void InitializeDatabase(
      const bool execute_create_script,
      ledger::ResultCallback callback);

  void OnDatabaseInitialized(
      const type::Result result,
      ledger::ResultCallback callback);

  // ledger.h
  void Initialize(
      const bool execute_create_script,
      ledger::ResultCallback callback) override;

  void CreateWallet(ledger::ResultCallback callback) override;

  void OneTimeTip(
      const std::string& publisher_key,
      const double amount,
      ledger::ResultCallback callback) override;

  void OnLoad(
      type::VisitDataPtr visit_data,
      const uint64_t& current_time) override;

  void OnUnload(uint32_t tab_id, const uint64_t& current_time) override;

  void OnShow(uint32_t tab_id, const uint64_t& current_time) override;

  void OnHide(uint32_t tab_id, const uint64_t& current_time) override;

  void OnForeground(uint32_t tab_id, const uint64_t& current_time) override;

  void OnBackground(uint32_t tab_id, const uint64_t& current_time) override;

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

  std::string URIEncode(const std::string& value) override;

  void GetActivityInfoList(
      uint32_t start,
      uint32_t limit,
      type::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoListCallback callback) override;

  void GetExcludedList(ledger::PublisherInfoListCallback callback) override;

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

  void GetRewardsParameters(
      ledger::GetRewardsParametersCallback callback) override;

  void FetchPromotions(
      ledger::FetchPromotionCallback callback) const override;

  void ClaimPromotion(
      const std::string& promotion_id,
      const std::string& payload,
      ledger::ClaimPromotionCallback callback) const override;

  void AttestPromotion(
      const std::string& promotion_id,
      const std::string& solution,
      ledger::AttestPromotionCallback callback) const override;

  void GetBalanceReport(
      type::ActivityMonth month,
      int year,
      ledger::GetBalanceReportCallback callback) const override;

  void GetAllBalanceReports(
      ledger::GetBalanceReportListCallback callback) const override;

  type::AutoContributePropertiesPtr GetAutoContributeProperties() override;

  void RecoverWallet(
      const std::string& pass_phrase,
      ledger::ResultCallback callback)  override;

  void SetPublisherExclude(
      const std::string& publisher_id,
      const type::PublisherExclude& exclude,
      ledger::ResultCallback callback) override;

  void RestorePublishers(ledger::ResultCallback callback) override;

  void GetPublisherActivityFromUrl(
      uint64_t windowId,
      type::VisitDataPtr visit_data,
      const std::string& publisher_blob) override;

  void GetPublisherBanner(
      const std::string& publisher_id,
      ledger::PublisherBannerCallback callback) override;

  void RemoveRecurringTip(
      const std::string& publisher_key,
      ledger::ResultCallback callback) override;

  uint64_t GetCreationStamp() override;

  void HasSufficientBalanceToReconcile(
      ledger::HasSufficientBalanceToReconcileCallback callback) override;

  void GetRewardsInternalsInfo(
      ledger::RewardsInternalsInfoCallback callback) override;

  void SaveRecurringTip(
      type::RecurringTipPtr info,
      ledger::ResultCallback callback) override;

  void GetRecurringTips(ledger::PublisherInfoListCallback callback) override;

  void GetOneTimeTips(ledger::PublisherInfoListCallback callback) override;

  void RefreshPublisher(
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback) override;

  void StartMonthlyContribution() override;

  void SaveMediaInfo(
      const std::string& type,
      const base::flat_map<std::string, std::string>& data,
      ledger::PublisherInfoCallback callback) override;

  void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      const uint64_t duration,
      const bool first_visit) override;

  void GetPublisherInfo(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback) override;

  void SavePublisherInfo(
      const uint64_t window_id,
      type::PublisherInfoPtr publisher_info,
      ledger::ResultCallback callback) override;

  void SetInlineTippingPlatformEnabled(
      const type::InlineTipsPlatforms platform,
      bool enabled) override;

  bool GetInlineTippingPlatformEnabled(
      const type::InlineTipsPlatforms platform) override;

  std::string GetShareURL(
      const base::flat_map<std::string, std::string>& args) override;

  void GetPendingContributions(
      ledger::PendingContributionInfoListCallback callback) override;

  void RemovePendingContribution(
      const uint64_t id,
      ledger::ResultCallback callback) override;

  void RemoveAllPendingContributions(
      ledger::ResultCallback callback) override;

  void GetPendingContributionsTotal(
      ledger::PendingContributionsTotalCallback callback) override;

  void FetchBalance(ledger::FetchBalanceCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         ledger::ExternalWalletCallback callback) override;

  void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback) override;

  void DisconnectWallet(
      const std::string& wallet_type,
      ledger::ResultCallback callback) override;

  void GetAllPromotions(ledger::GetAllPromotionsCallback callback) override;

  void GetAnonWalletStatus(ledger::ResultCallback callback) override;

  void GetTransactionReport(
      const type::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback) override;

  void GetContributionReport(
      const type::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback) override;

  void GetAllContributions(
      ledger::ContributionInfoListCallback callback) override;

  void SavePublisherInfoForTip(
      type::PublisherInfoPtr info,
      ledger::ResultCallback callback) override;

  void GetMonthlyReport(
      const type::ActivityMonth month,
      const int year,
      ledger::GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(
      ledger::GetAllMonthlyReportIdsCallback callback) override;

  void ProcessSKU(
      const std::vector<type::SKUOrderItem>& items,
      const std::string& wallet_type,
      ledger::SKUOrderCallback callback) override;

  void Shutdown(ledger::ResultCallback callback) override;

  void GetEventLogs(ledger::GetEventLogsCallback callback) override;

  void GetBraveWallet(GetBraveWalletCallback callback) override;

  std::string GetWalletPassphrase() const override;

  void LinkBraveWallet(
      const std::string& destination_payment_id,
      ResultCallback callback) override;

  void GetTransferableAmount(GetTransferableAmountCallback callback) override;

  // end ledger.h

  void OnAllDone(const type::Result result, ledger::ResultCallback callback);

  ledger::LedgerClient* ledger_client_;
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
  std::unique_ptr<uphold::Uphold> uphold_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool initialized_task_scheduler_;

  bool initializing_;
  bool shutting_down_ = false;

  std::map<uint32_t, type::VisitData> current_pages_;
  uint64_t last_tab_active_time_;
  uint32_t last_shown_tab_id_;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LEDGER_IMPL_H_
