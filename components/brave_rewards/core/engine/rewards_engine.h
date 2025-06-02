/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_REWARDS_ENGINE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_REWARDS_ENGINE_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/supports_user_data.h"
#include "brave/components/brave_rewards/core/engine/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/engine/rewards_log_stream.h"
#include "brave/components/brave_rewards/core/mojom/rewards_engine.mojom.h"
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

  void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) override;

  void GetPublisherMinVisits(GetPublisherMinVisitsCallback callback) override;

  void GetReconcileStamp(GetReconcileStampCallback callback) override;

  void SetPublisherExclude(const std::string& publisher_key,
                           mojom::PublisherExclude exclude,
                           SetPublisherExcludeCallback callback) override;

  void RestorePublishers(RestorePublishersCallback callback) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) override;

  void SetPublisherMinVisits(int visits) override;

  void GetBalanceReport(mojom::ActivityMonth month,
                        int32_t year,
                        GetBalanceReportCallback callback) override;

  void NotifyPublisherPageVisit(uint64_t tab_id,
                                mojom::VisitDataPtr visit_data,
                                const std::string& publisher_blob) override;

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
  std::unique_ptr<bitflyer::Bitflyer> bitflyer_;
  std::unique_ptr<gemini::Gemini> gemini_;
  std::unique_ptr<uphold::Uphold> uphold_;
  std::unique_ptr<zebpay::ZebPay> zebpay_;

  base::OneShotEvent ready_event_;
  base::WeakPtrFactory<RewardsEngine> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_REWARDS_ENGINE_H_
