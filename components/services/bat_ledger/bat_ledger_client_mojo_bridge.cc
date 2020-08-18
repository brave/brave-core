/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_client_mojo_bridge.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "brave/base/containers/utils.h"

namespace bat_ledger {

namespace {

void OnResultCallback(ledger::ResultCallback callback, ledger::Result result) {
  callback(result);
}

void OnLoadState(const ledger::OnLoadCallback& callback,
                 const ledger::Result result,
                 const std::string& value) {
  callback(result, value);
}

}  // namespace

BatLedgerClientMojoBridge::BatLedgerClientMojoBridge(
      mojo::PendingAssociatedRemote<mojom::BatLedgerClient> client_info) {
  bat_ledger_client_.Bind(std::move(client_info));
}

BatLedgerClientMojoBridge::~BatLedgerClientMojoBridge() = default;

void OnLoadURL(
    const ledger::LoadURLCallback& callback,
    ledger::UrlResponsePtr response_ptr) {
  callback(response_ptr ? *response_ptr : ledger::UrlResponse());
}

void BatLedgerClientMojoBridge::LoadURL(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    const ledger::UrlMethod method,
    ledger::LoadURLCallback callback) {
  if (!Connected())
    return;

  bat_ledger_client_->LoadURL(url, headers, content, contentType,
      method, base::BindOnce(&OnLoadURL, std::move(callback)));
}

void BatLedgerClientMojoBridge::OnReconcileComplete(
    const ledger::Result result,
    ledger::ContributionInfoPtr contribution) {
  if (!Connected())
    return;

  bat_ledger_client_->OnReconcileComplete(result, std::move(contribution));
}

void BatLedgerClientMojoBridge::Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->Log(file, line, verbose_level, message);
}

void BatLedgerClientMojoBridge::OnLoadLedgerState(
    ledger::OnLoadCallback callback,
    const ledger::Result result,
    const std::string& data) {
  callback(result, data);
}

void BatLedgerClientMojoBridge::LoadLedgerState(
    ledger::OnLoadCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  bat_ledger_client_->LoadLedgerState(
      base::BindOnce(&BatLedgerClientMojoBridge::OnLoadLedgerState,
          AsWeakPtr(),
          std::move(callback)));
}

void BatLedgerClientMojoBridge::OnLoadPublisherState(
    ledger::OnLoadCallback callback,
    const ledger::Result result,
    const std::string& data) {
  callback(result, data);
}

void BatLedgerClientMojoBridge::LoadPublisherState(
    ledger::OnLoadCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  bat_ledger_client_->LoadPublisherState(
      base::BindOnce(&BatLedgerClientMojoBridge::OnLoadPublisherState,
          AsWeakPtr(),
          std::move(callback)));
}

void BatLedgerClientMojoBridge::SetTimer(uint64_t time_offset,
    uint32_t* timer_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->SetTimer(time_offset, timer_id);  // sync
}

void BatLedgerClientMojoBridge::KillTimer(const uint32_t timer_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->KillTimer(timer_id);  // sync
}

void BatLedgerClientMojoBridge::OnPanelPublisherInfo(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    uint64_t windowId) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->OnPanelPublisherInfo(
      result,
      std::move(info),
      windowId);
}

void OnFetchFavIcon(const ledger::FetchIconCallback& callback,
    bool success, const std::string& favicon_url) {
  callback(success, favicon_url);
}

void BatLedgerClientMojoBridge::FetchFavIcon(const std::string& url,
    const std::string& favicon_key,
    ledger::FetchIconCallback callback) {
  if (!Connected()) {
    callback(false, "");
    return;
  }

  bat_ledger_client_->FetchFavIcon(url, favicon_key,
      base::BindOnce(&OnFetchFavIcon, std::move(callback)));
}

std::string BatLedgerClientMojoBridge::URIEncode(const std::string& value) {
  if (!Connected())
    return "";

  std::string encoded_value;
  bat_ledger_client_->URIEncode(value, &encoded_value);
  return encoded_value;
}

void BatLedgerClientMojoBridge::PublisherListNormalized(
    ledger::PublisherInfoList list) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->PublisherListNormalized(std::move(list));
}

void BatLedgerClientMojoBridge::SaveState(
    const std::string& name,
    const std::string& value,
    ledger::ResultCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->SaveState(
      name, value,
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void BatLedgerClientMojoBridge::LoadState(
    const std::string& name,
    ledger::OnLoadCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, std::string());
    return;
  }

  bat_ledger_client_->LoadState(
      name, base::BindOnce(&OnLoadState, std::move(callback)));
}

void BatLedgerClientMojoBridge::ResetState(
    const std::string& name,
    ledger::ResultCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->ResetState(
      name, base::BindOnce(&OnResultCallback, std::move(callback)));
}

void BatLedgerClientMojoBridge::SetBooleanState(const std::string& name,
                                               bool value) {
  bat_ledger_client_->SetBooleanState(name, value);
}

bool BatLedgerClientMojoBridge::GetBooleanState(const std::string& name) const {
  bool value;
  bat_ledger_client_->GetBooleanState(name, &value);
  return value;
}

void BatLedgerClientMojoBridge::SetIntegerState(const std::string& name,
                                               int value) {
  bat_ledger_client_->SetIntegerState(name, value);
}

int BatLedgerClientMojoBridge::GetIntegerState(const std::string& name) const {
  int value;
  bat_ledger_client_->GetIntegerState(name, &value);
  return value;
}

void BatLedgerClientMojoBridge::SetDoubleState(const std::string& name,
                                              double value) {
  bat_ledger_client_->SetDoubleState(name, value);
}

double BatLedgerClientMojoBridge::GetDoubleState(
    const std::string& name) const {
  double value;
  bat_ledger_client_->GetDoubleState(name, &value);
  return value;
}

void BatLedgerClientMojoBridge::SetStringState(const std::string& name,
                              const std::string& value) {
  bat_ledger_client_->SetStringState(name, value);
}

std::string BatLedgerClientMojoBridge::
GetStringState(const std::string& name) const {
  std::string value;
  bat_ledger_client_->GetStringState(name, &value);
  return value;
}

void BatLedgerClientMojoBridge::SetInt64State(const std::string& name,
                                             int64_t value) {
  bat_ledger_client_->SetInt64State(name, value);
}

int64_t BatLedgerClientMojoBridge::GetInt64State(
    const std::string& name) const {
  int64_t value;
  bat_ledger_client_->GetInt64State(name, &value);
  return value;
}

void BatLedgerClientMojoBridge::SetUint64State(const std::string& name,
                                              uint64_t value) {
  bat_ledger_client_->SetUint64State(name, value);
}

uint64_t BatLedgerClientMojoBridge::GetUint64State(
    const std::string& name) const {
  uint64_t value;
  bat_ledger_client_->GetUint64State(name, &value);
  return value;
}

void BatLedgerClientMojoBridge::ClearState(const std::string& name) {
  bat_ledger_client_->ClearState(name);
}

bool BatLedgerClientMojoBridge::GetBooleanOption(
    const std::string& name) const {
  bool value;
  bat_ledger_client_->GetBooleanOption(name, &value);
  return value;
}

int BatLedgerClientMojoBridge::GetIntegerOption(const std::string& name) const {
  int value;
  bat_ledger_client_->GetIntegerOption(name, &value);
  return value;
}

double BatLedgerClientMojoBridge::GetDoubleOption(
    const std::string& name) const {
  double value;
  bat_ledger_client_->GetDoubleOption(name, &value);
  return value;
}

std::string BatLedgerClientMojoBridge::GetStringOption(
    const std::string& name) const {
  std::string value;
  bat_ledger_client_->GetStringOption(name, &value);
  return value;
}

int64_t BatLedgerClientMojoBridge::GetInt64Option(
    const std::string& name) const {
  int64_t value;
  bat_ledger_client_->GetInt64Option(name, &value);
  return value;
}

uint64_t BatLedgerClientMojoBridge::GetUint64Option(
    const std::string& name) const {
  uint64_t value;
  bat_ledger_client_->GetUint64Option(name, &value);
  return value;
}

void BatLedgerClientMojoBridge::SetConfirmationsIsReady(const bool is_ready) {
  if (!Connected())
    return;

  bat_ledger_client_->SetConfirmationsIsReady(is_ready);
}

void BatLedgerClientMojoBridge::ConfirmationsTransactionHistoryDidChange() {
  if (!Connected())
    return;

  bat_ledger_client_->ConfirmationsTransactionHistoryDidChange();
}

bool BatLedgerClientMojoBridge::Connected() const {
  return bat_ledger_client_.is_bound();
}

void BatLedgerClientMojoBridge::OnContributeUnverifiedPublishers(
      ledger::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) {
  bat_ledger_client_->OnContributeUnverifiedPublishers(
      result,
      publisher_key,
      publisher_name);
}

std::map<std::string, ledger::ExternalWalletPtr>
BatLedgerClientMojoBridge::GetExternalWallets() {
  base::flat_map<std::string, ledger::ExternalWalletPtr> wallets;
  if (!Connected()) {
    return {};
  }

  bat_ledger_client_->GetExternalWallets(&wallets);
  return base::FlatMapToMap(std::move(wallets));
}

void BatLedgerClientMojoBridge::SaveExternalWallet(
    const std::string& wallet_type,
    ledger::ExternalWalletPtr wallet) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->SaveExternalWallet(wallet_type, std::move(wallet));
}

void OnShowNotification(
    const ledger::ResultCallback& callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoBridge::ShowNotification(
    const std::string& type,
    const std::vector<std::string>& args,
    ledger::ResultCallback callback) {
  bat_ledger_client_->ShowNotification(
      type,
      args,
      base::BindOnce(&OnShowNotification, std::move(callback)));
}

void BatLedgerClientMojoBridge::SetTransferFee(
    const std::string& wallet_type,
    ledger::TransferFeePtr transfer_fee) {
  bat_ledger_client_->SetTransferFee(wallet_type, std::move(transfer_fee));
}

ledger::TransferFeeList BatLedgerClientMojoBridge::GetTransferFees(
    const std::string& wallet_type) {
  base::flat_map<std::string, ledger::TransferFeePtr> list;
  bat_ledger_client_->GetTransferFees(wallet_type, &list);
  return base::FlatMapToMap(std::move(list));
}

void BatLedgerClientMojoBridge::RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id) {
  bat_ledger_client_->RemoveTransferFee(wallet_type, id);
}

ledger::ClientInfoPtr BatLedgerClientMojoBridge::GetClientInfo() {
  auto info = ledger::ClientInfo::New();
  bat_ledger_client_->GetClientInfo(&info);
  return info;
}

void BatLedgerClientMojoBridge::UnblindedTokensReady() {
  bat_ledger_client_->UnblindedTokensReady();
}

void BatLedgerClientMojoBridge::ReconcileStampReset() {
  bat_ledger_client_->ReconcileStampReset();
}

void OnRunDBTransaction(
    const ledger::RunDBTransactionCallback& callback,
    ledger::DBCommandResponsePtr response) {
  callback(std::move(response));
}

void BatLedgerClientMojoBridge::RunDBTransaction(
    ledger::DBTransactionPtr transaction,
    ledger::RunDBTransactionCallback callback) {
  bat_ledger_client_->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnRunDBTransaction, std::move(callback)));
}

void OnGetCreateScript(
    const ledger::GetCreateScriptCallback& callback,
    const std::string& script,
    const int table_version) {
  callback(script, table_version);
}

void BatLedgerClientMojoBridge::GetCreateScript(
    ledger::GetCreateScriptCallback callback) {
  bat_ledger_client_->GetCreateScript(
      base::BindOnce(&OnGetCreateScript, std::move(callback)));
}

void BatLedgerClientMojoBridge::PendingContributionSaved(
    const ledger::Result result) {
  bat_ledger_client_->PendingContributionSaved(result);
}

void BatLedgerClientMojoBridge::ClearAllNotifications() {
  bat_ledger_client_->ClearAllNotifications();
}

void OnDeleteLog(
    const ledger::ResultCallback callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoBridge::DeleteLog(
    ledger::ResultCallback callback) {
  bat_ledger_client_->DeleteLog(
      base::BindOnce(&OnDeleteLog, std::move(callback)));
}

}  // namespace bat_ledger
