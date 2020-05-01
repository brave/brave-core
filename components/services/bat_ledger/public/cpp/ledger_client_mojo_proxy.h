/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_PROXY_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_PROXY_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"

namespace bat_ledger {

class LedgerClientMojoProxy : public mojom::BatLedgerClient,
                          public base::SupportsWeakPtr<LedgerClientMojoProxy> {
 public:
  explicit LedgerClientMojoProxy(ledger::LedgerClient* ledger_client);
  ~LedgerClientMojoProxy() override;

  // bat_ledger::mojom::BatLedgerClient
  void LoadLedgerState(LoadLedgerStateCallback callback) override;
  void OnWalletProperties(
      const ledger::Result result,
      ledger::WalletPropertiesPtr properties) override;
  void OnReconcileComplete(
      const ledger::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::RewardsType type) override;

  void LoadPublisherState(LoadPublisherStateCallback callback) override;
  void SaveLedgerState(const std::string& ledger_state,
      SaveLedgerStateCallback callback) override;
  void SavePublisherState(
      const std::string& publisher_state,
      SavePublisherStateCallback callback) override;

  void FetchFavIcon(const std::string& url, const std::string& favicon_key,
      FetchFavIconCallback callback) override;

  void LoadNicewareList(LoadNicewareListCallback callback) override;

  void SetTimer(uint64_t time_offset, SetTimerCallback callback) override;
  void KillTimer(const uint32_t timer_id) override;
  void OnPanelPublisherInfo(
      const ledger::Result result,
      ledger::PublisherInfoPtr info,
      uint64_t window_id) override;

  void URIEncode(const std::string& value,
      URIEncodeCallback callback) override;

  void LoadURL(const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    ledger::UrlMethod method,
    LoadURLCallback callback) override;

  void PublisherListNormalized(ledger::PublisherInfoList list) override;

  void SaveState(const std::string& name,
                              const std::string& value,
                              SaveStateCallback callback) override;
  void LoadState(const std::string& name,
                              LoadStateCallback callback) override;
  void ResetState(
      const std::string& name,
      ResetStateCallback callback) override;
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

  void SetConfirmationsIsReady(const bool is_ready) override;

  void ConfirmationsTransactionHistoryDidChange() override;

  void OnContributeUnverifiedPublishers(
      const ledger::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) override;

  void GetExternalWallets(GetExternalWalletsCallback callback) override;

  void SaveExternalWallet(const std::string& wallet_type,
                          ledger::ExternalWalletPtr wallet) override;

  void ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ShowNotificationCallback callback) override;

  void SetTransferFee(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee) override;

  void GetTransferFees(
      const std::string& wallet_type,
      GetTransferFeesCallback callback) override;

  void RemoveTransferFee(
      const std::string& wallet_type,
      const std::string& id) override;

  void GetClientInfo(
      GetClientInfoCallback callback) override;

  void UnblindedTokensReady() override;

  void ReconcileStampReset() override;

  void RunDBTransaction(
      ledger::DBTransactionPtr transaction,
      RunDBTransactionCallback callback) override;

  void GetCreateScript(
      GetCreateScriptCallback callback) override;

  void PendingContributionSaved(const ledger::Result result) override;

 private:
  // workaround to pass base::OnceCallback into std::bind
  template <typename Callback>
  class CallbackHolder {
   public:
    CallbackHolder(base::WeakPtr<LedgerClientMojoProxy> client,
        Callback callback)
        : client_(client),
          callback_(std::move(callback)) {}
    ~CallbackHolder() = default;
    bool is_valid() { return !!client_.get(); }
    Callback& get() { return callback_; }

   private:
    base::WeakPtr<LedgerClientMojoProxy> client_;
    Callback callback_;
  };

  static void OnSavePublisherState(
      CallbackHolder<SavePublisherStateCallback>* holder,
      const ledger::Result result);

  static void OnSaveLedgerState(
      CallbackHolder<SaveLedgerStateCallback>* holder,
      const ledger::Result result);

  static void OnLoadLedgerState(
    CallbackHolder<LoadLedgerStateCallback>* holder,
    ledger::Result result,
    const std::string& data);

  static void OnLoadPublisherState(
    CallbackHolder<LoadLedgerStateCallback>* holder,
    ledger::Result result,
    const std::string& data);

  static void OnFetchFavIcon(
      CallbackHolder<FetchFavIconCallback>* holder,
      bool success,
      const std::string& favicon_url);

  static void OnLoadNicewareList(
      CallbackHolder<LoadNicewareListCallback>* holder,
      const ledger::Result result,
      const std::string& data);

  static void OnLoadURL(
      CallbackHolder<LoadURLCallback>* holder,
      int32_t response_code, const std::string& response,
      const std::map<std::string, std::string>& headers);

  static void OnSaveState(
      CallbackHolder<SaveStateCallback>* holder,
      ledger::Result result);

  static void OnLoadState(
      CallbackHolder<LoadStateCallback>* holder,
      ledger::Result result,
      const std::string& value);

  static void OnResetState(
      CallbackHolder<ResetStateCallback>* holder,
      ledger::Result result);

  static void OnGetExternalWallets(
    CallbackHolder<GetExternalWalletsCallback>* holder,
    std::map<std::string, ledger::ExternalWalletPtr> wallets);

  static void OnShowNotification(
    CallbackHolder<ShowNotificationCallback>* holder,
    const ledger::Result result);

  static void OnRunDBTransaction(
      CallbackHolder<RunDBTransactionCallback>* holder,
      ledger::DBCommandResponsePtr response);

  static void OnGetCreateScript(
      CallbackHolder<GetCreateScriptCallback>* holder,
      const std::string& script,
      const int table_version);

  ledger::LedgerClient* ledger_client_;

  DISALLOW_COPY_AND_ASSIGN(LedgerClientMojoProxy);
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_PUBLIC_CPP_LEDGER_CLIENT_MOJO_PROXY_H_
