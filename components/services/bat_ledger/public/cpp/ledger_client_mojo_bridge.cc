/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/public/cpp/ledger_client_mojo_bridge.h"

#include "base/logging.h"

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
    ledger::mojom::Result result,
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
    ledger::mojom::Result result,
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
    const ledger::mojom::Result result,
    ledger::mojom::ContributionInfoPtr contribution) {
  ledger_client_->OnReconcileComplete(
      result,
      std::move(contribution));
}

void LedgerClientMojoBridge::OnPanelPublisherInfo(
    const ledger::mojom::Result result,
    ledger::mojom::PublisherInfoPtr publisher_info,
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

void LedgerClientMojoBridge::LoadURL(ledger::mojom::UrlRequestPtr request,
                                     LoadURLCallback callback) {
  ledger_client_->LoadURL(
      std::move(request),
      base::BindOnce(
          [](LoadURLCallback callback,
             const ledger::mojom::UrlResponse& response) {
            std::move(callback).Run(ledger::mojom::UrlResponse::New(response));
          },
          std::move(callback)));
}

void LedgerClientMojoBridge::PublisherListNormalized(
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  ledger_client_->PublisherListNormalized(std::move(list));
}

void LedgerClientMojoBridge::OnPublisherRegistryUpdated() {
  ledger_client_->OnPublisherRegistryUpdated();
}

void LedgerClientMojoBridge::OnPublisherUpdated(
    const std::string& publisher_id) {
  ledger_client_->OnPublisherUpdated(publisher_id);
}

void LedgerClientMojoBridge::SetBooleanState(const std::string& name,
                                             bool value,
                                             SetBooleanStateCallback callback) {
  ledger_client_->SetBooleanState(name, value);
  std::move(callback).Run();
}

void LedgerClientMojoBridge::GetBooleanState(const std::string& name,
                                             GetBooleanStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetBooleanState(name));
}

void LedgerClientMojoBridge::SetIntegerState(const std::string& name,
                                             int value,
                                             SetIntegerStateCallback callback) {
  ledger_client_->SetIntegerState(name, value);
  std::move(callback).Run();
}

void LedgerClientMojoBridge::GetIntegerState(const std::string& name,
                                             GetIntegerStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetIntegerState(name));
}

void LedgerClientMojoBridge::SetDoubleState(const std::string& name,
                                            double value,
                                            SetDoubleStateCallback callback) {
  ledger_client_->SetDoubleState(name, value);
  std::move(callback).Run();
}

void LedgerClientMojoBridge::GetDoubleState(const std::string& name,
                                            GetDoubleStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetDoubleState(name));
}

void LedgerClientMojoBridge::SetStringState(const std::string& name,
                                            const std::string& value,
                                            SetStringStateCallback callback) {
  ledger_client_->SetStringState(name, value);
  std::move(callback).Run();
}

void LedgerClientMojoBridge::GetStringState(const std::string& name,
                                            GetStringStateCallback callback) {
  std::move(callback).Run(ledger_client_->GetStringState(name));
}

void LedgerClientMojoBridge::SetInt64State(const std::string& name,
                                           int64_t value,
                                           SetInt64StateCallback callback) {
  ledger_client_->SetInt64State(name, value);
  std::move(callback).Run();
}

void LedgerClientMojoBridge::GetInt64State(const std::string& name,
                                           GetInt64StateCallback callback) {
  std::move(callback).Run(ledger_client_->GetInt64State(name));
}

void LedgerClientMojoBridge::SetUint64State(const std::string& name,
                                            uint64_t value,
                                            SetUint64StateCallback callback) {
  ledger_client_->SetUint64State(name, value);
  std::move(callback).Run();
}

void LedgerClientMojoBridge::GetUint64State(const std::string& name,
                                            GetUint64StateCallback callback) {
  std::move(callback).Run(ledger_client_->GetUint64State(name));
}

void LedgerClientMojoBridge::ClearState(const std::string& name,
                                        ClearStateCallback callback) {
  ledger_client_->ClearState(name);
  std::move(callback).Run();
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

void LedgerClientMojoBridge::OnContributeUnverifiedPublishers(
    const ledger::mojom::Result result,
    const std::string& publisher_key,
    const std::string& publisher_name) {
  ledger_client_->OnContributeUnverifiedPublishers(
      result,
      publisher_key,
      publisher_name);
}

void LedgerClientMojoBridge::GetLegacyWallet(
    GetLegacyWalletCallback callback) {
  std::move(callback).Run(ledger_client_->GetLegacyWallet());
}

// static
void LedgerClientMojoBridge::OnShowNotification(
    CallbackHolder<ShowNotificationCallback>* holder,
    const ledger::mojom::Result result) {
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

void LedgerClientMojoBridge::RunDBTransaction(
    ledger::mojom::DBTransactionPtr transaction,
    RunDBTransactionCallback callback) {
  ledger_client_->RunDBTransaction(std::move(transaction), std::move(callback));
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
    const ledger::mojom::Result result) {
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

void LedgerClientMojoBridge::WalletDisconnected(
    const std::string& wallet_type) {
  ledger_client_->WalletDisconnected(wallet_type);
}

// static
void LedgerClientMojoBridge::OnDeleteLog(
    CallbackHolder<DeleteLogCallback>* holder,
    const ledger::mojom::Result result) {
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

void LedgerClientMojoBridge::EncryptString(const std::string& value,
                                           EncryptStringCallback callback) {
  std::move(callback).Run(ledger_client_->EncryptString(value));
}

void LedgerClientMojoBridge::DecryptString(const std::string& value,
                                           DecryptStringCallback callback) {
  std::move(callback).Run(ledger_client_->DecryptString(value));
}

}  // namespace bat_ledger
