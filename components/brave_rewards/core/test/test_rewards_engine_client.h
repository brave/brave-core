/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_TEST_REWARDS_ENGINE_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_TEST_REWARDS_ENGINE_CLIENT_H_

#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/core/rewards_database.h"

namespace brave_rewards::internal {

struct FakeEncryption {
  static std::string EncryptString(const std::string& value);
  static std::optional<std::string> DecryptString(const std::string& value);

  static std::string Base64EncryptString(const std::string& value);
  static std::optional<std::string> Base64DecryptString(
      const std::string& value);
};

struct TestNetworkResult {
  TestNetworkResult(const std::string& url,
                    mojom::UrlMethod method,
                    mojom::UrlResponsePtr response);

  ~TestNetworkResult();

  std::string url;
  mojom::UrlMethod method;
  mojom::UrlResponsePtr response;
};

struct TestSPLAccountBalanceResult {
  TestSPLAccountBalanceResult(const std::string& solana_address,
                              const std::string& token_mint_address,
                              mojom::SolanaAccountBalancePtr balance);

  ~TestSPLAccountBalanceResult();

  std::string solana_address;
  std::string token_mint_address;
  mojom::SolanaAccountBalancePtr balance;
};

// Returns the file path of the directory containing test data
base::FilePath GetTestDataPath();

// An implementation of mojom::RewardsEngineClient useful for unit testing.
// A full SQLite database is provided, loaded in memory.
class TestRewardsEngineClient : public mojom::RewardsEngineClient {
 public:
  TestRewardsEngineClient();

  TestRewardsEngineClient(const TestRewardsEngineClient&) = delete;
  TestRewardsEngineClient& operator=(const TestRewardsEngineClient&) = delete;

  ~TestRewardsEngineClient() override;

  void LoadLegacyState(LoadLegacyStateCallback callback) override;

  void LoadPublisherState(LoadPublisherStateCallback callback) override;

  void OnReconcileComplete(mojom::Result result,
                           mojom::ContributionInfoPtr contribution) override;

  void OnPanelPublisherInfo(mojom::Result result,
                            mojom::PublisherInfoPtr publisher_info,
                            uint64_t window_id) override;

  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    FetchFavIconCallback callback) override;

  void LoadURL(mojom::UrlRequestPtr request, LoadURLCallback callback) override;

  void GetSPLTokenAccountBalance(
      const std::string& solana_address,
      const std::string& token_mint_address,
      GetSPLTokenAccountBalanceCallback callback) override;

  void PublisherListNormalized(
      std::vector<mojom::PublisherInfoPtr> list) override;

  void OnPublisherRegistryUpdated() override;

  void OnPublisherUpdated(const std::string& publisher_id) override;

  void GetBooleanState(const std::string& name,
                       GetBooleanStateCallback callback) override;

  void SetBooleanState(const std::string& name,
                       bool value,
                       SetBooleanStateCallback callback) override;

  void GetIntegerState(const std::string& name,
                       GetIntegerStateCallback callback) override;

  void SetIntegerState(const std::string& name,
                       int32_t value,
                       SetIntegerStateCallback callback) override;

  void GetDoubleState(const std::string& name,
                      GetDoubleStateCallback callback) override;

  void SetDoubleState(const std::string& name,
                      double value,
                      SetDoubleStateCallback callback) override;

  void GetStringState(const std::string& name,
                      GetStringStateCallback callback) override;

  void SetStringState(const std::string& name,
                      const std::string& value,
                      SetStringStateCallback callback) override;

  void GetInt64State(const std::string& name,
                     GetInt64StateCallback callback) override;

  void SetInt64State(const std::string& name,
                     int64_t value,
                     SetInt64StateCallback callback) override;

  void GetUint64State(const std::string& name,
                      GetUint64StateCallback callback) override;

  void SetUint64State(const std::string& name,
                      uint64_t value,
                      SetUint64StateCallback callback) override;

  void GetValueState(const std::string& name,
                     GetValueStateCallback callback) override;

  void SetValueState(const std::string& name,
                     base::Value value,
                     SetValueStateCallback callback) override;

  void GetTimeState(const std::string& name,
                    GetTimeStateCallback callback) override;

  void SetTimeState(const std::string& name,
                    base::Time value,
                    SetTimeStateCallback callback) override;

  void ClearState(const std::string& name,
                  ClearStateCallback callback) override;

  void GetClientCountryCode(GetClientCountryCodeCallback callback) override;

  void GetLegacyWallet(GetLegacyWalletCallback callback) override;

  void ShowNotification(const std::string& type,
                        const std::vector<std::string>& args,
                        ShowNotificationCallback callback) override;

  void GetClientInfo(GetClientInfoCallback callback) override;

  void ReconcileStampReset() override;

  void RunDBTransaction(mojom::DBTransactionPtr transaction,
                        RunDBTransactionCallback callback) override;

  void Log(const std::string& file,
           int32_t line,
           int32_t verbose_level,
           const std::string& message) override;

  void ClearAllNotifications() override;

  void ExternalWalletConnected() override;

  void ExternalWalletLoggedOut() override;

  void ExternalWalletReconnected() override;

  void ExternalWalletDisconnected() override;

  void DeleteLog(DeleteLogCallback callback) override;

  void EncryptString(const std::string& value,
                     EncryptStringCallback callback) override;

  void DecryptString(const std::string& value,
                     DecryptStringCallback callback) override;

  // Test environment setup methods:

  void AddNetworkResultForTesting(const std::string& url,
                                  mojom::UrlMethod method,
                                  mojom::UrlResponsePtr response);

  void AddSPLAccountBalanceResultForTesting(
      const std::string& solana_address,
      const std::string& token_mint_address,
      mojom::SolanaAccountBalancePtr balance);

  RewardsDatabase& database() { return engine_database_; }

 private:
  RewardsDatabase engine_database_;
  base::Value::Dict state_store_;
  std::list<TestNetworkResult> network_results_;
  std::list<TestSPLAccountBalanceResult> spl_balance_results_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_TEST_REWARDS_ENGINE_CLIENT_H_
