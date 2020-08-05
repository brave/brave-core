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
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "bat/ledger/internal/api/api.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/internal/media/media.h"
#include "bat/ledger/internal/promotion/promotion.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/report/report.h"
#include "bat/ledger/internal/sku/sku.h"
#include "bat/ledger/internal/state/state.h"
#include "bat/ledger/internal/wallet/wallet.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_client.h"

namespace base {
class SequencedTaskRunner;
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

  ledger::LedgerClient* ledger_client() const;

  braveledger_state::State* state() const;

  braveledger_promotion::Promotion* promotion() const;

  braveledger_publisher::Publisher* publisher() const;

  braveledger_media::Media* media() const;

  braveledger_contribution::Contribution* contribution() const;

  braveledger_wallet::Wallet* wallet() const;

  braveledger_report::Report* report() const;

  braveledger_sku::SKU* sku() const;

  braveledger_api::API* api() const;

  virtual braveledger_database::Database* database() const;

  void Initialize(
      const bool execute_create_script,
      ledger::ResultCallback callback) override;

  void CreateWallet(ledger::ResultCallback callback) override;

  void SavePublisherInfo(
      ledger::PublisherInfoPtr publisher_info,
      ledger::ResultCallback callback) override;

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

  void SetPublisherMinVisitTime(int duration_in_seconds) override;

  void SetPublisherMinVisits(int visits) override;

  void SetPublisherAllowNonVerified(bool allow) override;

  void SetPublisherAllowVideos(bool allow) override;

  void SetAutoContributionAmount(double amount) override;

  void SetAutoContributeEnabled(bool enabled) override;

  uint64_t GetReconcileStamp() override;

  bool GetRewardsMainEnabled() override;

  int GetPublisherMinVisitTime() override;  // In milliseconds

  int GetPublisherMinVisits() override;

  bool GetPublisherAllowNonVerified() override;

  bool GetPublisherAllowVideos() override;

  double GetAutoContributionAmount() override;

  bool GetAutoContributeEnabled() override;

  void GetBalanceReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetBalanceReportCallback callback) const override;

  void GetAllBalanceReports(
      ledger::GetBalanceReportListCallback callback) const override;

  ledger::AutoContributePropertiesPtr GetAutoContributeProperties() override;

  void GetRewardsParameters(
      ledger::GetRewardsParametersCallback callback) override;

  void FetchPromotions(ledger::FetchPromotionCallback callback) const override;

  void ClaimPromotion(
      const std::string& promotion_id,
      const std::string& payload,
      ledger::ClaimPromotionCallback callback) const override;

  void AttestPromotion(
      const std::string& promotion_id,
      const std::string& solution,
      ledger::AttestPromotionCallback callback) const override;

  std::string GetWalletPassphrase() const override;

  void RecoverWallet(
      const std::string& pass_phrase,
      ledger::ResultCallback callback) override;

  virtual void LoadURL(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ledger::UrlMethod method,
      ledger::LoadURLCallback callback);

  std::string URIEncode(const std::string& value) override;

  void SaveVideoVisit(
      const std::string& publisher_id,
      const ledger::VisitData& visit_data,
      uint64_t duration,
      uint64_t window_id,
      ledger::PublisherInfoCallback callback);

  void SetPublisherExclude(
      const std::string& publisher_id,
      const ledger::PublisherExclude& exclude,
      ledger::ResultCallback callback) override;

  void RestorePublishers(ledger::ResultCallback callback) override;

  bool IsWalletCreated() override;

  void GetPublisherActivityFromUrl(
      uint64_t windowId,
      ledger::VisitDataPtr visit_data,
      const std::string& publisher_blob) override;

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

  void ResetReconcileStamp();

  uint64_t GetCreationStamp() override;

  void SaveContributionInfo(
      ledger::ContributionInfoPtr info,
      ledger::ResultCallback callback);

  void HasSufficientBalanceToReconcile(
      ledger::HasSufficientBalanceToReconcileCallback callback) override;

  void GetRewardsInternalsInfo(
      ledger::RewardsInternalsInfoCallback callback) override;
  void StartMonthlyContribution() override;

  void RefreshPublisher(
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback) override;

  void SaveMediaInfo(const std::string& type,
                     const std::map<std::string, std::string>& data,
                     ledger::PublisherInfoCallback callback) override;

  void SetInlineTippingPlatformEnabled(
      const ledger::InlineTipsPlatforms platform,
      bool enabled) override;

  bool GetInlineTippingPlatformEnabled(
      const ledger::InlineTipsPlatforms platform) override;

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

  void FetchBalance(ledger::FetchBalanceCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         ledger::ExternalWalletCallback callback) override;

  void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback) override;

  void DisconnectWallet(
      const std::string& wallet_type,
      ledger::ResultCallback callback) override;

  void GetServerPublisherInfo(
      const std::string& publisher_key,
      ledger::GetServerPublisherInfoCallback callback);

  void GetAllPromotions(
    ledger::GetAllPromotionsCallback callback) override;

  void GetAnonWalletStatus(ledger::ResultCallback callback) override;

  void GetTransactionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback) override;

  void GetContributionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback) override;

  void GetAllContributions(
      ledger::ContributionInfoListCallback callback) override;

  void GetMonthlyReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(
      ledger::GetAllMonthlyReportIdsCallback callback) override;

  void ProcessSKU(
      const std::vector<ledger::SKUOrderItem>& items,
      ledger::ExternalWalletPtr wallet,
      ledger::SKUOrderCallback callback) override;

  void RetrySKU(
      const std::string& order_id,
      ledger::ExternalWalletPtr wallet,
      ledger::SKUOrderCallback callback);

  void Shutdown(ledger::ResultCallback callback) override;

 private:
  void OnInitialized(
      const ledger::Result result,
      ledger::ResultCallback callback);

  void StartServices();

  void OnStateInitialized(
      const ledger::Result result,
      ledger::ResultCallback callback);

  void InitializeDatabase(
      const bool execute_create_script,
      ledger::ResultCallback callback);

  void OnCreateWallet(
      const ledger::Result result,
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

  void saveVisitCallback(const std::string& publisher,
                         uint64_t verifiedTimestamp);

  void OnGetPendingContributions(
    const ledger::PendingContributionInfoList& list,
    ledger::PendingContributionInfoListCallback callback);

  void OnDatabaseInitialized(
      const ledger::Result result,
      ledger::ResultCallback callback);

  void OnServerPublisherInfoLoaded(
      ledger::ServerPublisherInfoPtr server_info,
      const std::string& publisher_key,
      ledger::GetServerPublisherInfoCallback callback);

  void RefreshPromotions(bool retryAfterError);

  void OnRefreshPublisher(
      const ledger::UrlResponse& response,
      const std::string& publisher_key,
      ledger::OnRefreshPublisherCallback callback);

  void ShutdownWallets(
      const ledger::Result result,
      ledger::ResultCallback callback);

  ledger::LedgerClient* ledger_client_;
  std::unique_ptr<braveledger_promotion::Promotion> promotion_;
  std::unique_ptr<braveledger_publisher::Publisher> publisher_;
  std::unique_ptr<braveledger_media::Media> media_;
  std::unique_ptr<braveledger_contribution::Contribution> contribution_;
  std::unique_ptr<braveledger_wallet::Wallet> wallet_;
  std::unique_ptr<braveledger_database::Database> database_;
  std::unique_ptr<braveledger_report::Report> report_;
  std::unique_ptr<braveledger_sku::SKU> sku_;
  std::unique_ptr<braveledger_state::State> state_;
  std::unique_ptr<braveledger_api::API> api_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  bool initialized_task_scheduler_;

  bool initializing_;
  bool shutting_down_ = false;

  std::map<uint32_t, ledger::VisitData> current_pages_;
  uint64_t last_tab_active_time_;
  uint32_t last_shown_tab_id_;
};

}  // namespace bat_ledger

#endif  // BAT_LEDGER_LEDGER_IMPL_H_
