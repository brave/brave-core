/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_IOS_H_
#define BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_IOS_H_

#import <Foundation/Foundation.h>
#include <string>
#include <vector>
#import "bat/ledger/ledger_client.h"

@protocol LedgerClientBridge;

class LedgerClientIOS : public ledger::LedgerClient {
 public:
  explicit LedgerClientIOS(id<LedgerClientBridge> bridge);
  ~LedgerClientIOS() override;

 private:
  __unsafe_unretained id<LedgerClientBridge> bridge_;

  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::client::FetchIconCallback callback) override;
  void LoadLedgerState(ledger::client::OnLoadCallback callback) override;
  void LoadPublisherState(ledger::client::OnLoadCallback callback) override;
  void LoadURL(ledger::mojom::UrlRequestPtr request,
               ledger::client::LoadURLCallback callback) override;
  void Log(const char* file,
           const int line,
           const int verbose_level,
           const std::string& message) override;
  void OnPanelPublisherInfo(ledger::mojom::Result result,
                            ledger::mojom::PublisherInfoPtr publisher_info,
                            uint64_t windowId) override;
  void OnPublisherRegistryUpdated() override;
  void OnPublisherUpdated(const std::string& publisher_id) override;
  void OnReconcileComplete(
      ledger::mojom::Result result,
      ledger::mojom::ContributionInfoPtr contribution) override;
  void PublisherListNormalized(
      std::vector<ledger::mojom::PublisherInfoPtr> list) override;
  std::string URIEncode(const std::string& value) override;
  void OnContributeUnverifiedPublishers(
      ledger::mojom::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) override;
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
  std::string GetLegacyWallet() override;
  void ShowNotification(const std::string& type,
                        const std::vector<std::string>& args,
                        ledger::client::LegacyResultCallback callback) override;
  bool GetBooleanOption(const std::string& name) const override;
  int GetIntegerOption(const std::string& name) const override;
  double GetDoubleOption(const std::string& name) const override;
  std::string GetStringOption(const std::string& name) const override;
  int64_t GetInt64Option(const std::string& name) const override;
  uint64_t GetUint64Option(const std::string& name) const override;
  ledger::mojom::ClientInfoPtr GetClientInfo() override;
  void UnblindedTokensReady() override;
  void ReconcileStampReset() override;
  void RunDBTransaction(
      ledger::mojom::DBTransactionPtr transaction,
      ledger::client::RunDBTransactionCallback callback) override;
  void GetCreateScript(
      ledger::client::GetCreateScriptCallback callback) override;
  void PendingContributionSaved(const ledger::mojom::Result result) override;
  void ClearAllNotifications() override;
  void ExternalWalletConnected() const override;
  void ExternalWalletLoggedOut() const override;
  void ExternalWalletReconnected() const override;
  void DeleteLog(ledger::client::LegacyResultCallback callback) override;
  absl::optional<std::string> EncryptString(const std::string& value) override;
  absl::optional<std::string> DecryptString(const std::string& value) override;
};

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_IOS_H_
