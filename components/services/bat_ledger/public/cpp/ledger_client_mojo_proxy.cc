/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/public/cpp/ledger_client_mojo_proxy.h"

#include "base/logging.h"
#include "mojo/public/cpp/bindings/map.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace bat_ledger {

namespace {  // TODO(anyone): move into a util class

int32_t ToMojomResult(ledger::Result result) {
  return (int32_t)result;
}

ledger::Result ToLedgerResult(int32_t result) {
  return (ledger::Result)result;
}

ledger::REWARDS_CATEGORY ToLedgerPublisherCategory(int32_t category) {
  return (ledger::REWARDS_CATEGORY)category;
}

ledger::Grant ToLedgerGrant(const std::string& grant_json) {
  ledger::Grant grant;
  grant.loadFromJson(grant_json);
  return grant;
}

ledger::URL_METHOD ToLedgerURLMethod(int32_t method) {
  return (ledger::URL_METHOD)method;
}

}  // namespace

LedgerClientMojoProxy::LedgerClientMojoProxy(
    ledger::LedgerClient* ledger_client)
  : ledger_client_(ledger_client) {
}

LedgerClientMojoProxy::~LedgerClientMojoProxy() {
}

template <typename Callback>
void LedgerClientMojoProxy::CallbackHolder<Callback>::OnLedgerStateLoaded(
    ledger::Result result, const std::string& data) {
  NOTREACHED();
}

template <>
void LedgerClientMojoProxy::CallbackHolder<
  LedgerClientMojoProxy::LoadLedgerStateCallback>::OnLedgerStateLoaded(
    ledger::Result result, const std::string& data) {
  if (is_valid())
    std::move(callback_).Run(ToMojomResult(result), data);
  delete this;
}

void LedgerClientMojoProxy::LoadLedgerState(LoadLedgerStateCallback callback) {
  auto* holder = new CallbackHolder<LoadLedgerStateCallback>(AsWeakPtr(),
      std::move(callback));
  ledger_client_->LoadLedgerState(holder);
}

void LedgerClientMojoProxy::GenerateGUID(GenerateGUIDCallback callback) {
  std::move(callback).Run(ledger_client_->GenerateGUID());
}

void LedgerClientMojoProxy::OnWalletInitialized(int32_t result) {
  ledger_client_->OnWalletInitialized(ToLedgerResult(result));
}

void LedgerClientMojoProxy::OnWalletProperties(int32_t result,
    const std::string& info) {
  std::unique_ptr<ledger::WalletInfo> wallet_info;
  if (!info.empty()) {
    wallet_info.reset(new ledger::WalletInfo());
    wallet_info->loadFromJson(info);
  }
  ledger_client_->OnWalletProperties(ToLedgerResult(result),
      std::move(wallet_info));
}

void LedgerClientMojoProxy::LoadPublisherState(
    LoadPublisherStateCallback callback) {
  auto* holder = new CallbackHolder<LoadPublisherStateCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->LoadPublisherState(holder);
}

template <typename Callback>
void LedgerClientMojoProxy::CallbackHolder<Callback>::OnPublisherStateLoaded(
    ledger::Result result, const std::string& data) {
  NOTREACHED();
}

template <>
void LedgerClientMojoProxy::CallbackHolder<
  LedgerClientMojoProxy::LoadPublisherStateCallback>::OnPublisherStateLoaded(
    ledger::Result result, const std::string& data) {
  if (is_valid())
    std::move(callback_).Run(ToMojomResult(result), data);
  delete this;
}

void LedgerClientMojoProxy::LoadPublisherList(
    LoadPublisherListCallback callback) {
  auto* holder = new CallbackHolder<LoadPublisherListCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->LoadPublisherList(holder);
}

template <typename Callback>
void LedgerClientMojoProxy::CallbackHolder<Callback>::OnPublisherListLoaded(
    ledger::Result result, const std::string& data) {
  NOTREACHED();
}

template <>
void LedgerClientMojoProxy::CallbackHolder<
  LedgerClientMojoProxy::LoadPublisherListCallback>::OnPublisherListLoaded(
    ledger::Result result, const std::string& data) {
  if (is_valid())
    std::move(callback_).Run(ToMojomResult(result), data);
  delete this;
}

void LedgerClientMojoProxy::SaveLedgerState(
    const std::string& ledger_state, SaveLedgerStateCallback callback) {
  auto* holder = new CallbackHolder<SaveLedgerStateCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SaveLedgerState(ledger_state, holder);
}

template <typename Callback>
void LedgerClientMojoProxy::CallbackHolder<Callback>::OnLedgerStateSaved(
    ledger::Result result) {
  NOTREACHED();
}

template <>
void LedgerClientMojoProxy::CallbackHolder<
  LedgerClientMojoProxy::SaveLedgerStateCallback>::OnLedgerStateSaved(
    ledger::Result result) {
  if (is_valid())
    std::move(callback_).Run(ToMojomResult(result));
  delete this;
}

void LedgerClientMojoProxy::SavePublisherState(
    const std::string& publisher_state, SavePublisherStateCallback callback) {
  auto* holder = new CallbackHolder<SavePublisherStateCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SavePublisherState(publisher_state, holder);
}

template <typename Callback>
void LedgerClientMojoProxy::CallbackHolder<Callback>::OnPublisherStateSaved(
    ledger::Result result) {
  NOTREACHED();
}

template <>
void LedgerClientMojoProxy::CallbackHolder<
  LedgerClientMojoProxy::SavePublisherStateCallback>::OnPublisherStateSaved(
    ledger::Result result) {
  if (is_valid())
    std::move(callback_).Run(ToMojomResult(result));
  delete this;
}

void LedgerClientMojoProxy::SavePublishersList(
    const std::string& publishers_list, SavePublishersListCallback callback) {
  auto* holder = new CallbackHolder<SavePublishersListCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SavePublishersList(publishers_list, holder);
}

template <typename Callback>
void LedgerClientMojoProxy::CallbackHolder<Callback>::OnPublishersListSaved(
    ledger::Result result) {
  NOTREACHED();
}

template <>
void LedgerClientMojoProxy::CallbackHolder<
  LedgerClientMojoProxy::SavePublishersListCallback>::OnPublishersListSaved(
    ledger::Result result) {
  if (is_valid())
    std::move(callback_).Run(ToMojomResult(result));
  delete this;
}

void LedgerClientMojoProxy::OnGrant(int32_t result, const std::string& grant) {
  ledger_client_->OnGrant(ToLedgerResult(result), ToLedgerGrant(grant));
}

void LedgerClientMojoProxy::OnGrantCaptcha(const std::string& image,
    const std::string& hint) {
  ledger_client_->OnGrantCaptcha(image, hint);
}

void LedgerClientMojoProxy::OnRecoverWallet(int32_t result, double balance,
    const std::vector<std::string>& grants) {
  std::vector<ledger::Grant> ledger_grants;
  for (auto const& grant : grants) {
    ledger_grants.push_back(ToLedgerGrant(grant));
  }

  ledger_client_->OnRecoverWallet(
      ToLedgerResult(result), balance, ledger_grants);
}

void LedgerClientMojoProxy::OnReconcileComplete(int32_t result,
    const std::string& viewing_id,
    int32_t category,
    const std::string& probi) {
  ledger_client_->OnReconcileComplete(ToLedgerResult(result), viewing_id,
      ToLedgerPublisherCategory(category), probi);
}

void LedgerClientMojoProxy::OnGrantFinish(int32_t result,
    const std::string& grant) {
  ledger_client_->OnGrantFinish(ToLedgerResult(result), ToLedgerGrant(grant));
}

// static
void LedgerClientMojoProxy::OnSavePublisherInfo(
    CallbackHolder<SavePublisherInfoCallback>* holder,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
  std::string json_info = info ? info->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(ToMojomResult(result), json_info);
  delete holder;
}

void LedgerClientMojoProxy::SavePublisherInfo(
    const std::string& publisher_info,
    SavePublisherInfoCallback callback) {
  // deleted in OnSavePublisherInfo
  auto* holder = new CallbackHolder<SavePublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!publisher_info.empty()) {
    info.reset(new ledger::PublisherInfo());
    info->loadFromJson(publisher_info);
  }

  ledger_client_->SavePublisherInfo(std::move(info),
      std::bind(LedgerClientMojoProxy::OnSavePublisherInfo, holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnLoadPublisherInfo(
    CallbackHolder<LoadPublisherInfoCallback>* holder,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
  std::string json_info = info ? info->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(ToMojomResult(result), json_info);
  delete holder;
}

void LedgerClientMojoProxy::LoadPublisherInfo(
    const std::string& publisher_key,
    LoadPublisherInfoCallback callback) {
  // deleted in OnLoadPublisherInfo
  auto* holder = new CallbackHolder<LoadPublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->LoadPublisherInfo(publisher_key,
      std::bind(LedgerClientMojoProxy::OnLoadPublisherInfo, holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnLoadPanelPublisherInfo(
    CallbackHolder<LoadPanelPublisherInfoCallback>* holder,
    ledger::Result result, std::unique_ptr<ledger::PublisherInfo> info) {
  std::string json_info = info ? info->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(ToMojomResult(result), json_info);
  delete holder;
}

void LedgerClientMojoProxy::LoadPanelPublisherInfo(const std::string& filter,
    LoadPanelPublisherInfoCallback callback) {
  // deleted in OnLoadPanelPublisherInfo
  auto* holder = new CallbackHolder<LoadPanelPublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger::ActivityInfoFilter publisher_info_filter;
  publisher_info_filter.loadFromJson(filter);
  ledger_client_->LoadPanelPublisherInfo(publisher_info_filter,
      std::bind(LedgerClientMojoProxy::OnLoadPanelPublisherInfo,
        holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnLoadMediaPublisherInfo(
    CallbackHolder<LoadMediaPublisherInfoCallback>* holder,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
  std::string json_info = info ? info->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(ToMojomResult(result), json_info);
  delete holder;
}

void LedgerClientMojoProxy::LoadMediaPublisherInfo(
    const std::string& media_key,
    LoadMediaPublisherInfoCallback callback) {
  // deleted in OnLoadMediaPublisherInfo
  auto* holder = new CallbackHolder<LoadMediaPublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->LoadMediaPublisherInfo(media_key,
      std::bind(LedgerClientMojoProxy::OnLoadMediaPublisherInfo,
                holder, _1, _2));
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

void LedgerClientMojoProxy::OnExcludedSitesChanged(
    const std::string& publisher_id,
    int exclude) {
  ledger_client_->OnExcludedSitesChanged(
      publisher_id,
      static_cast<ledger::PUBLISHER_EXCLUDE>(exclude));
}

void LedgerClientMojoProxy::OnPanelPublisherInfo(int32_t result,
    const std::string& info, uint64_t window_id) {
  std::unique_ptr<ledger::PublisherInfo> publisher_info;
  if (!info.empty()) {
    publisher_info.reset(new ledger::PublisherInfo());
    publisher_info->loadFromJson(info);
  }
  ledger_client_->OnPanelPublisherInfo(ToLedgerResult(result),
      std::move(publisher_info), window_id);
}

// static
void LedgerClientMojoProxy::OnFetchFavIcon(
    CallbackHolder<FetchFavIconCallback>* holder,
    bool success, const std::string& favicon_url) {
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
void LedgerClientMojoProxy::OnGetRecurringTips(
    CallbackHolder<GetRecurringTipsCallback>* holder,
    const ledger::PublisherInfoList& publisher_info_list,
    uint32_t next_record) {
  std::vector<std::string> list;
  for (const auto& publisher_info : publisher_info_list) {
    list.push_back(publisher_info.ToJson());
  }

  if (holder->is_valid())
    std::move(holder->get()).Run(list, next_record);
  delete holder;
}

void LedgerClientMojoProxy::GetRecurringTips(
    GetRecurringTipsCallback callback) {
  // deleted in OnGetRecurringTips
  auto* holder = new CallbackHolder<GetRecurringTipsCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->GetRecurringTips(
      std::bind(LedgerClientMojoProxy::OnGetRecurringTips,
                holder,
                _1,
                _2));
}

// static
void LedgerClientMojoProxy::OnLoadNicewareList(
    CallbackHolder<LoadNicewareListCallback>* holder,
    int32_t result, const std::string& data) {
  if (holder->is_valid())
    std::move(holder->get()).Run(ToLedgerResult(result), data);
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

// static
void LedgerClientMojoProxy::OnRecurringRemoved(
    CallbackHolder<OnRemoveRecurringCallback>* holder, int32_t result) {
  if (holder->is_valid())
    std::move(holder->get()).Run(ToLedgerResult(result));
  delete holder;
}

void LedgerClientMojoProxy::OnRemoveRecurring(const std::string& publisher_key,
    OnRemoveRecurringCallback callback) {
  // deleted in OnRecurringRemoved
  auto* holder = new CallbackHolder<OnRemoveRecurringCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->OnRemoveRecurring(publisher_key,
      std::bind(LedgerClientMojoProxy::OnRecurringRemoved, holder, _1));
}

void LedgerClientMojoProxy::SaveContributionInfo(const std::string& probi,
    int32_t month, int32_t year, uint32_t date,
    const std::string& publisher_key, int32_t category) {
  ledger_client_->SaveContributionInfo(probi, month, year, date, publisher_key,
      ToLedgerPublisherCategory(category));
}

void LedgerClientMojoProxy::SaveMediaPublisherInfo(
    const std::string& media_key, const std::string& publisher_id) {
  ledger_client_->SaveMediaPublisherInfo(media_key, publisher_id);
}

void LedgerClientMojoProxy::FetchGrants(const std::string& lang,
    const std::string& payment_id) {
  ledger_client_->FetchGrants(lang, payment_id);
}

void LedgerClientMojoProxy::GetGrantCaptcha(
    const std::string& promotion_id,
    const std::string& promotion_type) {
  ledger_client_->GetGrantCaptcha(promotion_id, promotion_type);
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
  if (holder->is_valid())
    std::move(holder->get())
        .Run(response_code, response, mojo::MapToFlatMap(headers));
  delete holder;
}

void LedgerClientMojoProxy::LoadURL(const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    int32_t method,
    LoadURLCallback callback) {
  // deleted in OnLoadURL
  auto* holder = new CallbackHolder<LoadURLCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->LoadURL(url, headers, content, contentType,
      ToLedgerURLMethod(method),
      std::bind(LedgerClientMojoProxy::OnLoadURL, holder, _1, _2, _3));
}

void LedgerClientMojoProxy::SavePendingContribution(const std::string& list) {
  ledger::PendingContributionList contribution_list;
  contribution_list.loadFromJson(list);

  ledger_client_->SavePendingContribution(contribution_list);
}

// static
void LedgerClientMojoProxy::OnLoadActivityInfo(
    CallbackHolder<LoadActivityInfoCallback>* holder,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
  std::string json_info = info ? info->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(ToMojomResult(result), json_info);
  delete holder;
}

void LedgerClientMojoProxy::LoadActivityInfo(
    const std::string& filter,
    LoadActivityInfoCallback callback) {
  // deleted in OnLoadActivityInfo
  auto* holder = new CallbackHolder<LoadActivityInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger::ActivityInfoFilter publisher_info_filter;
  publisher_info_filter.loadFromJson(filter);
  ledger_client_->LoadActivityInfo(publisher_info_filter,
      std::bind(LedgerClientMojoProxy::OnLoadActivityInfo, holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnSaveActivityInfo(
    CallbackHolder<SaveActivityInfoCallback>* holder,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> info) {
  std::string json_info = info ? info->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(ToMojomResult(result), json_info);
  delete holder;
}

void LedgerClientMojoProxy::SaveActivityInfo(
    const std::string& publisher_info,
    SaveActivityInfoCallback callback) {
  // deleted in OnSaveActivityInfo
  auto* holder = new CallbackHolder<SaveActivityInfoCallback>(
      AsWeakPtr(), std::move(callback));
  std::unique_ptr<ledger::PublisherInfo> info;
  if (!publisher_info.empty()) {
    info.reset(new ledger::PublisherInfo());
    info->loadFromJson(publisher_info);
  }

  ledger_client_->SaveActivityInfo(std::move(info),
      std::bind(LedgerClientMojoProxy::OnSaveActivityInfo, holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnRestorePublishersDone(
    CallbackHolder<OnRestorePublishersCallback>* holder,
    bool result) {
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::OnRestorePublishers(
    OnRestorePublishersCallback callback) {
  // deleted in OnRestorePublishersDone
  auto* holder = new CallbackHolder<OnRestorePublishersCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->OnRestorePublishers(
      std::bind(LedgerClientMojoProxy::OnRestorePublishersDone, holder, _1));
}

// static
void LedgerClientMojoProxy::OnGetActivityInfoList(
    CallbackHolder<GetActivityInfoListCallback>* holder,
    const ledger::PublisherInfoList& publisher_info_list,
    uint32_t next_record) {
  std::vector<std::string> list;
  for (const auto& publisher_info : publisher_info_list) {
    list.push_back(publisher_info.ToJson());
  }

  if (holder->is_valid())
    std::move(holder->get()).Run(list, next_record);
  delete holder;
}

void LedgerClientMojoProxy::GetActivityInfoList(uint32_t start,
    uint32_t limit,
    const std::string& filter,
    GetActivityInfoListCallback callback) {
  // deleted in OnGetActivityInfoList
  auto* holder = new CallbackHolder<GetActivityInfoListCallback>(
      AsWeakPtr(), std::move(callback));

  ledger::ActivityInfoFilter publisher_info_filter;
  publisher_info_filter.loadFromJson(filter);

  ledger_client_->GetActivityInfoList(start,
      limit,
      publisher_info_filter,
      std::bind(LedgerClientMojoProxy::OnGetActivityInfoList,
                holder,
                _1,
                _2));
}

void LedgerClientMojoProxy::SaveNormalizedPublisherList(
    const std::string& normalized_list) {

  ledger::PublisherInfoListStruct list;
  list.loadFromJson(normalized_list);

  ledger_client_->SaveNormalizedPublisherList(list);
}

// static
void LedgerClientMojoProxy::OnSaveState(
    CallbackHolder<SaveStateCallback>* holder,
    const ledger::Result result) {
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

void LedgerClientMojoProxy::SetConfirmationsIsReady(const bool is_ready) {
  ledger_client_->SetConfirmationsIsReady(is_ready);
}

void LedgerClientMojoProxy::ConfirmationsTransactionHistoryDidChange() {
  ledger_client_->ConfirmationsTransactionHistoryDidChange();
}

// static
void LedgerClientMojoProxy::OnGetExcludedPublishersNumberDB(
    CallbackHolder<GetExcludedPublishersNumberDBCallback>* holder,
    uint32_t number) {
  if (holder->is_valid())
    std::move(holder->get()).Run(number);
  delete holder;
}

void LedgerClientMojoProxy::GetExcludedPublishersNumberDB(
    GetExcludedPublishersNumberDBCallback callback) {
  // deleted in OnGetRecurringTips
  auto* holder = new CallbackHolder<GetExcludedPublishersNumberDBCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->GetExcludedPublishersNumberDB(
      std::bind(LedgerClientMojoProxy::OnGetExcludedPublishersNumberDB,
        holder, _1));
}

}  // namespace bat_ledger
