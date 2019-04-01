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

void OnExcludedNumberDB(const ledger::GetExcludedPublishersNumberDBCallback& callback,
                        uint32_t result) {
  callback(result);
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

void BatLedgerClientMojoProxy::OnWalletProperties(ledger::Result result,
    std::unique_ptr<ledger::WalletInfo> info) {
  if (!Connected())
    return;

  std::string json_info = info ? info->ToJson() : "";
  bat_ledger_client_->OnWalletProperties(ToMojomResult(result), json_info);
}

void BatLedgerClientMojoProxy::OnGrant(ledger::Result result,
    const ledger::Grant& grant) {
  if (!Connected())
    return;

  bat_ledger_client_->OnGrant(ToMojomResult(result), grant.ToJson());
}

void BatLedgerClientMojoProxy::OnGrantCaptcha(const std::string& image,
    const std::string& hint) {
  if (!Connected())
    return;

  bat_ledger_client_->OnGrantCaptcha(image, hint);
}

void BatLedgerClientMojoProxy::OnRecoverWallet(ledger::Result result,
    double balance, const std::vector<ledger::Grant>& grants) {
  if (!Connected())
    return;

  std::vector<std::string> grant_jsons;
  for (auto const& grant : grants) {
    grant_jsons.push_back(grant.ToJson());
  }

  bat_ledger_client_->OnRecoverWallet(
      ToMojomResult(result), balance, grant_jsons);
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
    const ledger::Grant& grant) {
  if (!Connected())
    return;

  bat_ledger_client_->OnGrantFinish(ToMojomResult(result), grant.ToJson());
}

void BatLedgerClientMojoProxy::OnLoadLedgerState(
    ledger::LedgerCallbackHandler* handler,
    int32_t result,
    const std::string& data) {
  handler->OnLedgerStateLoaded(ToLedgerResult(result), data);
}

void BatLedgerClientMojoProxy::LoadLedgerState(
    ledger::LedgerCallbackHandler* handler) {
  if (!Connected()) {
    handler->OnLedgerStateLoaded(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  bat_ledger_client_->LoadLedgerState(
      base::BindOnce(&BatLedgerClientMojoProxy::OnLoadLedgerState, AsWeakPtr(),
        base::Unretained(handler)));
}

void BatLedgerClientMojoProxy::OnLoadPublisherState(
    ledger::LedgerCallbackHandler* handler,
    int32_t result, const std::string& data) {
  handler->OnPublisherStateLoaded(ToLedgerResult(result), data);
}

void BatLedgerClientMojoProxy::LoadPublisherState(
    ledger::LedgerCallbackHandler* handler) {
  if (!Connected()) {
    handler->OnPublisherStateLoaded(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  bat_ledger_client_->LoadPublisherState(
      base::BindOnce(&BatLedgerClientMojoProxy::OnLoadPublisherState,
        AsWeakPtr(), base::Unretained(handler)));
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

void OnSavePublisherInfo(const ledger::PublisherInfoCallback& callback,
    int32_t result, const std::string& publisher_info) {
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!publisher_info.empty()) {
    info.reset(new ledger::PublisherInfo());
    info->loadFromJson(publisher_info);
  }
  callback(ToLedgerResult(result), std::move(info));
}

void BatLedgerClientMojoProxy::SavePublisherInfo(
    std::unique_ptr<ledger::PublisherInfo> publisher_info,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR,
        std::unique_ptr<ledger::PublisherInfo>());
    return;
  }

  std::string json_info = publisher_info ? publisher_info->ToJson() : "";
  bat_ledger_client_->SavePublisherInfo(json_info,
      base::BindOnce(&OnSavePublisherInfo, std::move(callback)));
}

void OnLoadPublisherInfo(const ledger::PublisherInfoCallback& callback,
    int32_t result, const std::string& publisher_info) {
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!publisher_info.empty()) {
    info.reset(new ledger::PublisherInfo());
    info->loadFromJson(publisher_info);
  }
  callback(ToLedgerResult(result), std::move(info));
}

void BatLedgerClientMojoProxy::LoadPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR,
        std::unique_ptr<ledger::PublisherInfo>());
    return;
  }

  bat_ledger_client_->LoadPublisherInfo(publisher_key,
      base::BindOnce(&OnLoadPublisherInfo, std::move(callback)));
}

void OnLoadPanelPublisherInfo(const ledger::PublisherInfoCallback& callback,
    int32_t result, const std::string& publisher_info) {
  std::unique_ptr<ledger::PublisherInfo> info;

  if (!publisher_info.empty()) {
    info.reset(new ledger::PublisherInfo());
    info->loadFromJson(publisher_info);
  }

  callback(ToLedgerResult(result), std::move(info));
}

void BatLedgerClientMojoProxy::LoadPanelPublisherInfo(
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR,
        std::unique_ptr<ledger::PublisherInfo>());
    return;
  }

  bat_ledger_client_->LoadPanelPublisherInfo(filter.ToJson(),
      base::BindOnce(&OnLoadPanelPublisherInfo, std::move(callback)));
}

void OnLoadMediaPublisherInfo(const ledger::PublisherInfoCallback& callback,
    int32_t result, const std::string& publisher_info) {
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!publisher_info.empty()) {
    info.reset(new ledger::PublisherInfo());
    info->loadFromJson(publisher_info);
  }
  callback(ToLedgerResult(result), std::move(info));
}

void BatLedgerClientMojoProxy::LoadMediaPublisherInfo(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR,
        std::unique_ptr<ledger::PublisherInfo>());
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

void BatLedgerClientMojoProxy::OnPanelPublisherInfo(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info,
    uint64_t windowId) {
  if (!Connected()) {
    return;
  }

  std::string json_info = info ? info->ToJson() : "";
  bat_ledger_client_->OnPanelPublisherInfo(ToMojomResult(result),
      json_info, windowId);
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
                        const std::vector<std::string>& publisher_info_list,
                        uint32_t next_record) {
  ledger::PublisherInfoList list;

  for (const auto& publisher_info : publisher_info_list) {
    ledger::PublisherInfo info;
    info.loadFromJson(publisher_info);
    list.push_back(info);
  }

  callback(list, next_record);
}

void BatLedgerClientMojoProxy::GetRecurringTips(
    ledger::PublisherInfoListCallback callback) {
  if (!Connected()) {
    callback(std::vector<ledger::PublisherInfo>(), 0);
    return;
  }

  bat_ledger_client_->GetRecurringTips(
      base::BindOnce(&OnGetRecurringTips, std::move(callback)));
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

void BatLedgerClientMojoProxy::GetGrantCaptcha(
    const std::string& promotion_id,
    const std::string& promotion_type) {
  if (!Connected())
    return;

  bat_ledger_client_->GetGrantCaptcha(promotion_id, promotion_type);
}

std::string BatLedgerClientMojoProxy::URIEncode(const std::string& value) {
  if (!Connected())
    return "";

  std::string encoded_value;
  bat_ledger_client_->URIEncode(value, &encoded_value);
  return encoded_value;
}

void BatLedgerClientMojoProxy::SavePendingContribution(
      const ledger::PendingContributionList& list) {
  if (!Connected())
    return;

  bat_ledger_client_->SavePendingContribution(list.ToJson());
}

void OnLoadActivityInfo(const ledger::PublisherInfoCallback& callback,
    int32_t result, const std::string& publisher_info) {
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!publisher_info.empty()) {
    info.reset(new ledger::PublisherInfo());
    info->loadFromJson(publisher_info);
  }
  callback(ToLedgerResult(result), std::move(info));
}

void BatLedgerClientMojoProxy::LoadActivityInfo(
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR,
        std::unique_ptr<ledger::PublisherInfo>());
    return;
  }

  bat_ledger_client_->LoadActivityInfo(filter.ToJson(),
      base::BindOnce(&OnLoadActivityInfo, std::move(callback)));
}

void OnSaveActivityInfo(const ledger::PublisherInfoCallback& callback,
    int32_t result, const std::string& publisher_info) {
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!publisher_info.empty()) {
    info.reset(new ledger::PublisherInfo());
    info->loadFromJson(publisher_info);
  }
  callback(ToLedgerResult(result), std::move(info));
}

void BatLedgerClientMojoProxy::SaveActivityInfo(
    std::unique_ptr<ledger::PublisherInfo> publisher_info,
    ledger::PublisherInfoCallback callback) {
  if (!Connected()) {
    callback(ledger::Result::LEDGER_ERROR,
        std::unique_ptr<ledger::PublisherInfo>());
    return;
  }

  std::string json_info = publisher_info ? publisher_info->ToJson() : "";
  bat_ledger_client_->SaveActivityInfo(json_info,
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
    const std::vector<std::string>& publisher_info_list,
    uint32_t next_record) {
  ledger::PublisherInfoList list;

  for (const auto& publisher_info : publisher_info_list) {
    ledger::PublisherInfo info;
    info.loadFromJson(publisher_info);
    list.push_back(info);
  }

  callback(list, next_record);
}

void BatLedgerClientMojoProxy::GetActivityInfoList(uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilter filter,
    ledger::PublisherInfoListCallback callback) {
  if (!Connected()) {
    callback(std::vector<ledger::PublisherInfo>(), 0);
    return;
  }

  bat_ledger_client_->GetActivityInfoList(start,
      limit,
      filter.ToJson(),
      base::BindOnce(&OnGetActivityInfoList, std::move(callback)));
}


void BatLedgerClientMojoProxy::SaveNormalizedPublisherList(
    const ledger::PublisherInfoListStruct& normalized_list) {
  if (!Connected()) {
    return;
  }

  bat_ledger_client_->SaveNormalizedPublisherList(normalized_list.ToJson());
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

void BatLedgerClientMojoProxy::GetExcludedPublishersNumberDB(
    ledger::GetExcludedPublishersNumberDBCallback callback) {
  if (!Connected()) {
    callback(0);
    return;
  }

  bat_ledger_client_->GetExcludedPublishersNumberDB(
      base::BindOnce(&OnExcludedNumberDB, std::move(callback)));
}

}  // namespace bat_ledger
