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

template <typename Callback>
void LedgerClientMojoProxy::CallbackHolder<Callback>::OnLedgerStateLoaded(
    ledger::Result result,
    const std::string& data,
    ledger::InitializeCallback callback) {
  NOTREACHED();
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

void LedgerClientMojoProxy::GenerateGUID(GenerateGUIDCallback callback) {
  std::move(callback).Run(ledger_client_->GenerateGUID());
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
      std::bind(LedgerClientMojoProxy::OnLoadPublisherState, holder, _1, _2));
}

template <typename Callback>
void LedgerClientMojoProxy::CallbackHolder<Callback>::OnPublisherStateLoaded(
    ledger::Result result,
    const std::string& data,
    ledger::InitializeCallback callback) {
  NOTREACHED();
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
    std::move(callback_).Run(result);
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
    std::move(callback_).Run(result);
  delete this;
}

void LedgerClientMojoProxy::OnReconcileComplete(
    const ledger::Result result,
    const std::string& viewing_id,
    const double amount,
    const ledger::RewardsType type) {
  ledger_client_->OnReconcileComplete(
      result,
      viewing_id,
      amount,
      type);
}

// static
void LedgerClientMojoProxy::OnSavePublisherInfo(
    CallbackHolder<SavePublisherInfoCallback>* holder,
    const ledger::Result result,
    ledger::PublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));
  delete holder;
}

void LedgerClientMojoProxy::SavePublisherInfo(
    ledger::PublisherInfoPtr publisher_info,
    SavePublisherInfoCallback callback) {
  // deleted in OnSavePublisherInfo
  auto* holder = new CallbackHolder<SavePublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SavePublisherInfo(std::move(publisher_info),
      std::bind(LedgerClientMojoProxy::OnSavePublisherInfo, holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnLoadPublisherInfo(
    CallbackHolder<LoadPublisherInfoCallback>* holder,
    const ledger::Result result,
    ledger::PublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));
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
    const ledger::Result result,
    ledger::PublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));
  delete holder;
}

void LedgerClientMojoProxy::LoadPanelPublisherInfo(
    ledger::ActivityInfoFilterPtr filter,
    LoadPanelPublisherInfoCallback callback) {
  // deleted in OnLoadPanelPublisherInfo
  auto* holder = new CallbackHolder<LoadPanelPublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger::ActivityInfoFilter publisher_info_filter;
  ledger_client_->LoadPanelPublisherInfo(std::move(filter),
      std::bind(LedgerClientMojoProxy::OnLoadPanelPublisherInfo,
        holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnLoadMediaPublisherInfo(
    CallbackHolder<LoadMediaPublisherInfoCallback>* holder,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));
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
void LedgerClientMojoProxy::OnSaveRecurringTip(
    CallbackHolder<SaveRecurringTipCallback>* holder,
    const ledger::Result result) {
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::SaveRecurringTip(
    ledger::RecurringTipPtr info,
    SaveRecurringTipCallback callback) {
  // deleted in OnSaveRecurringTip
  auto* holder = new CallbackHolder<SaveRecurringTipCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SaveRecurringTip(
      std::move(info),
      std::bind(LedgerClientMojoProxy::OnSaveRecurringTip,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetRecurringTips(
    CallbackHolder<GetRecurringTipsCallback>* holder,
    ledger::PublisherInfoList publisher_info_list,
    uint32_t next_record) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(publisher_info_list), next_record);
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

// static
void LedgerClientMojoProxy::OnRemoveRecurringTip(
    CallbackHolder<RemoveRecurringTipCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::RemoveRecurringTip(const std::string& publisher_key,
    RemoveRecurringTipCallback callback) {
  // deleted in OnRemoveRecurringTip
  auto* holder = new CallbackHolder<RemoveRecurringTipCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->RemoveRecurringTip(publisher_key,
      std::bind(LedgerClientMojoProxy::OnRemoveRecurringTip, holder, _1));
}

// static
void LedgerClientMojoProxy::OnSaveContributionInfo(
    CallbackHolder<SaveContributionInfoCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::SaveContributionInfo(
    ledger::ContributionInfoPtr info,
    SaveContributionInfoCallback callback) {
    // deleted in OnSaveContributionInfo
  auto* holder = new CallbackHolder<RemoveRecurringTipCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SaveContributionInfo(
      std::move(info),
      std::bind(LedgerClientMojoProxy::OnSaveContributionInfo, holder, _1));
}

void LedgerClientMojoProxy::SaveMediaPublisherInfo(
    const std::string& media_key, const std::string& publisher_id) {
  ledger_client_->SaveMediaPublisherInfo(media_key, publisher_id);
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

// static
void LedgerClientMojoProxy::OnSavePendingContribution(
    CallbackHolder<SavePendingContributionCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::SavePendingContribution(
    ledger::PendingContributionList list,
    SavePendingContributionCallback callback) {
  // deleted in OnSavePendingContribution
  auto* holder = new CallbackHolder<SavePendingContributionCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->SavePendingContribution(std::move(list),
      std::bind(LedgerClientMojoProxy::OnSavePendingContribution, holder, _1));
}

// static
void LedgerClientMojoProxy::OnLoadActivityInfo(
    CallbackHolder<LoadActivityInfoCallback>* holder,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));
  delete holder;
}

void LedgerClientMojoProxy::LoadActivityInfo(
    ledger::ActivityInfoFilterPtr filter,
    LoadActivityInfoCallback callback) {
  // deleted in OnLoadActivityInfo
  auto* holder = new CallbackHolder<LoadActivityInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->LoadActivityInfo(std::move(filter),
      std::bind(LedgerClientMojoProxy::OnLoadActivityInfo, holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnSaveActivityInfo(
    CallbackHolder<SaveActivityInfoCallback>* holder,
    ledger::Result result,
    ledger::PublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));
  delete holder;
}

void LedgerClientMojoProxy::SaveActivityInfo(
    ledger::PublisherInfoPtr publisher_info,
    SaveActivityInfoCallback callback) {
  // deleted in OnSaveActivityInfo
  auto* holder = new CallbackHolder<SaveActivityInfoCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->SaveActivityInfo(std::move(publisher_info),
      std::bind(LedgerClientMojoProxy::OnSaveActivityInfo, holder, _1, _2));
}

// static
void LedgerClientMojoProxy::OnRestorePublishers(
    CallbackHolder<RestorePublishersCallback>* holder,
    ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::RestorePublishers(
    RestorePublishersCallback callback) {
  // deleted in OnRestorePublishers
  auto* holder = new CallbackHolder<RestorePublishersCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->RestorePublishers(
      std::bind(LedgerClientMojoProxy::OnRestorePublishers, holder, _1));
}

// static
void LedgerClientMojoProxy::OnGetActivityInfoList(
    CallbackHolder<GetActivityInfoListCallback>* holder,
    ledger::PublisherInfoList publisher_info_list,
    uint32_t next_record) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(publisher_info_list), next_record);
  delete holder;
}

void LedgerClientMojoProxy::GetActivityInfoList(uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) {
  // deleted in OnGetActivityInfoList
  auto* holder = new CallbackHolder<GetActivityInfoListCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_client_->GetActivityInfoList(start,
      limit,
      std::move(filter),
      std::bind(LedgerClientMojoProxy::OnGetActivityInfoList,
                holder,
                _1,
                _2));
}

void LedgerClientMojoProxy::SaveNormalizedPublisherList(
    ledger::PublisherInfoList list) {
  ledger_client_->SaveNormalizedPublisherList(std::move(list));
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

// static
void LedgerClientMojoProxy::OnGetOneTimeTips(
    CallbackHolder<GetOneTimeTipsCallback>* holder,
    ledger::PublisherInfoList publisher_info_list,
    uint32_t next_record) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(publisher_info_list), next_record);
  delete holder;
}

void LedgerClientMojoProxy::GetOneTimeTips(
    GetOneTimeTipsCallback callback) {
  // deleted in OnGetOneTimeTips
  auto* holder = new CallbackHolder<GetOneTimeTipsCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->GetOneTimeTips(
      std::bind(LedgerClientMojoProxy::OnGetOneTimeTips,
                holder,
                _1,
                _2));
}

// static
void LedgerClientMojoProxy::OnGetPendingContributions(
    CallbackHolder<GetPendingContributionsCallback>* holder,
    ledger::PendingContributionInfoList list) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list));
  delete holder;
}

void LedgerClientMojoProxy::GetPendingContributions(
    GetPendingContributionsCallback callback) {
  // deleted in OnGetPendingContributions
  auto* holder = new CallbackHolder<GetPendingContributionsCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->GetPendingContributions(
      std::bind(LedgerClientMojoProxy::OnGetPendingContributions,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnRemovePendingContribution(
    CallbackHolder<RemovePendingContributionCallback>* holder,
    ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::RemovePendingContribution(
    const uint64_t id,
    RemovePendingContributionCallback callback) {
  // deleted in OnRemovePendingContribution
  auto* holder = new CallbackHolder<RemovePendingContributionCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->RemovePendingContribution(
      id,
      std::bind(LedgerClientMojoProxy::OnRemovePendingContribution,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnRemoveAllPendingContributions(
    CallbackHolder<RemovePendingContributionCallback>* holder,
    ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void LedgerClientMojoProxy::RemoveAllPendingContributions(
    RemovePendingContributionCallback callback) {
  // deleted in OnRemoveAllPendingContributions
  auto* holder = new CallbackHolder<RemovePendingContributionCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->RemoveAllPendingContributions(
      std::bind(LedgerClientMojoProxy::OnRemoveAllPendingContributions,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetPendingContributionsTotal(
    CallbackHolder<GetPendingContributionsTotalCallback>* holder,
    double amount) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(amount);
  delete holder;
}

void LedgerClientMojoProxy::GetPendingContributionsTotal(
    GetPendingContributionsTotalCallback callback) {
  // deleted in OnGetPendingContributionsTotal
  auto* holder = new CallbackHolder<GetPendingContributionsTotalCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->GetPendingContributionsTotal(
      std::bind(LedgerClientMojoProxy::OnGetPendingContributionsTotal,
                holder,
                _1));
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

// static
void LedgerClientMojoProxy::OnDeleteActivityInfo(
    CallbackHolder<DeleteActivityInfoCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::DeleteActivityInfo(
      const std::string& publisher_key,
      DeleteActivityInfoCallback callback) {
  auto* holder = new CallbackHolder<DeleteActivityInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_client_->DeleteActivityInfo(
      publisher_key,
      std::bind(LedgerClientMojoProxy::OnDeleteActivityInfo,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnClearAndInsertServerPublisherList(
    CallbackHolder<ClearAndInsertServerPublisherListCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::ClearAndInsertServerPublisherList(
      ledger::ServerPublisherInfoList list,
      ClearAndInsertServerPublisherListCallback callback) {
  auto* holder = new CallbackHolder<ClearAndInsertServerPublisherListCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->ClearAndInsertServerPublisherList(
      std::move(list),
      std::bind(LedgerClientMojoProxy::OnClearAndInsertServerPublisherList,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetServerPublisherInfo(
    CallbackHolder<GetServerPublisherInfoCallback>* holder,
    ledger::ServerPublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(info));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetServerPublisherInfo(
    const std::string& publisher_key,
    GetServerPublisherInfoCallback callback) {
  auto* holder = new CallbackHolder<GetServerPublisherInfoCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetServerPublisherInfo(
      publisher_key,
      std::bind(LedgerClientMojoProxy::OnGetServerPublisherInfo,
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

// static
void LedgerClientMojoProxy::OnInsertOrUpdateContributionQueue(
    CallbackHolder<InsertOrUpdateContributionQueueCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::InsertOrUpdateContributionQueue(
    ledger::ContributionQueuePtr info,
    InsertOrUpdateContributionQueueCallback callback) {
  auto* holder = new CallbackHolder<InsertOrUpdateContributionQueueCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->InsertOrUpdateContributionQueue(
      std::move(info),
      std::bind(LedgerClientMojoProxy::OnInsertOrUpdateContributionQueue,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnDeleteContributionQueue(
    CallbackHolder<DeleteContributionQueueCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::DeleteContributionQueue(
    const uint64_t id,
    DeleteContributionQueueCallback callback) {
  auto* holder = new CallbackHolder<DeleteContributionQueueCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->DeleteContributionQueue(
      id,
      std::bind(LedgerClientMojoProxy::OnDeleteContributionQueue,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetFirstContributionQueue(
    CallbackHolder<GetFirstContributionQueueCallback>* holder,
    ledger::ContributionQueuePtr info) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(info));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetFirstContributionQueue(
    GetFirstContributionQueueCallback callback) {
  auto* holder = new CallbackHolder<GetFirstContributionQueueCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetFirstContributionQueue(
      std::bind(LedgerClientMojoProxy::OnGetFirstContributionQueue,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnInsertOrUpdatePromotion(
    CallbackHolder<InsertOrUpdatePromotionCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::InsertOrUpdatePromotion(
    ledger::PromotionPtr info,
    InsertOrUpdatePromotionCallback callback) {
  auto* holder = new CallbackHolder<InsertOrUpdatePromotionCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->InsertOrUpdatePromotion(
      std::move(info),
      std::bind(LedgerClientMojoProxy::OnInsertOrUpdatePromotion,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetPromotion(
    CallbackHolder<GetPromotionCallback>* holder,
    ledger::PromotionPtr info) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(info));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetPromotion(
    const std::string& id,
    GetPromotionCallback callback) {
  auto* holder = new CallbackHolder<GetPromotionCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetPromotion(
      id,
      std::bind(LedgerClientMojoProxy::OnGetPromotion,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetAllPromotions(
    CallbackHolder<GetAllPromotionsCallback>* holder,
    ledger::PromotionMap promotions) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(base::MapToFlatMap(std::move(promotions)));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetAllPromotions(
    GetAllPromotionsCallback callback) {
  auto* holder = new CallbackHolder<GetAllPromotionsCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetAllPromotions(
      std::bind(LedgerClientMojoProxy::OnGetAllPromotions,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnInsertOrUpdateUnblindedToken(
    CallbackHolder<InsertOrUpdateUnblindedTokenCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::InsertOrUpdateUnblindedToken(
    ledger::UnblindedTokenPtr info,
    InsertOrUpdateUnblindedTokenCallback callback) {
  auto* holder = new CallbackHolder<InsertOrUpdateUnblindedTokenCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->InsertOrUpdateUnblindedToken(
      std::move(info),
      std::bind(LedgerClientMojoProxy::OnInsertOrUpdateUnblindedToken,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetAllUnblindedTokens(
    CallbackHolder<GetAllUnblindedTokensCallback>* holder,
    ledger::UnblindedTokenList list) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(list));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetAllUnblindedTokens(
    GetAllUnblindedTokensCallback callback) {
  auto* holder = new CallbackHolder<GetAllUnblindedTokensCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetAllUnblindedTokens(
      std::bind(LedgerClientMojoProxy::OnGetAllUnblindedTokens,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnDeleteUnblindedToken(
    CallbackHolder<DeleteUnblindedTokensCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::DeleteUnblindedTokens(
    const std::vector<std::string>& id_list,
    DeleteUnblindedTokensCallback callback) {
  auto* holder = new CallbackHolder<DeleteUnblindedTokensCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->DeleteUnblindedTokens(
      id_list,
      std::bind(LedgerClientMojoProxy::OnDeleteUnblindedToken,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnDeleteUnblindedTokensForPromotion(
    CallbackHolder<DeleteUnblindedTokensForPromotionCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::DeleteUnblindedTokensForPromotion(
    const std::string& promotion_id,
    DeleteUnblindedTokensForPromotionCallback callback) {
  auto* holder = new CallbackHolder<DeleteUnblindedTokensForPromotionCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->DeleteUnblindedTokensForPromotion(
      promotion_id,
      std::bind(LedgerClientMojoProxy::OnDeleteUnblindedTokensForPromotion,
                holder,
                _1));
}

void LedgerClientMojoProxy::GetClientInfo(
    GetClientInfoCallback callback) {
  auto info = ledger_client_->GetClientInfo();
  std::move(callback).Run(std::move(info));
}

void LedgerClientMojoProxy::UnblindedTokensReady() {
  ledger_client_->UnblindedTokensReady();
}

// static
void LedgerClientMojoProxy::OnGetTransactionReport(
    CallbackHolder<GetTransactionReportCallback>* holder,
    ledger::TransactionReportInfoList list) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(list));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetTransactionReport(
    const ledger::ActivityMonth month,
    const int year,
    GetTransactionReportCallback callback) {
  auto* holder = new CallbackHolder<GetTransactionReportCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetTransactionReport(
      month,
      year,
      std::bind(LedgerClientMojoProxy::OnGetTransactionReport,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetContributionReport(
    CallbackHolder<GetContributionReportCallback>* holder,
    ledger::ContributionReportInfoList list) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(list));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetContributionReport(
    const ledger::ActivityMonth month,
    const int year,
    GetContributionReportCallback callback) {
  auto* holder = new CallbackHolder<GetContributionReportCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetContributionReport(
      month,
      year,
      std::bind(LedgerClientMojoProxy::OnGetContributionReport,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetNotCompletedContributions(
    CallbackHolder<GetIncompleteContributionsCallback>* holder,
    ledger::ContributionInfoList list) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(list));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetIncompleteContributions(
    GetIncompleteContributionsCallback callback) {
  auto* holder = new CallbackHolder<GetIncompleteContributionsCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetIncompleteContributions(
      std::bind(LedgerClientMojoProxy::OnGetNotCompletedContributions,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnGetContributionInfo(
    CallbackHolder<GetContributionInfoCallback>* holder,
    ledger::ContributionInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(info));
  }
  delete holder;
}

void LedgerClientMojoProxy::GetContributionInfo(
    const std::string& contribution_id,
    GetContributionInfoCallback callback) {
  auto* holder = new CallbackHolder<GetContributionInfoCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->GetContributionInfo(
      contribution_id,
      std::bind(LedgerClientMojoProxy::OnGetContributionInfo,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnUpdateContributionInfoStepAndCount(
    CallbackHolder<UpdateContributionInfoStepAndCountCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::UpdateContributionInfoStepAndCount(
    const std::string& contribution_id,
    const ledger::ContributionStep step,
    const int32_t retry_count,
    UpdateContributionInfoStepAndCountCallback callback) {
  auto* holder = new CallbackHolder<UpdateContributionInfoStepAndCountCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->UpdateContributionInfoStepAndCount(
      contribution_id,
      step,
      retry_count,
      std::bind(LedgerClientMojoProxy::OnUpdateContributionInfoStepAndCount,
                holder,
                _1));
}

// static
void LedgerClientMojoProxy::OnUpdateContributionInfoContributedAmount(
    CallbackHolder<UpdateContributionInfoContributedAmountCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void LedgerClientMojoProxy::UpdateContributionInfoContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    UpdateContributionInfoContributedAmountCallback callback) {
  auto* holder =
      new CallbackHolder<UpdateContributionInfoContributedAmountCallback>(
      AsWeakPtr(),
      std::move(callback));
  ledger_client_->UpdateContributionInfoContributedAmount(
      contribution_id,
      publisher_key,
      std::bind(
          LedgerClientMojoProxy::OnUpdateContributionInfoContributedAmount,
          holder,
          _1));
}

void LedgerClientMojoProxy::ReconcileStampReset() {
  ledger_client_->ReconcileStampReset();
}

}  // namespace bat_ledger
