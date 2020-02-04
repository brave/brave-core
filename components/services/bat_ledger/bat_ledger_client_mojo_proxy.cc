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

void OnSaveState(const ledger::OnSaveCallback& callback,
                 const ledger::Result result) {
  callback(result);
}

void OnLoadState(const ledger::OnLoadCallback& callback,
                 const ledger::Result result,
                 const std::string& value) {
  callback(result, value);
}

void OnResetState(const ledger::OnSaveCallback& callback,
                  const ledger::Result result) {
  callback(result);
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

std::string BatLedgerClientMojoProxy::GenerateGUID() const {
  if (!Connected())
    return "";

  std::string guid;
  bat_ledger_client_->GenerateGUID(&guid);
  return guid;
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
    const std::string& viewing_id,
    const double amount,
    const ledger::RewardsType type) {
  if (!Connected())
    return;

  bat_ledger_client_->OnReconcileComplete(
      result,
      viewing_id,
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

void BatLedgerClientMojoProxy::OnSaveLedgerState(
    ledger::LedgerCallbackHandler* handler,
    const ledger::Result result) {
  handler->OnLedgerStateSaved(result);
}

void BatLedgerClientMojoProxy::SaveLedgerState(
    const std::string& ledger_state, ledger::LedgerCallbackHandler* handler) {
  if (!Connected()) {
    handler->OnLedgerStateSaved(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->SaveLedgerState(ledger_state,
      base::BindOnce(&BatLedgerClientMojoProxy::OnSaveLedgerState,
        AsWeakPtr(), base::Unretained(handler)));
}

void BatLedgerClientMojoProxy::OnSavePublisherState(
    ledger::LedgerCallbackHandler* handler,
    const ledger::Result result) {
  handler->OnPublisherStateSaved(result);
}

void BatLedgerClientMojoProxy::SavePublisherState(
    const std::string& publisher_state,
    ledger::LedgerCallbackHandler* handler) {
  if (!Connected()) {
    handler->OnPublisherStateSaved(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->SavePublisherState(publisher_state,
      base::BindOnce(&BatLedgerClientMojoProxy::OnSavePublisherState,
        AsWeakPtr(), base::Unretained(handler)));
}

void OnSavePublisherInfo(
    const ledger::PublisherInfoCallback& callback,
    const ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(result, std::move(publisher_info));
}

void BatLedgerClientMojoProxy::SavePublisherInfo(
    ledger::PublisherInfoPtr publisher_info,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  bat_ledger_client_->SavePublisherInfo(
      std::move(publisher_info),
      base::BindOnce(&OnSavePublisherInfo, std::move(callback)));
}

void OnLoadPublisherInfo(
    const ledger::PublisherInfoCallback& callback,
    const ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(result, std::move(publisher_info));
}

void BatLedgerClientMojoProxy::LoadPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  bat_ledger_client_->LoadPublisherInfo(publisher_key,
      base::BindOnce(&OnLoadPublisherInfo, std::move(callback)));
}

void OnLoadPanelPublisherInfo(
    const ledger::PublisherInfoCallback& callback,
    const ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(result, std::move(publisher_info));
}

void BatLedgerClientMojoProxy::LoadPanelPublisherInfo(
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->LoadPanelPublisherInfo(std::move(filter),
      base::BindOnce(&OnLoadPanelPublisherInfo, std::move(callback)));
}

void OnLoadMediaPublisherInfo(
    const ledger::PublisherInfoCallback& callback,
    const ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(result, std::move(publisher_info));
}

void BatLedgerClientMojoProxy::LoadMediaPublisherInfo(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  bat_ledger_client_->LoadMediaPublisherInfo(media_key,
      base::BindOnce(&OnLoadMediaPublisherInfo, std::move(callback)));
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

void OnSaveRecurringTip(const ledger::SaveRecurringTipCallback& callback,
                        const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::SaveRecurringTip(
    ledger::RecurringTipPtr info,
    ledger::SaveRecurringTipCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->SaveRecurringTip(
      std::move(info),
      base::BindOnce(&OnSaveRecurringTip, std::move(callback)));
}

void OnGetRecurringTips(const ledger::PublisherInfoListCallback& callback,
                        ledger::PublisherInfoList publisher_info_list,
                        uint32_t next_record) {
  callback(std::move(publisher_info_list), next_record);
}

void BatLedgerClientMojoProxy::GetRecurringTips(
    ledger::PublisherInfoListCallback callback) {
  if (!Connected()) {
    callback(ledger::PublisherInfoList(), 0);
    return;
  }

  bat_ledger_client_->GetRecurringTips(
      base::BindOnce(&OnGetRecurringTips, std::move(callback)));
}

void OnGetOneTimeTips(const ledger::PublisherInfoListCallback& callback,
                      ledger::PublisherInfoList publisher_info_list,
                      uint32_t next_record) {
  callback(std::move(publisher_info_list), next_record);
}

void BatLedgerClientMojoProxy::GetOneTimeTips(
    ledger::PublisherInfoListCallback callback) {
  if (!Connected()) {
    callback(ledger::PublisherInfoList(), 0);
    return;
  }

  bat_ledger_client_->GetOneTimeTips(
      base::BindOnce(&OnGetOneTimeTips, std::move(callback)));
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

void OnRemoveRecurringTip(
    const ledger::RemoveRecurringTipCallback& callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::RemoveRecurringTipCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->RemoveRecurringTip(publisher_key,
      base::BindOnce(&OnRemoveRecurringTip, std::move(callback)));
}

void OnSaveContributionInfo(
    const ledger::ResultCallback& callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::SaveContributionInfo(
    ledger::ContributionInfoPtr info,
    ledger::ResultCallback callback) {
  if (!Connected())
    return;

  bat_ledger_client_->SaveContributionInfo(
      std::move(info),
      base::BindOnce(&OnSaveContributionInfo, std::move(callback)));
}

void BatLedgerClientMojoProxy::SaveMediaPublisherInfo(
    const std::string& media_key, const std::string& publisher_id) {
  if (!Connected())
    return;

  bat_ledger_client_->SaveMediaPublisherInfo(media_key, publisher_id);
}

std::string BatLedgerClientMojoProxy::URIEncode(const std::string& value) {
  if (!Connected())
    return "";

  std::string encoded_value;
  bat_ledger_client_->URIEncode(value, &encoded_value);
  return encoded_value;
}

void OnSavePendingContribution(
    const ledger::SavePendingContributionCallback& callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::SavePendingContribution(
    ledger::PendingContributionList list,
    ledger::SavePendingContributionCallback callback) {
  if (!Connected())
    return;

  bat_ledger_client_->SavePendingContribution(
      std::move(list),
      base::BindOnce(&OnSavePendingContribution, std::move(callback)));
}

void OnLoadActivityInfo(
    const ledger::PublisherInfoCallback& callback,
    const ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(result, std::move(publisher_info));
}

void BatLedgerClientMojoProxy::LoadActivityInfo(
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  bat_ledger_client_->LoadActivityInfo(std::move(filter),
      base::BindOnce(&OnLoadActivityInfo, std::move(callback)));
}

void OnSaveActivityInfo(
    const ledger::PublisherInfoCallback& callback,
    const ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(result, std::move(publisher_info));
}

void BatLedgerClientMojoProxy::SaveActivityInfo(
    ledger::PublisherInfoPtr publisher_info,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  bat_ledger_client_->SaveActivityInfo(
      std::move(publisher_info),
      base::BindOnce(&OnSaveActivityInfo, std::move(callback)));
}

void OnRestorePublishers(
    const ledger::RestorePublishersCallback& callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::RestorePublishers(
    ledger::RestorePublishersCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->RestorePublishers(
      base::BindOnce(&OnRestorePublishers, std::move(callback)));
}

void OnGetActivityInfoList(const ledger::PublisherInfoListCallback& callback,
    ledger::PublisherInfoList publisher_info_list,
    uint32_t next_record) {
  callback(std::move(publisher_info_list), next_record);
}

void BatLedgerClientMojoProxy::GetActivityInfoList(uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  if (!Connected()) {
    callback(ledger::PublisherInfoList(), 0);
    return;
  }

  bat_ledger_client_->GetActivityInfoList(start,
      limit,
      std::move(filter),
      base::BindOnce(&OnGetActivityInfoList, std::move(callback)));
}


void BatLedgerClientMojoProxy::SaveNormalizedPublisherList(
    ledger::PublisherInfoList normalized_list) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->SaveNormalizedPublisherList(std::move(normalized_list));
}

void BatLedgerClientMojoProxy::SaveState(
    const std::string& name,
    const std::string& value,
    ledger::OnSaveCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->SaveState(
      name, value,
      base::BindOnce(&OnSaveState, std::move(callback)));
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
    ledger::OnResetCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->ResetState(
      name, base::BindOnce(&OnResetState, std::move(callback)));
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

void OnGetPendingContributions(
    const ledger::PendingContributionInfoListCallback& callback,
    ledger::PendingContributionInfoList list) {
  callback(std::move(list));
}

void BatLedgerClientMojoProxy::GetPendingContributions(
    ledger::PendingContributionInfoListCallback callback) {
  if (!Connected()) {
    callback(ledger::PendingContributionInfoList());
    return;
  }

  bat_ledger_client_->GetPendingContributions(
      base::BindOnce(&OnGetPendingContributions, std::move(callback)));
}

void OnRemovePendingContribution(
    const ledger::RemovePendingContributionCallback& callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::RemovePendingContribution(
    const uint64_t id,
    ledger::RemovePendingContributionCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->RemovePendingContribution(
      id,
      base::BindOnce(&OnRemovePendingContribution, std::move(callback)));
}

void OnRemoveAllPendingContributions(
    const ledger::RemovePendingContributionCallback& callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::RemoveAllPendingContributions(
    ledger::RemovePendingContributionCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->RemoveAllPendingContributions(
      base::BindOnce(&OnRemoveAllPendingContributions, std::move(callback)));
}

void OnGetPendingContributionsTotal(
    const ledger::PendingContributionsTotalCallback& callback,
    double amount) {
  callback(amount);
}

void BatLedgerClientMojoProxy::GetPendingContributionsTotal(
    ledger::PendingContributionsTotalCallback callback) {
  if (!Connected()) {
    callback(0.0);
    return;
  }

  bat_ledger_client_->GetPendingContributionsTotal(
      base::BindOnce(&OnGetPendingContributionsTotal, std::move(callback)));
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
    const ledger::ShowNotificationCallback& callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::ShowNotification(
    const std::string& type,
    const std::vector<std::string>& args,
    ledger::ShowNotificationCallback callback) {
  bat_ledger_client_->ShowNotification(
      type,
      args,
      base::BindOnce(&OnShowNotification, std::move(callback)));
}

void OnDeleteActivityInfo(
  const ledger::DeleteActivityInfoCallback& callback,
  const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::DeleteActivityInfo(
    const std::string& publisher_key,
    ledger::DeleteActivityInfoCallback callback) {
  bat_ledger_client_->DeleteActivityInfo(
      publisher_key,
      base::BindOnce(&OnDeleteActivityInfo, std::move(callback)));
}

void OnClearAndInsertServerPublisherList(
  const ledger::ClearAndInsertServerPublisherListCallback& callback,
  const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::ClearAndInsertServerPublisherList(
    ledger::ServerPublisherInfoList list,
    ledger::ClearAndInsertServerPublisherListCallback callback) {
  bat_ledger_client_->ClearAndInsertServerPublisherList(
      std::move(list),
      base::BindOnce(&OnClearAndInsertServerPublisherList,
          std::move(callback)));
}

void OnGetServerPublisherInfo(
  const ledger::GetServerPublisherInfoCallback& callback,
  ledger::ServerPublisherInfoPtr info) {
  callback(std::move(info));
}

void BatLedgerClientMojoProxy::GetServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) {
  bat_ledger_client_->GetServerPublisherInfo(
      publisher_key,
      base::BindOnce(&OnGetServerPublisherInfo, std::move(callback)));
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

void BatLedgerClientMojoProxy::InsertOrUpdateContributionQueue(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback) {
  bat_ledger_client_->InsertOrUpdateContributionQueue(
      std::move(info),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void BatLedgerClientMojoProxy::DeleteContributionQueue(
    const uint64_t id,
    ledger::ResultCallback callback) {
  bat_ledger_client_->DeleteContributionQueue(
      id,
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void OnGetFirstContributionQueue(
    const ledger::GetFirstContributionQueueCallback& callback,
    ledger::ContributionQueuePtr info) {
  callback(std::move(info));
}

void BatLedgerClientMojoProxy::GetFirstContributionQueue(
    ledger::GetFirstContributionQueueCallback callback) {
  bat_ledger_client_->GetFirstContributionQueue(
      base::BindOnce(&OnGetFirstContributionQueue, std::move(callback)));
}

void BatLedgerClientMojoProxy::InsertOrUpdatePromotion(
    ledger::PromotionPtr info,
    ledger::ResultCallback callback) {
  bat_ledger_client_->InsertOrUpdatePromotion(
      std::move(info),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void OnGetPromotion(
    const ledger::GetPromotionCallback& callback,
    ledger::PromotionPtr info) {
  callback(std::move(info));
}

void BatLedgerClientMojoProxy::GetPromotion(
    const std::string& id,
    ledger::GetPromotionCallback callback) {
  bat_ledger_client_->GetPromotion(
      id,
      base::BindOnce(&OnGetPromotion, std::move(callback)));
}
void OnGetAllPromotions(
    const ledger::GetAllPromotionsCallback& callback,
    base::flat_map<std::string, ledger::PromotionPtr> promotions) {
  callback(base::FlatMapToMap(std::move(promotions)));
}

void BatLedgerClientMojoProxy::GetAllPromotions(
    ledger::GetAllPromotionsCallback callback) {
  bat_ledger_client_->GetAllPromotions(
      base::BindOnce(&OnGetAllPromotions, std::move(callback)));
}

void BatLedgerClientMojoProxy::InsertOrUpdateUnblindedToken(
    ledger::UnblindedTokenPtr info,
    ledger::ResultCallback callback) {
  bat_ledger_client_->InsertOrUpdateUnblindedToken(
      std::move(info),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void OnGetAllUnblindedTokens(
    const ledger::GetAllUnblindedTokensCallback& callback,
    ledger::UnblindedTokenList list) {
  callback(std::move(list));
}

void BatLedgerClientMojoProxy::GetAllUnblindedTokens(
    ledger::GetAllUnblindedTokensCallback callback) {
  bat_ledger_client_->GetAllUnblindedTokens(
      base::BindOnce(&OnGetAllUnblindedTokens, std::move(callback)));
}

void BatLedgerClientMojoProxy::DeleteUnblindedTokens(
    const std::vector<std::string>& id_list,
    ledger::ResultCallback callback) {
  bat_ledger_client_->DeleteUnblindedTokens(
      id_list,
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void BatLedgerClientMojoProxy::DeleteUnblindedTokensForPromotion(
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  bat_ledger_client_->DeleteUnblindedTokensForPromotion(
      promotion_id,
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

ledger::ClientInfoPtr BatLedgerClientMojoProxy::GetClientInfo() {
  auto info = ledger::ClientInfo::New();
  bat_ledger_client_->GetClientInfo(&info);
  return info;
}

void BatLedgerClientMojoProxy::UnblindedTokensReady() {
  bat_ledger_client_->UnblindedTokensReady();
}

void OnGetTransactionReport(
    ledger::GetTransactionReportCallback callback,
    ledger::TransactionReportInfoList list) {
  callback(std::move(list));
}

void BatLedgerClientMojoProxy::GetTransactionReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetTransactionReportCallback callback) {
  bat_ledger_client_->GetTransactionReport(
      month,
      year,
      base::BindOnce(&OnGetTransactionReport, std::move(callback)));
}

void OnGetContributionReport(
    ledger::GetContributionReportCallback callback,
    ledger::ContributionReportInfoList list) {
  callback(std::move(list));
}

void BatLedgerClientMojoProxy::GetContributionReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetContributionReportCallback callback) {
  bat_ledger_client_->GetContributionReport(
      month,
      year,
      base::BindOnce(&OnGetContributionReport, std::move(callback)));
}

void OnGetNotCompletedContributions(
    ledger::GetIncompleteContributionsCallback callback,
    ledger::ContributionInfoList list) {
  callback(std::move(list));
}

void BatLedgerClientMojoProxy::GetIncompleteContributions(
    ledger::GetIncompleteContributionsCallback callback) {
  bat_ledger_client_->GetIncompleteContributions(
      base::BindOnce(&OnGetNotCompletedContributions, std::move(callback)));
}

void OnGetContributionInfo(
    ledger::GetContributionInfoCallback callback,
    ledger::ContributionInfoPtr info) {
  callback(std::move(info));
}

void BatLedgerClientMojoProxy::GetContributionInfo(
    const std::string& contribution_id,
    ledger::GetContributionInfoCallback callback) {
  bat_ledger_client_->GetContributionInfo(
      contribution_id,
      base::BindOnce(&OnGetContributionInfo, std::move(callback)));
}

void OnUpdateContributionInfoStepAndCount(
    ledger::ResultCallback callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::UpdateContributionInfoStepAndCount(
    const std::string& contribution_id,
    const ledger::ContributionStep step,
    const int32_t retry_count,
    ledger::ResultCallback callback) {
  bat_ledger_client_->UpdateContributionInfoStepAndCount(
      contribution_id,
      step,
      retry_count,
      base::BindOnce(&OnUpdateContributionInfoStepAndCount,
          std::move(callback)));
}

void OnUpdateContributionInfoContributedAmount(
    ledger::ResultCallback callback,
    const ledger::Result result) {
  callback(result);
}

void BatLedgerClientMojoProxy::UpdateContributionInfoContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  bat_ledger_client_->UpdateContributionInfoContributedAmount(
      contribution_id,
      publisher_key,
      base::BindOnce(&OnUpdateContributionInfoContributedAmount,
          std::move(callback)));
}

void BatLedgerClientMojoProxy::ReconcileStampReset() {
  bat_ledger_client_->ReconcileStampReset();
}

}  // namespace bat_ledger
