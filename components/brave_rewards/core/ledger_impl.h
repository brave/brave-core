/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_H_

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/types/always_false.h"
#include "brave/components/brave_rewards/common/mojom/bat_ledger.mojom.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace ledger {
inline mojom::Environment _environment = mojom::Environment::PRODUCTION;
inline bool is_debug = false;
inline bool is_testing = false;
inline int state_migration_target_version_for_testing = -1;
inline int reconcile_interval = 0;  // minutes
inline int retry_interval = 0;      // seconds
}  // namespace ledger

namespace braveledger_media {
class Media;
}

namespace ledger {
namespace promotion {
class Promotion;
}
namespace publisher {
class Publisher;
}
namespace contribution {
class Contribution;
}
namespace wallet {
class Wallet;
}
namespace database {
class Database;
}
namespace report {
class Report;
}
namespace state {
class State;
}
namespace api {
class API;
}
namespace recovery {
class Recovery;
}
namespace bitflyer {
class Bitflyer;
}
namespace gemini {
class Gemini;
}
namespace uphold {
class Uphold;
}

class LedgerImpl : public mojom::Ledger {
 public:
  explicit LedgerImpl(
      mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote);

  ~LedgerImpl() override;

  LedgerImpl(const LedgerImpl&) = delete;

  LedgerImpl& operator=(const LedgerImpl&) = delete;

  // mojom::Ledger implementation begin (in the order of appearance in Mojom)
  void Initialize(bool execute_create_script,
                  InitializeCallback callback) override;

  void SetEnvironment(mojom::Environment environment) override;

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

  void SetPublisherAllowNonVerified(bool allow) override;

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

  void GetTransactionReport(mojom::ActivityMonth month,
                            int year,
                            GetTransactionReportCallback callback) override;

  void GetContributionReport(mojom::ActivityMonth month,
                             int year,
                             GetContributionReportCallback callback) override;

  void GetAllContributions(GetAllContributionsCallback callback) override;

  void SavePublisherInfoForTip(
      mojom::PublisherInfoPtr info,
      SavePublisherInfoForTipCallback callback) override;

  void GetMonthlyReport(mojom::ActivityMonth month,
                        int year,
                        GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(GetAllMonthlyReportIdsCallback callback) override;

  void GetAllPromotions(GetAllPromotionsCallback callback) override;

  void Shutdown(ShutdownCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;
  // mojom::Ledger implementation end

  // mojom::LedgerClient helpers begin (in the order of appearance in Mojom)
  template <typename LoadURLCallback>
  void LoadURL(mojom::UrlRequestPtr request, LoadURLCallback callback) {
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

    if constexpr (std::is_same_v<LoadURLCallback, ledger::LoadURLCallback>) {
      ledger_client_->LoadURL(std::move(request), std::move(callback));
    } else {
      ledger_client_->LoadURL(std::move(request),
                              base::BindOnce(
                                  [](LegacyLoadURLCallback callback,
                                     mojom::UrlResponsePtr response) {
                                    callback(std::move(response));
                                  },
                                  std::move(callback)));
    }
  }

  std::string URIEncode(const std::string& value);

  template <typename T>
  T GetState(const std::string& name) {
    T value;

    if constexpr (std::is_same_v<T, bool>) {
      ledger_client_->GetBooleanState(name, &value);
    } else if constexpr (std::is_same_v<T, int32_t>) {
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
    } else if constexpr (std::is_same_v<T, int32_t>) {
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
    } else if constexpr (std::is_same_v<T, int32_t>) {
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

  std::string GetLegacyWallet();

  mojom::ClientInfoPtr GetClientInfo();

  template <typename RunDBTransactionCallback>
  void RunDBTransaction(mojom::DBTransactionPtr transaction,
                        RunDBTransactionCallback callback) {
    if constexpr (std::is_same_v<RunDBTransactionCallback,
                                 ledger::RunDBTransactionCallback>) {
      ledger_client_->RunDBTransaction(std::move(transaction),
                                       std::move(callback));
    } else {
      ledger_client_->RunDBTransaction(
          std::move(transaction),
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
  // mojom::LedgerClient helpers end

  mojom::LedgerClient* client();

  state::State* state();

  virtual promotion::Promotion* promotion();

  publisher::Publisher* publisher();

  braveledger_media::Media* media();

  contribution::Contribution* contribution();

  wallet::Wallet* wallet();

  report::Report* report();

  api::API* api();

  virtual database::Database* database();

  bitflyer::Bitflyer* bitflyer();

  gemini::Gemini* gemini();

  uphold::Uphold* uphold();

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
                                  LegacyResultCallback callback);

  void OnDatabaseInitialized(mojom::Result result,
                             LegacyResultCallback callback);

  void OnStateInitialized(LegacyResultCallback callback, mojom::Result result);

  void OnInitialized(mojom::Result result, LegacyResultCallback callback);

  void StartServices();

  void OnAllDone(mojom::Result result, LegacyResultCallback callback);

  template <typename T>
  void WhenReady(T callback);

  mojo::AssociatedRemote<mojom::LedgerClient> ledger_client_;
  std::unique_ptr<promotion::Promotion> promotion_;
  std::unique_ptr<publisher::Publisher> publisher_;
  std::unique_ptr<braveledger_media::Media> media_;
  std::unique_ptr<contribution::Contribution> contribution_;
  std::unique_ptr<wallet::Wallet> wallet_;
  std::unique_ptr<database::Database> database_;
  std::unique_ptr<report::Report> report_;
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

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_IMPL_H_
