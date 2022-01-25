/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_BRIDGE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"

namespace bat_ledger {

class LedgerClientMojoBridge :
    public mojom::BatLedgerClient,
    public base::SupportsWeakPtr<LedgerClientMojoBridge> {
 public:
  explicit LedgerClientMojoBridge(ledger::LedgerClient* ledger_client);
  ~LedgerClientMojoBridge() override;

  LedgerClientMojoBridge(const LedgerClientMojoBridge&) = delete;
  LedgerClientMojoBridge& operator=(const LedgerClientMojoBridge&) = delete;

  // bat_ledger::mojom::BatLedgerClient
  void LoadLedgerState(LoadLedgerStateCallback callback) override;
  void OnReconcileComplete(
      const ledger::type::Result result,
      ledger::type::ContributionInfoPtr contribution) override;

  void LoadPublisherState(LoadPublisherStateCallback callback) override;

  void FetchFavIcon(const std::string& url, const std::string& favicon_key,
      FetchFavIconCallback callback) override;

  void OnPanelPublisherInfo(
      const ledger::type::Result result,
      ledger::type::PublisherInfoPtr info,
      uint64_t window_id) override;

  void URIEncode(const std::string& value,
      URIEncodeCallback callback) override;

  void LoadURL(
      ledger::type::UrlRequestPtr request,
      LoadURLCallback callback) override;

  void PublisherListNormalized(ledger::type::PublisherInfoList list) override;

  void SetBooleanState(const std::string& name, bool value) override;
  void GetBooleanState(const std::string& name,
                       GetBooleanStateCallback callback) override;
  void SetIntegerState(const std::string& name, int value) override;
  void GetIntegerState(const std::string& name,
                       GetIntegerStateCallback callback) override;
  void SetDoubleState(const std::string& name, double value) override;
  void GetDoubleState(const std::string& name,
                      GetDoubleStateCallback callback) override;
  void SetStringState(const std::string& name,
                      const std::string& value) override;
  void GetStringState(const std::string& name,
                      GetStringStateCallback callback) override;
  void SetInt64State(const std::string& name, int64_t value) override;
  void GetInt64State(const std::string& name,
                     GetInt64StateCallback callback) override;
  void SetUint64State(const std::string& name, uint64_t value) override;
  void GetUint64State(const std::string& name,
                      GetUint64StateCallback callback) override;
  void ClearState(const std::string& name) override;

  void GetBooleanOption(
      const std::string& name,
      GetBooleanOptionCallback callback) override;
  void GetIntegerOption(
      const std::string& name,
      GetIntegerOptionCallback callback) override;
  void GetDoubleOption(
      const std::string& name,
      GetDoubleOptionCallback callback) override;
  void GetStringOption(
      const std::string& name,
      GetStringOptionCallback callback) override;
  void GetInt64Option(
      const std::string& name,
      GetInt64OptionCallback callback) override;
  void GetUint64Option(
      const std::string& name,
      GetUint64OptionCallback callback) override;

  void OnContributeUnverifiedPublishers(
      const ledger::type::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) override;

  void GetLegacyWallet(GetLegacyWalletCallback callback) override;

  void ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ShowNotificationCallback callback) override;

  void GetClientInfo(
      GetClientInfoCallback callback) override;

  void UnblindedTokensReady() override;

  void ReconcileStampReset() override;

  void RunDBTransaction(
      ledger::type::DBTransactionPtr transaction,
      RunDBTransactionCallback callback) override;

  void GetCreateScript(
      GetCreateScriptCallback callback) override;

  void PendingContributionSaved(const ledger::type::Result result) override;

  void Log(
      const std::string& file,
      const int32_t line,
      const int32_t verbose_level,
      const std::string& message) override;

  void ClearAllNotifications() override;

  void WalletDisconnected(const std::string& wallet_type) override;

  void DeleteLog(DeleteLogCallback callback) override;

  void EncryptString(const std::string& value,
                     EncryptStringCallback callback) override;

  void DecryptString(const std::string& value,
                     DecryptStringCallback callback) override;

 private:
  // workaround to pass base::OnceCallback into std::bind
  template <typename Callback>
  class CallbackHolder {
   public:
    CallbackHolder(base::WeakPtr<LedgerClientMojoBridge> client,
        Callback callback)
        : client_(client),
          callback_(std::move(callback)) {}
    ~CallbackHolder() = default;
    bool is_valid() { return !!client_.get(); }
    Callback& get() { return callback_; }

   private:
    base::WeakPtr<LedgerClientMojoBridge> client_;
    Callback callback_;
  };

  static void OnLoadLedgerState(
    CallbackHolder<LoadLedgerStateCallback>* holder,
    ledger::type::Result result,
    const std::string& data);

  static void OnLoadPublisherState(
    CallbackHolder<LoadLedgerStateCallback>* holder,
    ledger::type::Result result,
    const std::string& data);

  static void OnFetchFavIcon(
      CallbackHolder<FetchFavIconCallback>* holder,
      bool success,
      const std::string& favicon_url);

  static void OnLoadURL(
      CallbackHolder<LoadURLCallback>* holder,
      const ledger::type::UrlResponse& response);

  static void OnShowNotification(
    CallbackHolder<ShowNotificationCallback>* holder,
    const ledger::type::Result result);

  static void OnRunDBTransaction(
      CallbackHolder<RunDBTransactionCallback>* holder,
      ledger::type::DBCommandResponsePtr response);

  static void OnGetCreateScript(
      CallbackHolder<GetCreateScriptCallback>* holder,
      const std::string& script,
      const int table_version);

  static void OnDeleteLog(
      CallbackHolder<DeleteLogCallback>* holder,
      const ledger::type::Result result);

  raw_ptr<ledger::LedgerClient> ledger_client_ = nullptr;
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_BRIDGE_H_
