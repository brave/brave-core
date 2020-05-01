/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_client_mojo_proxy.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "brave/base/containers/utils.h"

namespace bat_ledger {

namespace {

class LogStreamImpl : public ledger::LogStream {
 public:
  LogStreamImpl(const char* file,
                int line,
                const ledger::LogLevel log_level) {
    switch (log_level) {
      case ledger::LogLevel::LOG_INFO:
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_INFO);
        break;
      case ledger::LogLevel::LOG_WARNING:
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_WARNING);
        break;
      case ledger::LogLevel::LOG_ERROR:
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_ERROR);
        break;
      default:
        log_message_ = std::make_unique<logging::LogMessage>(
            file, line, logging::LOG_VERBOSE);
        break;
    }
  }

  LogStreamImpl(const char* file,
                int line,
                int level) {
    // VLOG has negative log level
    log_message_ = std::make_unique<logging::LogMessage>(file, line, -level);
  }

  std::ostream& stream() override {
    return log_message_->stream();
  }

 private:
  std::unique_ptr<logging::LogMessage> log_message_;
  DISALLOW_COPY_AND_ASSIGN(LogStreamImpl);
};

void OnResultCallback(ledger::ResultCallback callback, ledger::Result result) {
  callback(result);
}

void OnLoadState(const ledger::OnLoadCallback& callback,
                 const ledger::Result result,
                 const std::string& value) {
  callback(result, value);
}

void OnGetExternalWallets(
    ledger::GetExternalWalletsCallback callback,
    base::flat_map<std::string, ledger::ExternalWalletPtr> wallets) {
  callback(base::FlatMapToMap(std::move(wallets)));
}

}  // namespace

BatLedgerClientMojoProxy::BatLedgerClientMojoProxy(
    mojom::BatLedgerClientAssociatedPtrInfo client_info) {
  bat_ledger_client_.Bind(std::move(client_info));
}

BatLedgerClientMojoProxy::~BatLedgerClientMojoProxy() {
}

void OnLoadURL(const ledger::LoadURLCallback& callback,
    int32_t response_code, const std::string& response,
    const base::flat_map<std::string, std::string>& headers) {
  callback(response_code, response, base::FlatMapToMap(headers));
}

void BatLedgerClientMojoProxy::LoadURL(
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

void BatLedgerClientMojoProxy::OnWalletProperties(
    ledger::Result result,
    ledger::WalletPropertiesPtr properties) {
  if (!Connected())
    return;

  bat_ledger_client_->OnWalletProperties(result,
                                         std::move(properties));
}

void BatLedgerClientMojoProxy::OnReconcileComplete(
    ledger::Result result,
    const std::string& contribution_id,
    const double amount,
    const ledger::RewardsType type) {
  if (!Connected())
    return;

  bat_ledger_client_->OnReconcileComplete(
      result,
      contribution_id,
      amount,
      type);
}

std::unique_ptr<ledger::LogStream> BatLedgerClientMojoProxy::Log(
    const char* file, int line, ledger::LogLevel level) const {
  // There's no need to proxy this
  return std::make_unique<LogStreamImpl>(file, line, level);
}

std::unique_ptr<ledger::LogStream> BatLedgerClientMojoProxy::VerboseLog(
    const char* file, int line, int level) const {
  // There's no need to proxy this
  return std::make_unique<LogStreamImpl>(file, line, level);
}

void BatLedgerClientMojoProxy::OnLoadLedgerState(
    ledger::OnLoadCallback callback,
    const ledger::Result result,
    const std::string& data) {
  callback(result, data);
}

void BatLedgerClientMojoProxy::LoadLedgerState(
    ledger::OnLoadCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  bat_ledger_client_->LoadLedgerState(
      base::BindOnce(&BatLedgerClientMojoProxy::OnLoadLedgerState, AsWeakPtr(),
        std::move(callback)));
}

void BatLedgerClientMojoProxy::OnLoadPublisherState(
    ledger::OnLoadCallback callback,
    const ledger::Result result,
    const std::string& data) {
  callback(result, data);
}

void BatLedgerClientMojoProxy::LoadPublisherState(
    ledger::OnLoadCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  bat_ledger_client_->LoadPublisherState(
      base::BindOnce(&BatLedgerClientMojoProxy::OnLoadPublisherState,
        AsWeakPtr(), std::move(callback)));
}

void OnSaveLedgerState(
    ledger::ResultCallback callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::SaveLedgerState(
    const std::string& ledger_state,
    ledger::ResultCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->SaveLedgerState(
      ledger_state,
      base::BindOnce(&OnSaveLedgerState, std::move(callback)));
}

void BatLedgerClientMojoProxy::OnSavePublisherState(
    ledger::ResultCallback callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::SavePublisherState(
    const std::string& publisher_state,
    ledger::ResultCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->SavePublisherState(
      publisher_state,
      base::BindOnce(&BatLedgerClientMojoProxy::OnSavePublisherState,
        AsWeakPtr(),
        callback));
}

void BatLedgerClientMojoProxy::SetTimer(uint64_t time_offset,
    uint32_t* timer_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->SetTimer(time_offset, timer_id);  // sync
}

void BatLedgerClientMojoProxy::KillTimer(const uint32_t timer_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->KillTimer(timer_id);  // sync
}

void BatLedgerClientMojoProxy::OnPanelPublisherInfo(
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

void BatLedgerClientMojoProxy::FetchFavIcon(const std::string& url,
    const std::string& favicon_key,
    ledger::FetchIconCallback callback) {
  if (!Connected()) {
    callback(false, "");
    return;
  }

  bat_ledger_client_->FetchFavIcon(url, favicon_key,
      base::BindOnce(&OnFetchFavIcon, std::move(callback)));
}

void OnLoadNicewareList(
    const ledger::GetNicewareListCallback& callback,
    const ledger::Result result,
    const std::string& data) {
  callback(result, data);
}

void BatLedgerClientMojoProxy::LoadNicewareList(
    ledger::GetNicewareListCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  bat_ledger_client_->LoadNicewareList(
      base::BindOnce(&OnLoadNicewareList, std::move(callback)));
}

std::string BatLedgerClientMojoProxy::URIEncode(const std::string& value) {
  if (!Connected())
    return "";

  std::string encoded_value;
  bat_ledger_client_->URIEncode(value, &encoded_value);
  return encoded_value;
}

void BatLedgerClientMojoProxy::PublisherListNormalized(
    ledger::PublisherInfoList list) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->PublisherListNormalized(std::move(list));
}

void BatLedgerClientMojoProxy::SaveState(
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

void BatLedgerClientMojoProxy::LoadState(
    const std::string& name,
    ledger::OnLoadCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, std::string());
    return;
  }

  bat_ledger_client_->LoadState(
      name, base::BindOnce(&OnLoadState, std::move(callback)));
}

void BatLedgerClientMojoProxy::ResetState(
    const std::string& name,
    ledger::ResultCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->ResetState(
      name, base::BindOnce(&OnResultCallback, std::move(callback)));
}

void BatLedgerClientMojoProxy::SetBooleanState(const std::string& name,
                                               bool value) {
  bat_ledger_client_->SetBooleanState(name, value);
}

bool BatLedgerClientMojoProxy::GetBooleanState(const std::string& name) const {
  bool value;
  bat_ledger_client_->GetBooleanState(name, &value);
  return value;
}

void BatLedgerClientMojoProxy::SetIntegerState(const std::string& name,
                                               int value) {
  bat_ledger_client_->SetIntegerState(name, value);
}

int BatLedgerClientMojoProxy::GetIntegerState(const std::string& name) const {
  int value;
  bat_ledger_client_->GetIntegerState(name, &value);
  return value;
}

void BatLedgerClientMojoProxy::SetDoubleState(const std::string& name,
                                              double value) {
  bat_ledger_client_->SetDoubleState(name, value);
}

double BatLedgerClientMojoProxy::GetDoubleState(const std::string& name) const {
  double value;
  bat_ledger_client_->GetDoubleState(name, &value);
  return value;
}

void BatLedgerClientMojoProxy::SetStringState(const std::string& name,
                              const std::string& value) {
  bat_ledger_client_->SetStringState(name, value);
}

std::string BatLedgerClientMojoProxy::
GetStringState(const std::string& name) const {
  std::string value;
  bat_ledger_client_->GetStringState(name, &value);
  return value;
}

void BatLedgerClientMojoProxy::SetInt64State(const std::string& name,
                                             int64_t value) {
  bat_ledger_client_->SetInt64State(name, value);
}

int64_t BatLedgerClientMojoProxy::GetInt64State(const std::string& name) const {
  int64_t value;
  bat_ledger_client_->GetInt64State(name, &value);
  return value;
}

void BatLedgerClientMojoProxy::SetUint64State(const std::string& name,
                                              uint64_t value) {
  bat_ledger_client_->SetUint64State(name, value);
}

uint64_t BatLedgerClientMojoProxy::GetUint64State(
    const std::string& name) const {
  uint64_t value;
  bat_ledger_client_->GetUint64State(name, &value);
  return value;
}

void BatLedgerClientMojoProxy::ClearState(const std::string& name) {
  bat_ledger_client_->ClearState(name);
}

bool BatLedgerClientMojoProxy::GetBooleanOption(const std::string& name) const {
  bool value;
  bat_ledger_client_->GetBooleanOption(name, &value);
  return value;
}

int BatLedgerClientMojoProxy::GetIntegerOption(const std::string& name) const {
  int value;
  bat_ledger_client_->GetIntegerOption(name, &value);
  return value;
}

double BatLedgerClientMojoProxy::GetDoubleOption(
    const std::string& name) const {
  double value;
  bat_ledger_client_->GetDoubleOption(name, &value);
  return value;
}

std::string BatLedgerClientMojoProxy::GetStringOption(
    const std::string& name) const {
  std::string value;
  bat_ledger_client_->GetStringOption(name, &value);
  return value;
}

int64_t BatLedgerClientMojoProxy::GetInt64Option(
    const std::string& name) const {
  int64_t value;
  bat_ledger_client_->GetInt64Option(name, &value);
  return value;
}

uint64_t BatLedgerClientMojoProxy::GetUint64Option(
    const std::string& name) const {
  uint64_t value;
  bat_ledger_client_->GetUint64Option(name, &value);
  return value;
}

void BatLedgerClientMojoProxy::SetConfirmationsIsReady(const bool is_ready) {
  if (!Connected())
    return;

  bat_ledger_client_->SetConfirmationsIsReady(is_ready);
}

void BatLedgerClientMojoProxy::ConfirmationsTransactionHistoryDidChange() {
  if (!Connected())
    return;

  bat_ledger_client_->ConfirmationsTransactionHistoryDidChange();
}

bool BatLedgerClientMojoProxy::Connected() const {
  return bat_ledger_client_.is_bound();
}

void BatLedgerClientMojoProxy::OnContributeUnverifiedPublishers(
      ledger::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) {
  bat_ledger_client_->OnContributeUnverifiedPublishers(
      result,
      publisher_key,
      publisher_name);
}

void BatLedgerClientMojoProxy::GetExternalWallets(
    ledger::GetExternalWalletsCallback callback) {
  if (!Connected()) {
    std::map<std::string, ledger::ExternalWalletPtr> wallets;
    callback(std::move(wallets));
    return;
  }

  bat_ledger_client_->GetExternalWallets(
      base::BindOnce(&OnGetExternalWallets, std::move(callback)));
}

void BatLedgerClientMojoProxy::SaveExternalWallet(
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

void BatLedgerClientMojoProxy::ShowNotification(
    const std::string& type,
    const std::vector<std::string>& args,
    ledger::ResultCallback callback) {
  bat_ledger_client_->ShowNotification(
      type,
      args,
      base::BindOnce(&OnShowNotification, std::move(callback)));
}

void BatLedgerClientMojoProxy::SetTransferFee(
    const std::string& wallet_type,
    ledger::TransferFeePtr transfer_fee) {
  bat_ledger_client_->SetTransferFee(wallet_type, std::move(transfer_fee));
}

ledger::TransferFeeList BatLedgerClientMojoProxy::GetTransferFees(
    const std::string& wallet_type) {
  base::flat_map<std::string, ledger::TransferFeePtr> list;
  bat_ledger_client_->GetTransferFees(wallet_type, &list);
  return base::FlatMapToMap(std::move(list));
}

void BatLedgerClientMojoProxy::RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id) {
  bat_ledger_client_->RemoveTransferFee(wallet_type, id);
}

ledger::ClientInfoPtr BatLedgerClientMojoProxy::GetClientInfo() {
  auto info = ledger::ClientInfo::New();
  bat_ledger_client_->GetClientInfo(&info);
  return info;
}

void BatLedgerClientMojoProxy::UnblindedTokensReady() {
  bat_ledger_client_->UnblindedTokensReady();
}

void BatLedgerClientMojoProxy::ReconcileStampReset() {
  bat_ledger_client_->ReconcileStampReset();
}

void OnRunDBTransaction(
    const ledger::RunDBTransactionCallback& callback,
    ledger::DBCommandResponsePtr response) {
  callback(std::move(response));
}

void BatLedgerClientMojoProxy::RunDBTransaction(
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

void BatLedgerClientMojoProxy::GetCreateScript(
    ledger::GetCreateScriptCallback callback) {
  bat_ledger_client_->GetCreateScript(
      base::BindOnce(&OnGetCreateScript, std::move(callback)));
}

void BatLedgerClientMojoProxy::PendingContributionSaved(
    const ledger::Result result) {
  bat_ledger_client_->PendingContributionSaved(result);
}

}  // namespace bat_ledger
