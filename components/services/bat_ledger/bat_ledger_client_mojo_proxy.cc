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
#include "mojo/public/cpp/bindings/map.h"

namespace bat_ledger {

namespace {

int32_t ToMojomResult(ledger::Result result) {
  return (int32_t)result;
}

ledger::Result ToLedgerResult(int32_t result) {
  return (ledger::Result)result;
}

int32_t ToMojomPublisherCategory(ledger::REWARDS_CATEGORY category) {
  return (int32_t)category;
}

int32_t ToMojomMethod(ledger::URL_METHOD method) {
  return (int32_t)method;
}

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

void OnSaveState(const ledger::OnSaveCallback& callback,
                 int32_t result) {
  callback(ToLedgerResult(result));
}

void OnLoadState(const ledger::OnLoadCallback& callback,
                 int32_t result,
                 const std::string& value) {
  callback(ToLedgerResult(result), value);
}

void OnResetState(const ledger::OnSaveCallback& callback,
                  int32_t result) {
  callback(ToLedgerResult(result));
}

void OnGetExternalWallets(
    ledger::GetExternalWalletsCallback callback,
    base::flat_map<std::string, ledger::ExternalWalletPtr> wallets) {
  callback(mojo::FlatMapToMap(std::move(wallets)));
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
  callback(response_code, response, mojo::FlatMapToMap(headers));
}

void BatLedgerClientMojoProxy::LoadURL(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    const ledger::URL_METHOD method,
    ledger::LoadURLCallback callback) {
  if (!Connected())
    return;

  bat_ledger_client_->LoadURL(url, headers, content, contentType,
      ToMojomMethod(method), base::BindOnce(&OnLoadURL, std::move(callback)));
}

void BatLedgerClientMojoProxy::OnWalletInitialized(ledger::Result result) {
  if (!Connected())
    return;

  bat_ledger_client_->OnWalletInitialized(ToMojomResult(result));
}

void BatLedgerClientMojoProxy::OnWalletProperties(
    ledger::Result result,
    ledger::WalletPropertiesPtr properties) {
  if (!Connected())
    return;

  bat_ledger_client_->OnWalletProperties(ToMojomResult(result),
                                         std::move(properties));
}

void BatLedgerClientMojoProxy::OnGrant(ledger::Result result,
                                       ledger::GrantPtr grant) {
  if (!Connected())
    return;

  bat_ledger_client_->OnGrant(ToMojomResult(result), std::move(grant));
}

void BatLedgerClientMojoProxy::OnGrantCaptcha(const std::string& image,
    const std::string& hint) {
  if (!Connected())
    return;

  bat_ledger_client_->OnGrantCaptcha(image, hint);
}

void BatLedgerClientMojoProxy::OnRecoverWallet(
    ledger::Result result,
    double balance,
    std::vector<ledger::GrantPtr> grants) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->OnRecoverWallet(ToMojomResult(result),
                                      balance,
                                      std::move(grants));
}

void BatLedgerClientMojoProxy::OnReconcileComplete(ledger::Result result,
    const std::string& viewing_id,
    ledger::REWARDS_CATEGORY category,
    const std::string& probi) {
  if (!Connected())
    return;

  bat_ledger_client_->OnReconcileComplete(ToMojomResult(result), viewing_id,
      ToMojomPublisherCategory(category), probi);
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

void BatLedgerClientMojoProxy::OnGrantFinish(ledger::Result result,
                                             ledger::GrantPtr grant) {
  if (!Connected())
    return;

  bat_ledger_client_->OnGrantFinish(ToMojomResult(result), std::move(grant));
}

void BatLedgerClientMojoProxy::OnLoadLedgerState(
    ledger::OnLoadCallback callback,
    int32_t result,
    const std::string& data) {
  callback(ToLedgerResult(result), data);
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
    int32_t result, const std::string& data) {
  callback(ToLedgerResult(result), data);
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

void BatLedgerClientMojoProxy::OnLoadPublisherList(
    ledger::LedgerCallbackHandler* handler,
    int32_t result, const std::string& data) {
  handler->OnPublisherListLoaded(ToLedgerResult(result), data);
}

void BatLedgerClientMojoProxy::LoadPublisherList(
    ledger::LedgerCallbackHandler* handler) {
  if (!Connected()) {
    handler->OnPublisherListLoaded(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  bat_ledger_client_->LoadPublisherList(
      base::BindOnce(&BatLedgerClientMojoProxy::OnLoadPublisherList,
        AsWeakPtr(), base::Unretained(handler)));
}

void BatLedgerClientMojoProxy::OnSaveLedgerState(
    ledger::LedgerCallbackHandler* handler,
    int32_t result) {
  handler->OnLedgerStateSaved(ToLedgerResult(result));
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
    int32_t result) {
  handler->OnPublisherStateSaved(ToLedgerResult(result));
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

void BatLedgerClientMojoProxy::OnSavePublishersList(
    ledger::LedgerCallbackHandler* handler,
    int32_t result) {
  handler->OnPublishersListSaved(ToLedgerResult(result));
}

void BatLedgerClientMojoProxy::SavePublishersList(
    const std::string& publishers_list,
    ledger::LedgerCallbackHandler* handler) {
  if (!Connected()) {
    handler->OnPublishersListSaved(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->SavePublishersList(publishers_list,
      base::BindOnce(&BatLedgerClientMojoProxy::OnSavePublishersList,
        AsWeakPtr(), base::Unretained(handler)));
}

void OnSavePublisherInfo(
    const ledger::PublisherInfoCallback& callback,
    int32_t result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(ToLedgerResult(result), std::move(publisher_info));
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

void OnLoadPublisherInfo(const ledger::PublisherInfoCallback& callback,
    int32_t result, ledger::PublisherInfoPtr publisher_info) {
  callback(ToLedgerResult(result), std::move(publisher_info));
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
    int32_t result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(ToLedgerResult(result), std::move(publisher_info));
}

void BatLedgerClientMojoProxy::LoadPanelPublisherInfo(
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->LoadPanelPublisherInfo(filter.ToJson(),
      base::BindOnce(&OnLoadPanelPublisherInfo, std::move(callback)));
}

void OnLoadMediaPublisherInfo(
    const ledger::PublisherInfoCallback& callback,
    int32_t result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(ToLedgerResult(result), std::move(publisher_info));
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

void BatLedgerClientMojoProxy::OnExcludedSitesChanged(
    const std::string& publisher_id,
    ledger::PUBLISHER_EXCLUDE exclude) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->OnExcludedSitesChanged(publisher_id, exclude);
}

void BatLedgerClientMojoProxy::OnPanelPublisherInfo(
    ledger::Result result,
    ledger::PublisherInfoPtr info,
    uint64_t windowId) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->OnPanelPublisherInfo(
      ToMojomResult(result), std::move(info), windowId);
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

void OnLoadNicewareList(const ledger::GetNicewareListCallback& callback,
    int32_t result, const std::string& data) {
  callback(ToLedgerResult(result), data);
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

void OnRecurringRemoved(const ledger::RecurringRemoveCallback& callback,
    int32_t result) {
  callback(ToLedgerResult(result));
}

void BatLedgerClientMojoProxy::OnRemoveRecurring(
    const std::string& publisher_key,
    ledger::RecurringRemoveCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->OnRemoveRecurring(publisher_key,
      base::BindOnce(&OnRecurringRemoved, std::move(callback)));
}

void BatLedgerClientMojoProxy::SaveContributionInfo(const std::string& probi,
    const int month,
    const int year,
    const uint32_t date,
    const std::string& publisher_key,
    const ledger::REWARDS_CATEGORY category) {
  if (!Connected())
    return;

  bat_ledger_client_->SaveContributionInfo(probi, month, year, date,
      publisher_key, ToMojomPublisherCategory(category));
}

void BatLedgerClientMojoProxy::SaveMediaPublisherInfo(
    const std::string& media_key, const std::string& publisher_id) {
  if (!Connected())
    return;

  bat_ledger_client_->SaveMediaPublisherInfo(media_key, publisher_id);
}

void BatLedgerClientMojoProxy::FetchGrants(const std::string& lang,
    const std::string& paymentId) {
  if (!Connected())
    return;

  bat_ledger_client_->FetchGrants(lang, paymentId);
}

std::string BatLedgerClientMojoProxy::URIEncode(const std::string& value) {
  if (!Connected())
    return "";

  std::string encoded_value;
  bat_ledger_client_->URIEncode(value, &encoded_value);
  return encoded_value;
}

void BatLedgerClientMojoProxy::SavePendingContribution(
      ledger::PendingContributionList list) {
  if (!Connected())
    return;

  bat_ledger_client_->SavePendingContribution(std::move(list));
}

void OnLoadActivityInfo(
    const ledger::PublisherInfoCallback& callback,
    int32_t result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(ToLedgerResult(result), std::move(publisher_info));
}

void BatLedgerClientMojoProxy::LoadActivityInfo(
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR, nullptr);
    return;
  }

  bat_ledger_client_->LoadActivityInfo(filter.ToJson(),
      base::BindOnce(&OnLoadActivityInfo, std::move(callback)));
}

void OnSaveActivityInfo(
    const ledger::PublisherInfoCallback& callback,
    int32_t result,
    ledger::PublisherInfoPtr publisher_info) {
  callback(ToLedgerResult(result), std::move(publisher_info));
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

void OnRestorePublishersDone(const ledger::OnRestoreCallback& callback,
    bool result) {
  callback(result);
}

void BatLedgerClientMojoProxy::OnRestorePublishers(
    ledger::OnRestoreCallback callback) {
  if (!Connected()) {
    callback(false);
    return;
  }

  bat_ledger_client_->OnRestorePublishers(
      base::BindOnce(&OnRestorePublishersDone, std::move(callback)));
}

void OnGetActivityInfoList(const ledger::PublisherInfoListCallback& callback,
    ledger::PublisherInfoList publisher_info_list,
    uint32_t next_record) {
  callback(std::move(publisher_info_list), next_record);
}

void BatLedgerClientMojoProxy::GetActivityInfoList(uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoListCallback callback) {
  if (!Connected()) {
    callback(ledger::PublisherInfoList(), 0);
    return;
  }

  bat_ledger_client_->GetActivityInfoList(start,
      limit,
      filter.ToJson(),
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
    const ledger::PendingContributionInfoListCallback& callback) {
  if (!Connected()) {
    callback(ledger::PendingContributionInfoList());
    return;
  }

  bat_ledger_client_->GetPendingContributions(
      base::BindOnce(&OnGetPendingContributions, std::move(callback)));
}

void OnRemovePendingContribution(
    const ledger::RemovePendingContributionCallback& callback,
    int32_t result) {
  callback(ToLedgerResult(result));
}

void BatLedgerClientMojoProxy::RemovePendingContribution(
    const std::string& publisher_key,
    const std::string& viewing_id,
    uint64_t added_date,
    const ledger::RemovePendingContributionCallback& callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bat_ledger_client_->RemovePendingContribution(
      publisher_key,
      viewing_id,
      added_date,
      base::BindOnce(&OnRemovePendingContribution, std::move(callback)));
}

void OnRemoveAllPendingContributions(
    const ledger::RemovePendingContributionCallback& callback,
    int32_t result) {
  callback(ToLedgerResult(result));
}

void BatLedgerClientMojoProxy::RemoveAllPendingContributions(
    const ledger::RemovePendingContributionCallback& callback) {
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
    const ledger::PendingContributionsTotalCallback& callback) {
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
  bat_ledger_client_->OnContributeUnverifiedPublishers(ToMojomResult(result),
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
    int32_t result) {
  callback(ToLedgerResult(result));
}

void BatLedgerClientMojoProxy::ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      const ledger::ShowNotificationCallback& callback) {
  bat_ledger_client_->ShowNotification(
      type,
      args,
      base::BindOnce(&OnShowNotification, std::move(callback)));
}

}  // namespace bat_ledger
