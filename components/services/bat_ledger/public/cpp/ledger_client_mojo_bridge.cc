/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/public/cpp/ledger_client_mojo_bridge.h"

#include "base/logging.h"
#include "brave/base/containers/utils.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace bat_ledger {

LedgerClientMojoBridge::LedgerClientMojoBridge(
    ledger::LedgerClient* ledger_client)
  : ledger_client_(ledger_client) {
  DCHECK(ledger_client_);
}

LedgerClientMojoBridge::~LedgerClientMojoBridge() = default;

// static
void LedgerClientMojoBridge::OnLoadLedgerState(
    CallbackHolder<LoadLedgerStateCallback>* holder,
    ledger::Result result,
    const std::string& data) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, data);
  delete holder;
}

void LedgerClientMojoBridge::LoadLedgerState(LoadLedgerStateCallback callback) {
  // deleted in OnLoadLedgerState
  auto* holder = new CallbackHolder<LoadLedgerStateCallback>(AsWeakPtr(),
      std::move(callback));
  ledger_client_->LoadLedgerState(
      std::bind(LedgerClientMojoBridge::OnLoadLedgerState, holder, _1, _2));
}

// static
void LedgerClientMojoBridge::OnLoadPublisherState(
    CallbackHolder<LoadLedgerStateCallback>* holder,
    ledger::Result result,
    const std::string& data) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, data);
  delete holder;
}

void LedgerClientMojoBridge::LoadPublisherState(
    LoadPublisherStateCallback callback) {
  auto* holder = new CallbackHolder<LoadPublisherStateCallback>(AsWeakPtr(),
      std::move(callback));
  ledger_client_->LoadPublisherState(
      std::bind(LedgerClientMojoBridge::OnLoadPublisherState,
          holder,
          _1,
          _2));
}

void LedgerClientMojoBridge::OnReconcileComplete(
    const ledger::Result result,
    ledger::ContributionInfoPtr contribution) {
  ledger_client_->OnReconcileComplete(
      result,
      std::move(contribution));
}

void LedgerClientMojoBridge::SetTimer(uint64_t time_offset,
    SetTimerCallback callback) {
  uint32_t timer_id;
  ledger_client_->SetTimer(time_offset, &timer_id);
  std::move(callback).Run(timer_id);
}

void LedgerClientMojoBridge::KillTimer(const uint32_t timer_id) {
  ledger_client_->KillTimer(timer_id);
}

void LedgerClientMojoBridge::OnPanelPublisherInfo(
    const ledger::Result result,
    ledger::PublisherInfoPtr publisher_info,
    uint64_t window_id) {
  ledger_client_->OnPanelPublisherInfo(
      result,
      std::move(publisher_info),
      window_id);
}

// static
void LedgerClientMojoBridge::OnFetchFavIcon(
    CallbackHolder<FetchFavIconCallback>* holder,
    bool success, const std::string& favicon_url) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(success, favicon_url);
  delete holder;
}

void LedgerClientMojoBridge::FetchFavIcon(const std::string& url,
    const std::string& favicon_key, FetchFavIconCallback callback) {
  // deleted in OnFetchFavIcon
  auto* holder = new CallbackHolder<FetchFavIconCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->FetchFavIcon(url, favicon_key,
      std::bind(LedgerClientMojoBridge::OnFetchFavIcon, holder, _1, _2));
}

void LedgerClientMojoBridge::URIEncode(const std::string& value,
    URIEncodeCallback callback) {
  std::move(callback).Run(ledger_client_->URIEncode(value));
}

// static
void LedgerClientMojoBridge::OnLoadURL(
    CallbackHolder<LoadURLCallback>* holder,
    const ledger::UrlResponse& response) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(ledger::UrlResponse::New(response));
    delete holder;
  }
}

void LedgerClientMojoBridge::LoadURL(const std::string& url,
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
      std::bind(LedgerClientMojoBridge::OnLoadURL, holder, _1));
}

void LedgerClientMojoBridge::PublisherListNormalized(
    ledger::PublisherInfoList list) {
  ledger_client_->PublisherListNormalized(std::move(list));
}

// static
void LedgerClientMojoBridge::OnSaveState(
    CallbackHolder<SaveStateCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoBridge::SaveState(
    const std::string& name,
    const std::string& value,
    SaveStateCallback callback) {
  // deleted in OnSaveState
  auto* holder = new CallbackHolder<SaveStateCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->SaveState(
      name, value,
      std::bind(LedgerClientMojoBridge::OnSaveState, holder, _1));
}

// static
void LedgerClientMojoBridge::OnLoadState(
    CallbackHolder<LoadStateCallback>* holder,
    const ledger::Result result,
    const std::string& value) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, value);
  delete holder;
}

void LedgerClientMojoBridge::LoadState(
    const std::string& name,
    LoadStateCallback callback) {
  // deleted in OnSaveState
  auto* holder = new CallbackHolder<LoadStateCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->LoadState(
      name, std::bind(LedgerClientMojoBridge::OnLoadState, holder,
                      _1, _2));
}

// static
void LedgerClientMojoBridge::OnResetState(
    CallbackHolder<ResetStateCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoBridge::ResetState(
    const std::string& name,
    ResetStateCallback callback) {
  // deleted in OnResetState
  auto* holder = new CallbackHolder<ResetStateCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->ResetState(
      name,
      std::bind(LedgerClientMojoBridge::OnResetState, holder, _1));
}

void LedgerClientMojoBridge::SetBooleanState(const std::string& name,
                                            bool value) {
  ledger_client_->SetBooleanState(name, value);
}

void LedgerClientMojoBridge::GetBooleanState(const std::string& name,
                                            GetBooleanStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetBooleanState(name));
}

void LedgerClientMojoBridge::SetIntegerState(const std::string& name,
                                            int value) {
  ledger_client_->SetIntegerState(name, value);
}

void LedgerClientMojoBridge::GetIntegerState(const std::string& name,
                                            GetIntegerStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetIntegerState(name));
}

void LedgerClientMojoBridge::SetDoubleState(const std::string& name,
                                           double value) {
  ledger_client_->SetDoubleState(name, value);
}

void LedgerClientMojoBridge::GetDoubleState(const std::string& name,
                                           GetDoubleStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetDoubleState(name));
}

void LedgerClientMojoBridge::SetStringState(const std::string& name,
                                           const std::string& value) {
  ledger_client_->SetStringState(name, value);
}

void LedgerClientMojoBridge::GetStringState(const std::string& name,
                                           GetStringStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetStringState(name));
}

void LedgerClientMojoBridge::SetInt64State(const std::string& name,
                                          int64_t value) {
  ledger_client_->SetInt64State(name, value);
}

void LedgerClientMojoBridge::GetInt64State(const std::string& name,
                                          GetInt64StateCallback callback) {
  std::move(callback).Run(ledger_client_->GetInt64State(name));
}

void LedgerClientMojoBridge::SetUint64State(const std::string& name,
                                           uint64_t value) {
  ledger_client_->SetUint64State(name, value);
}

void LedgerClientMojoBridge::GetUint64State(const std::string& name,
                                           GetUint64StateCallback callback) {
  std::move(callback).Run(ledger_client_->GetUint64State(name));
}

void LedgerClientMojoBridge::ClearState(const std::string& name) {
  ledger_client_->ClearState(name);
}

void LedgerClientMojoBridge::GetBooleanOption(
    const std::string& name,
    GetBooleanOptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetBooleanOption(name));
}

void LedgerClientMojoBridge::GetIntegerOption(
    const std::string& name,
    GetIntegerOptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetIntegerOption(name));
}

void LedgerClientMojoBridge::GetDoubleOption(
    const std::string& name,
    GetDoubleOptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetDoubleOption(name));
}

void LedgerClientMojoBridge::GetStringOption(
    const std::string& name,
    GetStringOptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetStringOption(name));
}

void LedgerClientMojoBridge::GetInt64Option(
    const std::string& name,
    GetInt64OptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetInt64Option(name));
}

void LedgerClientMojoBridge::GetUint64Option(
    const std::string& name,
    GetUint64OptionCallback callback) {
  std::move(callback).Run(ledger_client_->GetUint64Option(name));
}

void LedgerClientMojoBridge::SetConfirmationsIsReady(const bool is_ready) {
  ledger_client_->SetConfirmationsIsReady(is_ready);
}

void LedgerClientMojoBridge::ConfirmationsTransactionHistoryDidChange() {
  ledger_client_->ConfirmationsTransactionHistoryDidChange();
}

void LedgerClientMojoBridge::OnContributeUnverifiedPublishers(
      const ledger::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) {
  ledger_client_->OnContributeUnverifiedPublishers(
      result,
      publisher_key,
      publisher_name);
}

void LedgerClientMojoBridge::GetExternalWallets(
    GetExternalWalletsCallback callback) {
  std::move(callback).Run(
      base::MapToFlatMap(ledger_client_->GetExternalWallets()));
}

void LedgerClientMojoBridge::SaveExternalWallet(
    const std::string& wallet_type,
    ledger::ExternalWalletPtr wallet) {
  ledger_client_->SaveExternalWallet(wallet_type, std::move(wallet));
}

// static
void LedgerClientMojoBridge::OnShowNotification(
    CallbackHolder<ShowNotificationCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoBridge::ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ShowNotificationCallback callback) {
  auto* holder = new CallbackHolder<ShowNotificationCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->ShowNotification(
      type,
      args,
      std::bind(LedgerClientMojoBridge::OnShowNotification,
                holder,
                _1));
}

void LedgerClientMojoBridge::GetTransferFees(
    const std::string& wallet_type,
    GetTransferFeesCallback callback) {
  auto list = ledger_client_->GetTransferFees(wallet_type);
  std::move(callback).Run(base::MapToFlatMap(std::move(list)));
}

void LedgerClientMojoBridge::SetTransferFee(
    const std::string& wallet_type,
    ledger::TransferFeePtr transfer_fee) {
  ledger_client_->SetTransferFee(wallet_type, std::move(transfer_fee));
}

void LedgerClientMojoBridge::RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id) {
  ledger_client_->RemoveTransferFee(wallet_type, id);
}

void LedgerClientMojoBridge::GetClientInfo(
    GetClientInfoCallback callback) {
  auto info = ledger_client_->GetClientInfo();
  std::move(callback).Run(std::move(info));
}

void LedgerClientMojoBridge::UnblindedTokensReady() {
  ledger_client_->UnblindedTokensReady();
}

void LedgerClientMojoBridge::ReconcileStampReset() {
  ledger_client_->ReconcileStampReset();
}

// static
void LedgerClientMojoBridge::OnRunDBTransaction(
    CallbackHolder<RunDBTransactionCallback>* holder,
    ledger::DBCommandResponsePtr response) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(response));
  }
  delete holder;
}

void LedgerClientMojoBridge::RunDBTransaction(
    ledger::DBTransactionPtr transaction,
    RunDBTransactionCallback callback) {
  auto* holder = new CallbackHolder<RunDBTransactionCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->RunDBTransaction(
      std::move(transaction),
      std::bind(LedgerClientMojoBridge::OnRunDBTransaction,
                holder,
                _1));
}

// static
void LedgerClientMojoBridge::OnGetCreateScript(
    CallbackHolder<GetCreateScriptCallback>* holder,
    const std::string& script,
    const int table_version) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(script, table_version);
  }
  delete holder;
}

void LedgerClientMojoBridge::GetCreateScript(
    GetCreateScriptCallback callback) {
  auto* holder = new CallbackHolder<GetCreateScriptCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetCreateScript(
      std::bind(LedgerClientMojoBridge::OnGetCreateScript,
                holder,
                _1,
                _2));
}

void LedgerClientMojoBridge::PendingContributionSaved(
    const ledger::Result result) {
  ledger_client_->PendingContributionSaved(result);
}

void LedgerClientMojoBridge::Log(
    const std::string& file,
    const int32_t line,
    const int32_t verbose_level,
    const std::string& message) {
  ledger_client_->Log(file.c_str(), line, verbose_level, message);
}

void LedgerClientMojoBridge::ClearAllNotifications() {
  ledger_client_->ClearAllNotifications();
}

// static
void LedgerClientMojoBridge::OnDeleteLog(
    CallbackHolder<DeleteLogCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoBridge::DeleteLog(DeleteLogCallback callback) {
  auto* holder = new CallbackHolder<DeleteLogCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->DeleteLog(
      std::bind(LedgerClientMojoBridge::OnDeleteLog,
                holder,
                _1));
}

}  // namespace bat_ledger
