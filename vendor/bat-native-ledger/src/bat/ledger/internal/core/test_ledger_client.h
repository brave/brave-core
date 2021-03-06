/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_TEST_LEDGER_CLIENT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_TEST_LEDGER_CLIENT_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"
#include "bat/ledger/internal/ledger_database_impl.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

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

  void LoadLedgerState(client::OnLoadCallback callback) override;

  void LoadPublisherState(client::OnLoadCallback callback) override;

  void OnPanelPublisherInfo(mojom::Result result,
                            mojom::PublisherInfoPtr publisher_info,
                            uint64_t windowId) override;

  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    client::FetchIconCallback callback) override;

  std::string URIEncode(const std::string& value) override;

  void LoadURL(mojom::UrlRequestPtr request,
               client::LoadURLCallback callback) override;

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
                        client::ResultCallback callback) override;

  mojom::ClientInfoPtr GetClientInfo() override;

  void UnblindedTokensReady() override;

  void ReconcileStampReset() override;

  void RunDBTransaction(mojom::DBTransactionPtr transaction,
                        client::RunDBTransactionCallback callback) override;

  void GetCreateScript(client::GetCreateScriptCallback callback) override;

  void PendingContributionSaved(const mojom::Result result) override;

  void ClearAllNotifications() override;

  void WalletDisconnected(const std::string& wallet_type) override;

  void DeleteLog(client::ResultCallback callback) override;

  bool SetEncryptedStringState(const std::string& name,
                               const std::string& value) override;

  std::string GetEncryptedStringState(const std::string& name) override;

  // Test environment setup methods:

  void SetOptionForTesting(const std::string& name, base::Value&& value);

  void AddNetworkResultForTesting(const std::string& url,
                                  mojom::UrlMethod method,
                                  mojom::UrlResponsePtr response);

  using LogCallback = base::RepeatingCallback<void(const std::string&)>;
  void SetLogCallbackForTesting(LogCallback callback);

  LedgerDatabaseImpl* database() { return ledger_database_.get(); }

 private:
  void LoadURLAfterDelay(mojom::UrlRequestPtr request,
                         client::LoadURLCallback callback);

  void RunDBTransactionCompleted(client::RunDBTransactionCallback callback,
                                 mojom::DBCommandResponsePtr response);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::unique_ptr<LedgerDatabaseImpl> ledger_database_;
  base::Value state_store_;
  base::Value encrypted_state_store_;
  base::Value option_store_;
  std::list<TestNetworkResult> network_results_;
  LogCallback log_callback_;
  base::WeakPtrFactory<TestLedgerClient> weak_factory_{this};
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_TEST_LEDGER_CLIENT_H_
