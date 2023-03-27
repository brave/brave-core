/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_TEST_LEDGER_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_TEST_LEDGER_CLIENT_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/ledger_client.h"
#include "brave/components/brave_rewards/core/ledger_database.h"

namespace brave_rewards::core {

struct FakeEncryption {
  static std::string EncryptString(const std::string& value);
  static absl::optional<std::string> DecryptString(const std::string& value);

  static std::string Base64EncryptString(const std::string& value);
  static absl::optional<std::string> Base64DecryptString(
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

// Returns the file path of the directory containing test data
base::FilePath GetTestDataPath();

// An implementation of LedgerClient useful for unit testing. A full SQLite
// database is provided, loaded in memory.
class TestLedgerClient : public LedgerClient {
 public:
  TestLedgerClient();

  TestLedgerClient(const TestLedgerClient&) = delete;
  TestLedgerClient& operator=(const TestLedgerClient&) = delete;

  ~TestLedgerClient() override;

  void OnReconcileComplete(const mojom::Result result,
                           mojom::ContributionInfoPtr contribution) override;

  void LoadLedgerState(OnLoadCallback callback) override;

  void LoadPublisherState(OnLoadCallback callback) override;

  void OnPanelPublisherInfo(mojom::Result result,
                            mojom::PublisherInfoPtr publisher_info,
                            uint64_t windowId) override;

  void OnPublisherRegistryUpdated() override;

  void OnPublisherUpdated(const std::string& publisher_id) override;

  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    FetchIconCallback callback) override;

  std::string URIEncode(const std::string& value) override;

  void LoadURL(mojom::UrlRequestPtr request, LoadURLCallback callback) override;

  void Log(const char* file,
           const int line,
           const int verbose_level,
           const std::string& message) override;

  void PublisherListNormalized(
      std::vector<mojom::PublisherInfoPtr> list) override;

  void SetBooleanState(const std::string& name, bool value) override;

  bool GetBooleanState(const std::string& name) const override;

  void SetIntegerState(const std::string& name, int value) override;

  int GetIntegerState(const std::string& name) const override;

  void SetDoubleState(const std::string& name, double value) override;

  double GetDoubleState(const std::string& name) const override;

  void SetStringState(const std::string& name,
                      const std::string& value) override;

  std::string GetStringState(const std::string& name) const override;

  void SetInt64State(const std::string& name, int64_t value) override;

  int64_t GetInt64State(const std::string& name) const override;

  void SetUint64State(const std::string& name, uint64_t value) override;

  uint64_t GetUint64State(const std::string& name) const override;

  void SetValueState(const std::string& name, base::Value value) override;

  base::Value GetValueState(const std::string& name) const override;

  void SetTimeState(const std::string& name, base::Time time) override;

  base::Time GetTimeState(const std::string& name) const override;

  void ClearState(const std::string& name) override;

  bool GetBooleanOption(const std::string& name) const override;

  int GetIntegerOption(const std::string& name) const override;

  double GetDoubleOption(const std::string& name) const override;

  std::string GetStringOption(const std::string& name) const override;

  int64_t GetInt64Option(const std::string& name) const override;

  uint64_t GetUint64Option(const std::string& name) const override;

  void OnContributeUnverifiedPublishers(
      mojom::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) override;

  std::string GetLegacyWallet() override;

  void ShowNotification(const std::string& type,
                        const std::vector<std::string>& args,
                        LegacyResultCallback callback) override;

  mojom::ClientInfoPtr GetClientInfo() override;

  void UnblindedTokensReady() override;

  void ReconcileStampReset() override;

  void RunDBTransaction(mojom::DBTransactionPtr transaction,
                        RunDBTransactionCallback callback) override;

  void GetCreateScript(GetCreateScriptCallback callback) override;

  void PendingContributionSaved(const mojom::Result result) override;

  void ClearAllNotifications() override;

  void ExternalWalletConnected() const override;

  void ExternalWalletLoggedOut() const override;

  void ExternalWalletReconnected() const override;

  void DeleteLog(LegacyResultCallback callback) override;

  absl::optional<std::string> EncryptString(const std::string& value) override;

  absl::optional<std::string> DecryptString(const std::string& value) override;

  // Test environment setup methods:

  void SetOptionForTesting(const std::string& name, base::Value value);

  void AddNetworkResultForTesting(const std::string& url,
                                  mojom::UrlMethod method,
                                  mojom::UrlResponsePtr response);

  using LogCallback = base::RepeatingCallback<void(const std::string&)>;
  void SetLogCallbackForTesting(LogCallback callback);

  LedgerDatabase* database() { return &ledger_database_; }

 private:
  void LoadURLAfterDelay(mojom::UrlRequestPtr request,
                         LoadURLCallback callback);

  void RunDBTransactionAfterDelay(mojom::DBTransactionPtr transaction,
                                  RunDBTransactionCallback callback);

  LedgerDatabase ledger_database_;
  base::Value::Dict state_store_;
  base::Value::Dict option_store_;
  std::list<TestNetworkResult> network_results_;
  LogCallback log_callback_;
  base::WeakPtrFactory<TestLedgerClient> weak_factory_{this};
};

}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_TEST_LEDGER_CLIENT_H_
