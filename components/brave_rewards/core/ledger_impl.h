/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_H_

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/common/mojom/bat_ledger.mojom.h"
#include "brave/components/brave_rewards/core/api/api.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/legacy/media/media.h"
#include "brave/components/brave_rewards/core/logging/logging.h"  // does not belong in here, components should include it themselves
#include "brave/components/brave_rewards/core/promotion/promotion.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/recovery/recovery.h"
#include "brave/components/brave_rewards/core/report/report.h"
#include "brave/components/brave_rewards/core/sku/sku.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

#include "brave/components/brave_rewards/core/ledger.h"  // TODO(sszaloki)

namespace ledger {

class LedgerImpl : public mojom::Ledger {
 public:
  explicit LedgerImpl(
      mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
      mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote);

  ~LedgerImpl() override;

  LedgerImpl(const LedgerImpl&) = delete;

  LedgerImpl& operator=(const LedgerImpl&) = delete;

  // mojom::Ledger implementation begin
  void Initialize(bool execute_create_script,
                  InitializeCallback callback) override;

  void SetEnvironment(ledger::mojom::Environment environment) override;

  void SetDebug(bool debug) override;

  void SetReconcileInterval(int32_t interval) override;

  void SetRetryInterval(int32_t interval) override;

  void SetTesting() override;

  void SetStateMigrationTargetVersionForTesting(int32_t version) override;

  void GetEnvironment(GetEnvironmentCallback callback) override;

  void GetDebug(GetDebugCallback callback) override;

  void GetReconcileInterval(GetReconcileIntervalCallback callback) override;

  void GetRetryInterval(GetRetryIntervalCallback callback) override;

  void CreateRewardsWallet(const std::string& country,
                           CreateRewardsWalletCallback callback) override;

  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void GetAutoContributeProperties(
      GetAutoContributePropertiesCallback callback) override;

  void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) override;

  void GetPublisherMinVisits(GetPublisherMinVisitsCallback callback) override;

  void GetPublisherAllowNonVerified(
      GetPublisherAllowNonVerifiedCallback callback) override;

  void GetAutoContributeEnabled(
      GetAutoContributeEnabledCallback callback) override;

  void GetReconcileStamp(GetReconcileStampCallback callback) override;

  void OnLoad(ledger::mojom::VisitDataPtr visit_data,
              uint64_t current_time) override;

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
                 ledger::mojom::VisitDataPtr visit_data) override;

  void SetPublisherExclude(const std::string& publisher_key,
                           ledger::mojom::PublisherExclude exclude,
                           SetPublisherExcludeCallback callback) override;

  void RestorePublishers(RestorePublishersCallback callback) override;

  void FetchPromotions(FetchPromotionsCallback callback) override;

  void ClaimPromotion(const std::string& promotion_id,
                      const std::string& payload,
                      ClaimPromotionCallback callback) override;

  void AttestPromotion(const std::string& promotion_id,
                       const std::string& solution,
                       AttestPromotionCallback callback) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) override;

  void SetPublisherMinVisits(int visits) override;

  void SetPublisherAllowNonVerified(bool allow) override;

  void SetAutoContributionAmount(double amount) override;

  void SetAutoContributeEnabled(bool enabled) override;

  void GetBalanceReport(ledger::mojom::ActivityMonth month,
                        int32_t year,
                        GetBalanceReportCallback callback) override;

  void GetPublisherActivityFromUrl(uint64_t window_id,
                                   ledger::mojom::VisitDataPtr visit_data,
                                   const std::string& publisher_blob) override;

  void GetAutoContributionAmount(
      GetAutoContributionAmountCallback callback) override;

  void GetPublisherBanner(const std::string& publisher_id,
                          GetPublisherBannerCallback callback) override;

  void OneTimeTip(const std::string& publisher_key,
                  double amount,
                  OneTimeTipCallback callback) override;

  void RemoveRecurringTip(const std::string& publisher_key,
                          RemoveRecurringTipCallback callback) override;

  void GetCreationStamp(GetCreationStampCallback callback) override;

  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;

  void SaveRecurringTip(ledger::mojom::RecurringTipPtr info,
                        SaveRecurringTipCallback callback) override;

  void SendContribution(const std::string& publisher_id,
                        double amount,
                        bool set_monthly,
                        SendContributionCallback callback) override;

  void GetRecurringTips(GetRecurringTipsCallback callback) override;

  void GetOneTimeTips(GetOneTimeTipsCallback callback) override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           ledger::mojom::ActivityInfoFilterPtr filter,
                           GetActivityInfoListCallback callback) override;

  void GetPublishersVisitedCount(
      GetPublishersVisitedCountCallback callback) override;

  void GetExcludedList(GetExcludedListCallback callback) override;

  void RefreshPublisher(const std::string& publisher_key,
                        RefreshPublisherCallback callback) override;

  void StartContributionsForTesting() override;

  void UpdateMediaDuration(uint64_t window_id,
                           const std::string& publisher_key,
                           uint64_t duration,
                           bool first_visit) override;

  void IsPublisherRegistered(const std::string& publisher_id,
                             IsPublisherRegisteredCallback callback) override;

  void GetPublisherInfo(const std::string& publisher_key,
                        GetPublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(const std::string& publisher_key,
                             GetPublisherPanelInfoCallback callback) override;

  void SavePublisherInfo(uint64_t window_id,
                         ledger::mojom::PublisherInfoPtr publisher_info,
                         SavePublisherInfoCallback callback) override;

  void SetInlineTippingPlatformEnabled(
      ledger::mojom::InlineTipsPlatforms platform,
      bool enabled) override;

  void GetInlineTippingPlatformEnabled(
      ledger::mojom::InlineTipsPlatforms platform,
      GetInlineTippingPlatformEnabledCallback callback) override;

  void GetShareURL(const base::flat_map<std::string, std::string>& args,
                   GetShareURLCallback callback) override;

  void GetPendingContributions(
      GetPendingContributionsCallback callback) override;

  void RemovePendingContribution(
      uint64_t id,
      RemovePendingContributionCallback callback) override;

  void RemoveAllPendingContributions(
      RemovePendingContributionCallback callback) override;

  void GetPendingContributionsTotal(
      GetPendingContributionsTotalCallback callback) override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         GetExternalWalletCallback) override;

  void ConnectExternalWallet(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ConnectExternalWalletCallback) override;

  void GetTransactionReport(ledger::mojom::ActivityMonth month,
                            int year,
                            GetTransactionReportCallback callback) override;

  void GetContributionReport(ledger::mojom::ActivityMonth month,
                             int year,
                             GetContributionReportCallback callback) override;

  void GetAllContributions(GetAllContributionsCallback callback) override;

  void SavePublisherInfoForTip(
      ledger::mojom::PublisherInfoPtr info,
      SavePublisherInfoForTipCallback callback) override;

  void GetMonthlyReport(ledger::mojom::ActivityMonth month,
                        int year,
                        GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(GetAllMonthlyReportIdsCallback callback) override;

  void GetAllPromotions(GetAllPromotionsCallback callback) override;

  void Shutdown(ShutdownCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;

  // mojom::Ledger implementation end

  mojom::LedgerClient* client() const;

  ledger::state::State* state() const;

  virtual ledger::promotion::Promotion* promotion() const;

  ledger::publisher::Publisher* publisher() const;

  braveledger_media::Media* media() const;

  ledger::contribution::Contribution* contribution() const;

  ledger::wallet::Wallet* wallet() const;

  ledger::report::Report* report() const;

  ledger::sku::SKU* sku() const;

  ledger::api::API* api() const;

  virtual ledger::database::Database* database() const;

  ledger::bitflyer::Bitflyer* bitflyer() const;

  ledger::gemini::Gemini* gemini() const;

  ledger::uphold::Uphold* uphold() const;

  // mojom::LedgerClient async helpers begin
  template <typename LoadURLCallback>
  void LoadURLImpl(ledger::mojom::UrlRequestPtr request,
                   LoadURLCallback callback);

  void LoadURL(ledger::mojom::UrlRequestPtr request,
               ledger::LegacyLoadURLCallback callback);

  void LoadURL(ledger::mojom::UrlRequestPtr request,
               ledger::LoadURLCallback callback);

  template <typename RunDBTransactionCallback>
  void RunDBTransactionImpl(ledger::mojom::DBTransactionPtr transaction,
                            RunDBTransactionCallback callback);

  void RunDBTransaction(ledger::mojom::DBTransactionPtr transaction,
                        ledger::LegacyRunDBTransactionCallback callback);

  void RunDBTransaction(ledger::mojom::DBTransactionPtr transaction,
                        ledger::RunDBTransactionCallback callback);
  // mojom::LedgerClient async helpers end

  // mojom::LedgerClient sync helpers begin
  std::string URIEncode(const std::string& value) {
    std::string encoded_value;
    ledger_client_->URIEncode(value, &encoded_value);
    return encoded_value;
  }

  template <typename T>
  T GetState(const std::string& name) {
    T value;

    if constexpr (std::is_same_v<T, bool>) {
      ledger_client_->GetBooleanState(name, &value);
    } else if constexpr (std::is_same_v<T, int>) {
      ledger_client_->GetIntegerState(name, &value);
    } else if constexpr (std::is_same_v<T, double>) {
      ledger_client_->GetDoubleState(name, &value);
    } else if constexpr (std::is_same_v<T, std::string>) {
      ledger_client_->GetStringState(name, &value);
    } else if constexpr (std::is_same_v<T, int64_t>) {
      ledger_client_->GetInt64State(name, &value);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      ledger_client_->GetUint64State(name, &value);
    } else if constexpr (std::is_same_v<T, base::Value>) {
      ledger_client_->GetValueState(name, &value);
    } else if constexpr (std::is_same_v<T, base::Time>) {
      ledger_client_->GetTimeState(name, &value);
    } else {
      static_assert(base::AlwaysFalse<T>, "Unsupported type!");
    }

    return value;
  }

  template <typename T>
  void SetState(const std::string& name, T value) {
    if constexpr (std::is_same_v<T, bool>) {
      ledger_client_->SetBooleanState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, int>) {
      ledger_client_->SetIntegerState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, double>) {
      ledger_client_->SetDoubleState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, std::string>) {
      ledger_client_->SetStringState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, int64_t>) {
      ledger_client_->SetInt64State(name, std::move(value));
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      ledger_client_->SetUint64State(name, std::move(value));
    } else if constexpr (std::is_same_v<T, base::Value>) {
      ledger_client_->SetValueState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, base::Time>) {
      ledger_client_->SetTimeState(name, std::move(value));
    } else {
      static_assert(base::AlwaysFalse<T>, "Unsupported type!");
    }
  }

  template <typename T>
  T GetOption(const std::string& name) {
    T value;

    if constexpr (std::is_same_v<T, bool>) {
      ledger_client_->GetBooleanOption(name, &value);
    } else if constexpr (std::is_same_v<T, int>) {
      ledger_client_->GetIntegerOption(name, &value);
    } else if constexpr (std::is_same_v<T, double>) {
      ledger_client_->GetDoubleOption(name, &value);
    } else if constexpr (std::is_same_v<T, std::string>) {
      ledger_client_->GetStringOption(name, &value);
    } else if constexpr (std::is_same_v<T, int64_t>) {
      ledger_client_->GetInt64Option(name, &value);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      ledger_client_->GetUint64Option(name, &value);
    } else {
      static_assert(base::AlwaysFalse<T>, "Unsupported type!");
    }

    return value;
  }

  std::string GetLegacyWallet() {
    std::string wallet;
    ledger_client_->GetLegacyWallet(&wallet);
    return wallet;
  }

  ledger::mojom::ClientInfoPtr GetClientInfo() {
    auto info = ledger::mojom::ClientInfo::New();
    ledger_client_->GetClientInfo(&info);
    return info;
  }

  absl::optional<std::string> EncryptString(const std::string& value) {
    absl::optional<std::string> result;
    ledger_client_->EncryptString(value, &result);
    return result;
  }

  absl::optional<std::string> DecryptString(const std::string& value) {
    absl::optional<std::string> result;
    ledger_client_->DecryptString(value, &result);
    return result;
  }
  // mojom::LedgerClient sync helpers end

  bool IsShuttingDown() const;

 private:
  enum class ReadyState {
    kUninitialized,
    kInitializing,
    kReady,
    kShuttingDown
  };

  bool IsUninitialized() const;

  bool IsReady() const;

  virtual void InitializeDatabase(bool execute_create_script,
                                  ledger::LegacyResultCallback callback);

  void OnDatabaseInitialized(ledger::mojom::Result result,
                             ledger::LegacyResultCallback callback);

  void OnStateInitialized(ledger::LegacyResultCallback callback,
                          ledger::mojom::Result result);

  void OnInitialized(ledger::mojom::Result result,
                     ledger::LegacyResultCallback callback);

  void StartServices();

  void OnAllDone(ledger::mojom::Result result,
                 ledger::LegacyResultCallback callback);

  template <typename T>
  void WhenReady(T callback);

  mojo::AssociatedReceiver<mojom::Ledger> receiver_;
  mojo::AssociatedRemote<mojom::LedgerClient> ledger_client_;
  std::unique_ptr<ledger::promotion::Promotion> promotion_;
  std::unique_ptr<ledger::publisher::Publisher> publisher_;
  std::unique_ptr<braveledger_media::Media> media_;
  std::unique_ptr<ledger::contribution::Contribution> contribution_;
  std::unique_ptr<ledger::wallet::Wallet> wallet_;
  std::unique_ptr<ledger::database::Database> database_;
  std::unique_ptr<ledger::report::Report> report_;
  std::unique_ptr<ledger::sku::SKU> sku_;
  std::unique_ptr<ledger::state::State> state_;
  std::unique_ptr<ledger::api::API> api_;
  std::unique_ptr<ledger::recovery::Recovery> recovery_;
  std::unique_ptr<ledger::bitflyer::Bitflyer> bitflyer_;
  std::unique_ptr<ledger::gemini::Gemini> gemini_;
  std::unique_ptr<ledger::uphold::Uphold> uphold_;
  std::map<uint32_t, ledger::mojom::VisitData> current_pages_;
  uint64_t last_tab_active_time_ = 0;
  uint32_t last_shown_tab_id_ = -1;
  std::queue<std::function<void()>> ready_callbacks_;
  ReadyState ready_state_ = ReadyState::kUninitialized;
};

}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_H_
