/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_IMPL_H_

#include <map>
#include <queue>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/types/always_false.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/core/api/api.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/legacy/media/media.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/promotion/promotion.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/recovery/recovery.h"
#include "brave/components/brave_rewards/core/report/report.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_engine_context.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace brave_rewards::internal {

inline mojom::Environment _environment = mojom::Environment::PRODUCTION;
inline bool is_debug = false;
inline bool is_testing = false;
inline int state_migration_target_version_for_testing = -1;
inline int reconcile_interval = 0;  // minutes
inline int retry_interval = 0;      // seconds

#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
inline constexpr uint64_t kPublisherListRefreshInterval =
    7 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
#else
inline constexpr uint64_t kPublisherListRefreshInterval =
    3 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
#endif

class RewardsEngineImpl : public RewardsEngineContext,
                          public mojom::RewardsEngine {
 public:
  explicit RewardsEngineImpl(
      mojo::PendingAssociatedRemote<mojom::RewardsEngineClient> client_remote);

  ~RewardsEngineImpl() override;

  RewardsEngineImpl(const RewardsEngineImpl&) = delete;

  RewardsEngineImpl& operator=(const RewardsEngineImpl&) = delete;

  // mojom::RewardsEngine implementation begin (in the order of appearance in
  // Mojom)
  void Initialize(InitializeCallback callback) override;

  void GetEnvironment(GetEnvironmentCallback callback) override;

  void CreateRewardsWallet(const std::string& country,
                           CreateRewardsWalletCallback callback) override;

  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void GetAutoContributeProperties(
      GetAutoContributePropertiesCallback callback) override;

  void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) override;

  void GetPublisherMinVisits(GetPublisherMinVisitsCallback callback) override;

  void GetAutoContributeEnabled(
      GetAutoContributeEnabledCallback callback) override;

  void GetReconcileStamp(GetReconcileStampCallback callback) override;

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

  void SetPublisherExclude(const std::string& publisher_key,
                           mojom::PublisherExclude exclude,
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

  void SetAutoContributionAmount(double amount) override;

  void SetAutoContributeEnabled(bool enabled) override;

  void GetBalanceReport(mojom::ActivityMonth month,
                        int32_t year,
                        GetBalanceReportCallback callback) override;

  void GetPublisherActivityFromUrl(uint64_t window_id,
                                   mojom::VisitDataPtr visit_data,
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

  void SaveRecurringTip(mojom::RecurringTipPtr info,
                        SaveRecurringTipCallback callback) override;

  void SendContribution(const std::string& publisher_id,
                        double amount,
                        bool set_monthly,
                        SendContributionCallback callback) override;

  void GetRecurringTips(GetRecurringTipsCallback callback) override;

  void GetOneTimeTips(GetOneTimeTipsCallback callback) override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           mojom::ActivityInfoFilterPtr filter,
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
                         mojom::PublisherInfoPtr publisher_info,
                         SavePublisherInfoCallback callback) override;

  void SetInlineTippingPlatformEnabled(mojom::InlineTipsPlatforms platform,
                                       bool enabled) override;

  void GetInlineTippingPlatformEnabled(
      mojom::InlineTipsPlatforms platform,
      GetInlineTippingPlatformEnabledCallback callback) override;

  void GetShareURL(const base::flat_map<std::string, std::string>& args,
                   GetShareURLCallback callback) override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         GetExternalWalletCallback) override;

  void ConnectExternalWallet(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ConnectExternalWalletCallback) override;

  void GetTransactionReport(mojom::ActivityMonth month,
                            int year,
                            GetTransactionReportCallback callback) override;

  void GetContributionReport(mojom::ActivityMonth month,
                             int year,
                             GetContributionReportCallback callback) override;

  void GetAllContributions(GetAllContributionsCallback callback) override;

  void GetMonthlyReport(mojom::ActivityMonth month,
                        int year,
                        GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(GetAllMonthlyReportIdsCallback callback) override;

  void GetAllPromotions(GetAllPromotionsCallback callback) override;

  void Shutdown(ShutdownCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;
  // mojom::RewardsEngine implementation end

  // mojom::RewardsEngineClient helpers begin (in the order of appearance in
  // Mojom)
  template <typename Callback>
  void LoadURL(mojom::UrlRequestPtr request, Callback callback) {
    DCHECK(request);
    if (IsShuttingDown()) {
      BLOG(1, request->url + " will not be executed as we are shutting down");
      return;
    }

    if (!request->skip_log) {
      BLOG(5,
           UrlRequestToString(request->url, request->headers, request->content,
                              request->content_type, request->method));
    }

    if constexpr (std::is_same_v<Callback, LoadURLCallback>) {
      client_->LoadURL(std::move(request), std::move(callback));
    } else {
      client_->LoadURL(std::move(request),
                       base::BindOnce(
                           [](LegacyLoadURLCallback callback,
                              mojom::UrlResponsePtr response) {
                             callback(std::move(response));
                           },
                           std::move(callback)));
    }
  }

  template <typename T>
  T GetState(const std::string& name) {
    T value;

    if constexpr (std::is_same_v<T, bool>) {
      client_->GetBooleanState(name, &value);
    } else if constexpr (std::is_same_v<T, int32_t>) {
      client_->GetIntegerState(name, &value);
    } else if constexpr (std::is_same_v<T, double>) {
      client_->GetDoubleState(name, &value);
    } else if constexpr (std::is_same_v<T, std::string>) {
      client_->GetStringState(name, &value);
    } else if constexpr (std::is_same_v<T, int64_t>) {
      client_->GetInt64State(name, &value);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      client_->GetUint64State(name, &value);
    } else if constexpr (std::is_same_v<T, base::Value>) {
      client_->GetValueState(name, &value);
    } else if constexpr (std::is_same_v<T, base::Time>) {
      client_->GetTimeState(name, &value);
    } else {
      static_assert(base::AlwaysFalse<T>, "Unsupported type!");
    }

    return value;
  }

  template <typename T>
  void SetState(const std::string& name, T value) {
    if constexpr (std::is_same_v<T, bool>) {
      client_->SetBooleanState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, int32_t>) {
      client_->SetIntegerState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, double>) {
      client_->SetDoubleState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, std::string>) {
      client_->SetStringState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, int64_t>) {
      client_->SetInt64State(name, std::move(value));
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      client_->SetUint64State(name, std::move(value));
    } else if constexpr (std::is_same_v<T, base::Value>) {
      client_->SetValueState(name, std::move(value));
    } else if constexpr (std::is_same_v<T, base::Time>) {
      client_->SetTimeState(name, std::move(value));
    } else {
      static_assert(base::AlwaysFalse<T>, "Unsupported type!");
    }
  }

  bool IsBitFlyerRegion();

  std::string GetLegacyWallet();

  mojom::ClientInfoPtr GetClientInfo();

  template <typename Callback>
  void RunDBTransaction(mojom::DBTransactionPtr transaction,
                        Callback callback) {
    if constexpr (std::is_same_v<Callback, RunDBTransactionCallback>) {
      client_->RunDBTransaction(std::move(transaction), std::move(callback));
    } else {
      client_->RunDBTransaction(std::move(transaction),
                                base::BindOnce(
                                    [](LegacyRunDBTransactionCallback callback,
                                       mojom::DBCommandResponsePtr response) {
                                      callback(std::move(response));
                                    },
                                    std::move(callback)));
    }
  }

  absl::optional<std::string> EncryptString(const std::string& value);

  absl::optional<std::string> DecryptString(const std::string& value);
  // mojom::RewardsEngineClient helpers end

  mojom::RewardsEngineClient* client();

  promotion::Promotion* promotion() { return &promotion_; }

  publisher::Publisher* publisher() { return &publisher_; }

  Media* media() { return &media_; }

  contribution::Contribution* contribution() { return &contribution_; }

  wallet::Wallet* wallet() { return &wallet_; }

  report::Report* report() { return &report_; }

  state::State* state() { return &state_; }

  api::API* api() { return &api_; }

  recovery::Recovery* recovery() { return &recovery_; }

  bitflyer::Bitflyer* bitflyer() { return &bitflyer_; }

  gemini::Gemini* gemini() { return &gemini_; }

  uphold::Uphold* uphold() { return &uphold_; }

  bool IsShuttingDown() const;

  // This method is virtualised for test-only purposes.
  virtual database::Database* database();

  bool IsReady() const;

 private:
  enum class ReadyState {
    kUninitialized,
    kInitializing,
    kReady,
    kShuttingDown
  };

  bool IsUninitialized() const;

  virtual void InitializeDatabase(LegacyResultCallback callback);

  void OnDatabaseInitialized(mojom::Result result,
                             LegacyResultCallback callback);

  void OnStateInitialized(LegacyResultCallback callback, mojom::Result result);

  void OnInitialized(mojom::Result result, LegacyResultCallback callback);

  void StartServices();

  void OnAllDone(mojom::Result result, LegacyResultCallback callback);

  template <typename T>
  void WhenReady(T callback);

  mojo::AssociatedRemote<mojom::RewardsEngineClient> client_;

  promotion::Promotion promotion_;
  publisher::Publisher publisher_;
  Media media_;
  contribution::Contribution contribution_;
  wallet::Wallet wallet_;
  database::Database database_;
  report::Report report_;
  state::State state_;
  api::API api_;
  recovery::Recovery recovery_;
  bitflyer::Bitflyer bitflyer_;
  gemini::Gemini gemini_;
  uphold::Uphold uphold_;

  std::map<uint32_t, mojom::VisitData> current_pages_;
  uint64_t last_tab_active_time_ = 0;
  uint32_t last_shown_tab_id_ = -1;
  std::queue<std::function<void()>> ready_callbacks_;
  ReadyState ready_state_ = ReadyState::kUninitialized;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_IMPL_H_
