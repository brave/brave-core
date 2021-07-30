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
  void LoadURL(ledger::type::UrlRequestPtr request,
               ledger::client::LoadURLCallback callback) override;
  void Log(const char* file,
           const int line,
           const int verbose_level,
           const std::string& message) override;
  void OnPanelPublisherInfo(ledger::type::Result result,
                            ledger::type::PublisherInfoPtr publisher_info,
                            uint64_t windowId) override;
  void OnReconcileComplete(
      ledger::type::Result result,
      ledger::type::ContributionInfoPtr contribution) override;
  void PublisherListNormalized(ledger::type::PublisherInfoList list) override;
  std::string URIEncode(const std::string& value) override;
  void OnContributeUnverifiedPublishers(
      ledger::type::Result result,
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
  void ClearState(const std::string& name) override;
  std::string GetLegacyWallet() override;
  void ShowNotification(const std::string& type,
                        const std::vector<std::string>& args,
                        ledger::client::ResultCallback callback) override;
  bool GetBooleanOption(const std::string& name) const override;
  int GetIntegerOption(const std::string& name) const override;
  double GetDoubleOption(const std::string& name) const override;
  std::string GetStringOption(const std::string& name) const override;
  int64_t GetInt64Option(const std::string& name) const override;
  uint64_t GetUint64Option(const std::string& name) const override;
  ledger::type::ClientInfoPtr GetClientInfo() override;
  void UnblindedTokensReady() override;
  void ReconcileStampReset() override;
  void RunDBTransaction(
      ledger::type::DBTransactionPtr transaction,
      ledger::client::RunDBTransactionCallback callback) override;
  void GetCreateScript(
      ledger::client::GetCreateScriptCallback callback) override;
  void PendingContributionSaved(const ledger::type::Result result) override;
  void ClearAllNotifications() override;
  void WalletDisconnected(const std::string& wallet_type) override;
  void DeleteLog(ledger::client::ResultCallback callback) override;
  absl::optional<std::string> EncryptString(const std::string& value) override;
  absl::optional<std::string> DecryptString(const std::string& value) override;
};

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_IOS_H_
