/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/supports_user_data.h"
#include "base/types/always_false.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_log_stream.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace brave_rewards::internal {

namespace publisher {
class Publisher;
}

class Media;

namespace contribution {
class Contribution;
}

namespace wallet {
class Wallet;
}

namespace database {
class Database;
}

namespace state {
class State;
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

namespace zebpay {
class ZebPay;
}

namespace wallet_provider {
class WalletProvider;
}

class RewardsEngine : public mojom::RewardsEngine,
                      private base::SupportsUserData {
 public:
  RewardsEngine(
      mojo::PendingAssociatedRemote<mojom::RewardsEngineClient> client_remote,
      const mojom::RewardsEngineOptions& options);

  ~RewardsEngine() override;

  RewardsEngine(const RewardsEngine&) = delete;

  RewardsEngine& operator=(const RewardsEngine&) = delete;

  // mojom::RewardsEngine implementation begin (in the order of appearance in
  // Mojom)
  void Initialize(InitializeCallback callback) override;

  void GetEnvironment(GetEnvironmentCallback callback) override;

  void CreateRewardsWallet(const std::string& country,
                           CreateRewardsWalletCallback callback) override;

  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void FetchUICards(FetchUICardsCallback callback) override;

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

  void IsPublisherRegistered(const std::string& publisher_id,
                             IsPublisherRegisteredCallback callback) override;

  void GetPublisherInfo(const std::string& publisher_key,
                        GetPublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(const std::string& publisher_key,
                             GetPublisherPanelInfoCallback callback) override;

  void SavePublisherInfo(uint64_t window_id,
                         mojom::PublisherInfoPtr publisher_info,
                         SavePublisherInfoCallback callback) override;

  void GetShareURL(const base::flat_map<std::string, std::string>& args,
                   GetShareURLCallback callback) override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallet(GetExternalWalletCallback) override;

  void BeginExternalWalletLogin(
      const std::string& wallet_type,
      BeginExternalWalletLoginCallback callback) override;

  void ConnectExternalWallet(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ConnectExternalWalletCallback) override;

  void GetAllContributions(GetAllContributionsCallback callback) override;

  void Shutdown(ShutdownCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;
  // mojom::RewardsEngine implementation end

  // mojom::RewardsEngineClient helpers begin (in the order of appearance in
  // Mojom)

  template <typename T>
  T GetState(const std::string& name) {
    bool ok = false;
    T value{};

    if constexpr (std::is_same_v<T, bool>) {
      ok = client_->GetBooleanState(name, &value);
    } else if constexpr (std::is_same_v<T, int32_t>) {
      ok = client_->GetIntegerState(name, &value);
    } else if constexpr (std::is_same_v<T, double>) {
      ok = client_->GetDoubleState(name, &value);
    } else if constexpr (std::is_same_v<T, std::string>) {
      ok = client_->GetStringState(name, &value);
    } else if constexpr (std::is_same_v<T, int64_t>) {
      ok = client_->GetInt64State(name, &value);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      ok = client_->GetUint64State(name, &value);
    } else if constexpr (std::is_same_v<T, base::Value>) {
      ok = client_->GetValueState(name, &value);
    } else if constexpr (std::is_same_v<T, base::Time>) {
      ok = client_->GetTimeState(name, &value);
    } else {
      static_assert(base::AlwaysFalse<T>, "Unsupported type!");
    }

    // Occasionally during shutdown the engine can fail to read preferences from
    // the client, likely due to the complexities of sync mojo calls.
    // TODO(https://github.com/brave/brave-browser/issues/37816): User pref
    // access should be refactored to handle these errors gracefully.
    DCHECK(ok) << "Unable to read state from Rewards engine client";

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

  std::string GetClientCountryCode();

  bool IsAutoContributeSupportedForClient();

  std::string GetLegacyWallet();

  mojom::ClientInfoPtr GetClientInfo();

  // Performs logging to the Rewards logging file as implemented by the client.
  //
  //   Log(FROM_HERE) << "This will appear in the log file when verbose logging"
  //                     "is enabled.";
  //
  //   LogError(FROM_HERE) << "This will always appear in the log file."
  //                          "Do not use with arbitrary strings or data!";
  //
  // NOTE: Do not use arbitrary strings when using `LogError`, as this can
  // result in sensitive data being written to the Rewards log file.
  RewardsLogStream Log(base::Location location);
  RewardsLogStream LogError(base::Location location);

  std::optional<std::string> EncryptString(const std::string& value);

  std::optional<std::string> DecryptString(const std::string& value);
  // mojom::RewardsEngineClient helpers end

  base::WeakPtr<RewardsEngine> GetWeakPtr();
  base::WeakPtr<const RewardsEngine> GetWeakPtr() const;

  mojom::RewardsEngineClient* client();

  template <typename T>
  T& Get() {
    auto* key = T::GetHelperKey();
    if (auto* helper = this->GetUserData(key)) {
      return *static_cast<T*>(helper);
    }
    auto instance = std::make_unique<T>(*this);
    auto& ref = *instance;
    this->SetUserData(key, std::move(instance));
    return ref;
  }

  template <typename T>
  void SetHelperForTesting(std::unique_ptr<T> helper) {
    this->SetUserData(T::GetHelperKey(), std::move(helper));
  }

  publisher::Publisher* publisher() { return publisher_.get(); }

  Media* media() { return media_.get(); }

  contribution::Contribution* contribution() { return contribution_.get(); }

  wallet::Wallet* wallet() { return wallet_.get(); }

  state::State* state() { return state_.get(); }

  bitflyer::Bitflyer* bitflyer() { return bitflyer_.get(); }

  gemini::Gemini* gemini() { return gemini_.get(); }

  uphold::Uphold* uphold() { return uphold_.get(); }

  zebpay::ZebPay* zebpay() { return zebpay_.get(); }

  wallet_provider::WalletProvider* GetExternalWalletProvider(
      const std::string& wallet_type);

  // This method is virtualised for test-only purposes.
  virtual database::Database* database();

  const mojom::RewardsEngineOptions& options() const { return options_; }

  mojom::RewardsEngineOptions& GetOptionsForTesting() { return options_; }

 private:
  bool IsReady();

  void OnInitializationComplete(InitializeCallback callback, bool success);

  void OnShutdownComplete(ShutdownCallback callback, bool success);

  void OnRecurringTipSaved(SaveRecurringTipCallback callback,
                           mojom::Result result);

  template <typename T>
  void WhenReady(T callback);

  mojo::AssociatedRemote<mojom::RewardsEngineClient> client_;
  mojom::RewardsEngineOptions options_;

  std::unique_ptr<publisher::Publisher> publisher_;
  std::unique_ptr<Media> media_;
  std::unique_ptr<contribution::Contribution> contribution_;
  std::unique_ptr<wallet::Wallet> wallet_;
  std::unique_ptr<database::Database> database_;
  std::unique_ptr<state::State> state_;
  std::unique_ptr<bitflyer::Bitflyer> bitflyer_;
  std::unique_ptr<gemini::Gemini> gemini_;
  std::unique_ptr<uphold::Uphold> uphold_;
  std::unique_ptr<zebpay::ZebPay> zebpay_;

  std::map<uint32_t, mojom::VisitData> current_pages_;
  uint64_t last_tab_active_time_ = 0;
  uint32_t last_shown_tab_id_ = -1;

  base::OneShotEvent ready_event_;
  base::WeakPtrFactory<RewardsEngine> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_H_
