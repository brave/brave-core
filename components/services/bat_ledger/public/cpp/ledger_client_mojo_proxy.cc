/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/public/cpp/ledger_client_mojo_proxy.h"

#include "base/logging.h"
#include "brave/base/containers/utils.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace bat_ledger {

LedgerClientMojoProxy::LedgerClientMojoProxy(
    ledger::LedgerClient* ledger_client)
  : ledger_client_(ledger_client) {
}

LedgerClientMojoProxy::~LedgerClientMojoProxy() {
}

// static
void LedgerClientMojoProxy::OnLoadLedgerState(
    CallbackHolder<LoadLedgerStateCallback>* holder,
    ledger::Result result,
    const std::string& data) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, data);
  delete holder;
}

void LedgerClientMojoProxy::LoadLedgerState(LoadLedgerStateCallback callback) {
  // deleted in OnLoadLedgerState
  auto* holder = new CallbackHolder<LoadLedgerStateCallback>(AsWeakPtr(),
      std::move(callback));
  ledger_client_->LoadLedgerState(
      std::bind(LedgerClientMojoProxy::OnLoadLedgerState, holder, _1, _2));
}

void LedgerClientMojoProxy::OnWalletProperties(
    const ledger::Result result,
    ledger::WalletPropertiesPtr properties) {
  ledger_client_->OnWalletProperties(
      result,
      std::move(properties));
}

// static
void LedgerClientMojoProxy::OnLoadPublisherState(
    CallbackHolder<LoadLedgerStateCallback>* holder,
    ledger::Result result,
    const std::string& data) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, data);
  delete holder;
}

void LedgerClientMojoProxy::LoadPublisherState(
    LoadPublisherStateCallback callback) {
  auto* holder = new CallbackHolder<LoadPublisherStateCallback>(AsWeakPtr(),
      std::move(callback));
  ledger_client_->LoadPublisherState(
      std::bind(LedgerClientMojoProxy::OnLoadPublisherState,
          holder,
          _1,
          _2));
}

// static
void LedgerClientMojoProxy::OnSaveLedgerState(
    CallbackHolder<SaveLedgerStateCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::SaveLedgerState(
    const std::string& ledger_state,
    SaveLedgerStateCallback callback) {
  auto* holder = new CallbackHolder<SaveLedgerStateCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SaveLedgerState(
      ledger_state,
      std::bind(LedgerClientMojoProxy::OnSaveLedgerState,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnSavePublisherState(
    CallbackHolder<SavePublisherStateCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::SavePublisherState(
    const std::string& publisher_state,
    SavePublisherStateCallback callback) {
  auto* holder = new CallbackHolder<SavePublisherStateCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SavePublisherState(
      publisher_state,
      std::bind(LedgerClientMojoProxy::OnSavePublisherState,
                holder,
                _1));
}

void LedgerClientMojoProxy::OnReconcileComplete(
    const ledger::Result result,
    const std::string& contribution_id,
    const double amount,
    const ledger::RewardsType type) {
  ledger_client_->OnReconcileComplete(
      result,
      contribution_id,
      amount,
      type);
}

void LedgerClientMojoProxy::SetTimer(uint64_t time_offset,
    SetTimerCallback callback) {
  uint32_t timer_id;
  ledger_client_->SetTimer(time_offset, &timer_id);
  std::move(callback).Run(timer_id);
}

void LedgerClientMojoProxy::KillTimer(const uint32_t timer_id) {
  ledger_client_->KillTimer(timer_id);
}

void LedgerClientMojoProxy::OnPanelPublisherInfo(
    const ledger::Result result,
    ledger::PublisherInfoPtr publisher_info,
    uint64_t window_id) {
  ledger_client_->OnPanelPublisherInfo(
      result,
      std::move(publisher_info),
      window_id);
}

// static
void LedgerClientMojoProxy::OnFetchFavIcon(
    CallbackHolder<FetchFavIconCallback>* holder,
    bool success, const std::string& favicon_url) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(success, favicon_url);
  delete holder;
}

void LedgerClientMojoProxy::FetchFavIcon(const std::string& url,
    const std::string& favicon_key, FetchFavIconCallback callback) {
  // deleted in OnFetchFavIcon
  auto* holder = new CallbackHolder<FetchFavIconCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->FetchFavIcon(url, favicon_key,
      std::bind(LedgerClientMojoProxy::OnFetchFavIcon, holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnLoadNicewareList(
    CallbackHolder<LoadNicewareListCallback>* holder,
    const ledger::Result result,
    const std::string& data) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, data);
  delete holder;
}

void LedgerClientMojoProxy::LoadNicewareList(
    LoadNicewareListCallback callback) {
  // deleted in OnLoadNicewareList
  auto* holder = new CallbackHolder<LoadNicewareListCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->LoadNicewareList(
      std::bind(LedgerClientMojoProxy::OnLoadNicewareList,
        holder, _1, _2));
}

void LedgerClientMojoProxy::URIEncode(const std::string& value,
    URIEncodeCallback callback) {
  std::move(callback).Run(ledger_client_->URIEncode(value));
}

// static
void LedgerClientMojoProxy::OnLoadURL(
    CallbackHolder<LoadURLCallback>* holder,
    int32_t response_code, const std::string& response,
    const std::map<std::string, std::string>& headers) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get())
        .Run(response_code, response, base::MapToFlatMap(headers));
  delete holder;
}

void LedgerClientMojoProxy::LoadURL(const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    ledger::UrlMethod method,
    LoadURLCallback callback) {
  // deleted in OnLoadURL
  auto* holder = new CallbackHolder<LoadURLCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->LoadURL(url, headers, content, contentType,
      method,
      std::bind(LedgerClientMojoProxy::OnLoadURL, holder, _1, _2, _3));
}

void LedgerClientMojoProxy::PublisherListNormalized(
    ledger::PublisherInfoList list) {
  ledger_client_->PublisherListNormalized(std::move(list));
}

// static
void LedgerClientMojoProxy::OnSaveState(
    CallbackHolder<SaveStateCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::SaveState(
    const std::string& name,
    const std::string& value,
    SaveStateCallback callback) {
  // deleted in OnSaveState
  auto* holder = new CallbackHolder<SaveStateCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->SaveState(
      name, value,
      std::bind(LedgerClientMojoProxy::OnSaveState, holder, _1));
}

// static
void LedgerClientMojoProxy::OnLoadState(
    CallbackHolder<LoadStateCallback>* holder,
    const ledger::Result result,
    const std::string& value) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, value);
  delete holder;
}

void LedgerClientMojoProxy::LoadState(
    const std::string& name,
    LoadStateCallback callback) {
  // deleted in OnSaveState
  auto* holder = new CallbackHolder<LoadStateCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->LoadState(
      name, std::bind(LedgerClientMojoProxy::OnLoadState, holder,
                      _1, _2));
}

// static
void LedgerClientMojoProxy::OnResetState(
    CallbackHolder<ResetStateCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::ResetState(
    const std::string& name,
    ResetStateCallback callback) {
  // deleted in OnResetState
  auto* holder = new CallbackHolder<ResetStateCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->ResetState(
      name,
      std::bind(LedgerClientMojoProxy::OnResetState, holder, _1));
}

void LedgerClientMojoProxy::SetBooleanState(const std::string& name,
                                            bool value) {
  ledger_client_->SetBooleanState(name, value);
}

void LedgerClientMojoProxy::GetBooleanState(const std::string& name,
                                            GetBooleanStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetBooleanState(name));
}

void LedgerClientMojoProxy::SetIntegerState(const std::string& name,
                                            int value) {
  ledger_client_->SetIntegerState(name, value);
}

void LedgerClientMojoProxy::GetIntegerState(const std::string& name,
                                            GetIntegerStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetIntegerState(name));
}

void LedgerClientMojoProxy::SetDoubleState(const std::string& name,
                                           double value) {
  ledger_client_->SetDoubleState(name, value);
}

void LedgerClientMojoProxy::GetDoubleState(const std::string& name,
                                           GetDoubleStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetDoubleState(name));
}

void LedgerClientMojoProxy::SetStringState(const std::string& name,
                                           const std::string& value) {
  ledger_client_->SetStringState(name, value);
}

void LedgerClientMojoProxy::GetStringState(const std::string& name,
                                           GetStringStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetStringState(name));
}

void LedgerClientMojoProxy::SetInt64State(const std::string& name,
                                          int64_t value) {
  ledger_client_->SetInt64State(name, value);
}

void LedgerClientMojoProxy::GetInt64State(const std::string& name,
                                          GetInt64StateCallback callback) {
  std::move(callback).Run(ledger_client_->GetInt64State(name));
}

void LedgerClientMojoProxy::SetUint64State(const std::string& name,
                                           uint64_t value) {
  ledger_client_->SetUint64State(name, value);
}

void LedgerClientMojoProxy::GetUint64State(const std::string& name,
                                           GetUint64StateCallback callback) {
  std::move(callback).Run(ledger_client_->GetUint64State(name));
}

void LedgerClientMojoProxy::ClearState(const std::string& name) {
  ledger_client_->ClearState(name);
}

void LedgerClientMojoProxy::GetBooleanOption(
    const std::string& name,
    GetBooleanOptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetBooleanOption(name));
}

void LedgerClientMojoProxy::GetIntegerOption(
    const std::string& name,
    GetIntegerOptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetIntegerOption(name));
}

void LedgerClientMojoProxy::GetDoubleOption(
    const std::string& name,
    GetDoubleOptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetDoubleOption(name));
}

void LedgerClientMojoProxy::GetStringOption(
    const std::string& name,
    GetStringOptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetStringOption(name));
}

void LedgerClientMojoProxy::GetInt64Option(
    const std::string& name,
    GetInt64OptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetInt64Option(name));
}

void LedgerClientMojoProxy::GetUint64Option(
    const std::string& name,
    GetUint64OptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetUint64Option(name));
}

void LedgerClientMojoProxy::SetConfirmationsIsReady(const bool is_ready) {
  ledger_client_->SetConfirmationsIsReady(is_ready);
}

void LedgerClientMojoProxy::ConfirmationsTransactionHistoryDidChange() {
  ledger_client_->ConfirmationsTransactionHistoryDidChange();
}

void LedgerClientMojoProxy::OnContributeUnverifiedPublishers(
      const ledger::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) {
  ledger_client_->OnContributeUnverifiedPublishers(
      result,
      publisher_key,
      publisher_name);
}

// static
void LedgerClientMojoProxy::OnGetExternalWallets(
    CallbackHolder<GetExternalWalletsCallback>* holder,
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(base::MapToFlatMap(std::move(wallets)));
  delete holder;
}

void LedgerClientMojoProxy::GetExternalWallets(
    GetExternalWalletsCallback callback) {
  auto* holder = new CallbackHolder<GetExternalWalletsCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->GetExternalWallets(
      std::bind(LedgerClientMojoProxy::OnGetExternalWallets,
                holder,
                _1));
}

void LedgerClientMojoProxy::SaveExternalWallet(
    const std::string& wallet_type,
    ledger::ExternalWalletPtr wallet) {
  ledger_client_->SaveExternalWallet(wallet_type, std::move(wallet));
}

// static
void LedgerClientMojoProxy::OnShowNotification(
    CallbackHolder<ShowNotificationCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ShowNotificationCallback callback) {
  auto* holder = new CallbackHolder<ShowNotificationCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->ShowNotification(
      type,
      args,
      std::bind(LedgerClientMojoProxy::OnShowNotification,
                holder,
                _1));
}

void LedgerClientMojoProxy::GetTransferFees(
    const std::string& wallet_type,
    GetTransferFeesCallback callback) {
  auto list = ledger_client_->GetTransferFees(wallet_type);
  std::move(callback).Run(base::MapToFlatMap(std::move(list)));
}

void LedgerClientMojoProxy::SetTransferFee(
    const std::string& wallet_type,
    ledger::TransferFeePtr transfer_fee) {
  ledger_client_->SetTransferFee(wallet_type, std::move(transfer_fee));
}

void LedgerClientMojoProxy::RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id) {
  ledger_client_->RemoveTransferFee(wallet_type, id);
}

void LedgerClientMojoProxy::GetClientInfo(
    GetClientInfoCallback callback) {
  auto info = ledger_client_->GetClientInfo();
  std::move(callback).Run(std::move(info));
}

void LedgerClientMojoProxy::UnblindedTokensReady() {
  ledger_client_->UnblindedTokensReady();
}

void LedgerClientMojoProxy::ReconcileStampReset() {
  ledger_client_->ReconcileStampReset();
}

// static
void LedgerClientMojoProxy::OnRunDBTransaction(
    CallbackHolder<RunDBTransactionCallback>* holder,
    ledger::DBCommandResponsePtr response) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(response));
  }
  delete holder;
}

void LedgerClientMojoProxy::RunDBTransaction(
    ledger::DBTransactionPtr transaction,
    RunDBTransactionCallback callback) {
  auto* holder = new CallbackHolder<RunDBTransactionCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->RunDBTransaction(
      std::move(transaction),
      std::bind(LedgerClientMojoProxy::OnRunDBTransaction,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetCreateScript(
    CallbackHolder<GetCreateScriptCallback>* holder,
    const std::string& script,
    const int table_version) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(script, table_version);
  }
  delete holder;
}

void LedgerClientMojoProxy::GetCreateScript(
    GetCreateScriptCallback callback) {
  auto* holder = new CallbackHolder<GetCreateScriptCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetCreateScript(
      std::bind(LedgerClientMojoProxy::OnGetCreateScript,
                holder,
                _1,
                _2));
}

void LedgerClientMojoProxy::PendingContributionSaved(
    const ledger::Result result) {
  ledger_client_->PendingContributionSaved(result);
}

}  // namespace bat_ledger
