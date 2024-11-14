/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/parameters/rewards_parameters_provider.h"

#include <utility>

#include "base/json/values_util.h"
#include "brave/components/brave_rewards/core/common/callback_helpers.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::internal {

namespace {

constexpr char kRateKey[] = "rate";
constexpr char kTipChoicesPath[] = "tip.choices";
constexpr char kTipMonthlyChoicesPath[] = "tip.monthly_choices";
constexpr char kPayoutStatusKey[] = "payout_status";
constexpr char kWalletProviderRegionsKey[] = "wallet_provider_regions";
constexpr char kVBatDeadlineKey[] = "vbat_deadline";
constexpr char kVBatExpiredKey[] = "vbat_expired";
constexpr char kTosVersion[] = "tos_version";

constexpr base::TimeDelta kRefreshInterval = base::Hours(3);
constexpr base::TimeDelta kErrorRetryInterval = base::Seconds(30);
constexpr base::TimeDelta kRandomDelay = base::Minutes(10);

template <typename T>
base::Value::List VectorToList(const std::vector<T>& values) {
  base::Value::List list;
  for (auto& value : values) {
    list.Append(value);
  }
  return list;
}

template <typename T>
base::Value::Dict MapToDict(const T& map) {
  base::Value::Dict dict;
  for (auto& [key, value] : map) {
    dict.Set(key, value);
  }
  return dict;
}

}  // namespace

RewardsParametersProvider::RewardsParametersProvider(RewardsEngine& engine)
    : RewardsEngineHelper(engine) {}

RewardsParametersProvider::~RewardsParametersProvider() = default;

void RewardsParametersProvider::StartAutoUpdate() {
  Fetch(base::DoNothing());
}

mojom::RewardsParametersPtr RewardsParametersProvider::GetCachedParameters() {
  auto value = engine().GetState<base::Value>(state::kParameters);
  if (!value.is_dict()) {
    return nullptr;
  }
  return DictToParameters(value.GetDict());
}

void RewardsParametersProvider::GetParameters(GetParametersCallback callback) {
  // Return cached parameters if available.
  if (auto params = GetCachedParameters()) {
    DeferCallback(FROM_HERE, std::move(callback), std::move(params));
    return;
  }

  Fetch(std::move(callback));
}

mojom::RewardsParametersPtr RewardsParametersProvider::DictToParameters(
    const base::Value::Dict& dict) {
  auto parameters = mojom::RewardsParameters::New();

  if (auto rate = dict.FindDouble(kRateKey)) {
    parameters->rate = *rate;
  } else {
    // If the "rate" key does not exist, then assume that we don't yet have
    // valid data in the cache. For other fields, perform a best-effort read and
    // fallback to default field data if not available.
    return nullptr;
  }

  if (auto* list = dict.FindListByDottedPath(kTipChoicesPath)) {
    for (auto& list_value : *list) {
      if (list_value.is_double()) {
        parameters->tip_choices.push_back(list_value.GetDouble());
      }
    }
  }

  if (auto* list = dict.FindListByDottedPath(kTipMonthlyChoicesPath)) {
    for (auto& list_value : *list) {
      if (list_value.is_double()) {
        parameters->monthly_tip_choices.push_back(list_value.GetDouble());
      }
    }
  }

  if (auto* payout_status = dict.FindDict(kPayoutStatusKey)) {
    for (auto [key, item_value] : *payout_status) {
      if (item_value.is_string()) {
        parameters->payout_status.emplace(key, item_value.GetString());
      }
    }
  }

  if (auto* regions_value = dict.Find(kWalletProviderRegionsKey)) {
    auto regions =
        endpoints::GetParameters::ValueToWalletProviderRegions(*regions_value);
    if (regions) {
      parameters->wallet_provider_regions = std::move(*regions);
    }
  }

  if (auto* deadline_value = dict.Find(kVBatDeadlineKey)) {
    if (auto time = base::ValueToTime(*deadline_value)) {
      parameters->vbat_deadline = *time;
    }
  }

  if (auto vbat_expired = dict.FindBool(kVBatExpiredKey)) {
    parameters->vbat_expired = *vbat_expired;
  }

  if (auto tos_version = dict.FindInt(kTosVersion)) {
    parameters->tos_version = *tos_version;
  }

  return parameters;
}

void RewardsParametersProvider::Fetch(GetParametersCallback callback) {
  bool first_request = callbacks_.empty();
  callbacks_.push_back(std::move(callback));
  if (!first_request) {
    Log(FROM_HERE) << "Rewards parameters fetch in progress";
    return;
  }

  refresh_timer_.Stop();

  endpoints::RequestFor<endpoints::GetParameters>(engine()).Send(
      base::BindOnce(&RewardsParametersProvider::OnEndpointResult,
                     weak_factory_.GetWeakPtr()));
}

void RewardsParametersProvider::OnEndpointResult(
    endpoints::GetParameters::Result&& result) {
  if (result.has_value()) {
    mojom::RewardsParametersPtr params = std::move(result.value());
    StoreParameters(*params);
    RunCallbacks(std::move(params));
    SetRefreshTimer(kRefreshInterval);
  } else {
    // On error, return cached parameters. If we haven't been able to retrieve
    // any parameters from the server yet, then just return a default data
    // structure.
    auto params = GetCachedParameters();
    if (!params) {
      params = mojom::RewardsParameters::New();
    }
    RunCallbacks(std::move(params));
    SetRefreshTimer(kErrorRetryInterval);
  }
}

void RewardsParametersProvider::RunCallbacks(
    mojom::RewardsParametersPtr parameters) {
  DCHECK(parameters);
  auto callbacks = std::move(callbacks_);
  for (auto& callback : callbacks) {
    std::move(callback).Run(parameters->Clone());
  }
}

void RewardsParametersProvider::SetRefreshTimer(base::TimeDelta delay) {
  if (refresh_timer_.IsRunning()) {
    Log(FROM_HERE) << "Parameters timer in progress";
    return;
  }

  base::TimeDelta start_in = delay + util::GetRandomizedDelay(kRandomDelay);

  Log(FROM_HERE) << "Parameters timer set for " << start_in;

  refresh_timer_.Start(
      FROM_HERE, start_in,
      base::BindOnce(&RewardsParametersProvider::Fetch,
                     weak_factory_.GetWeakPtr(), base::DoNothing()));
}

void RewardsParametersProvider::StoreParameters(
    const mojom::RewardsParameters& parameters) {
  base::Value::Dict dict;

  dict.Set(kRateKey, parameters.rate);
  dict.SetByDottedPath(kTipChoicesPath, VectorToList(parameters.tip_choices));
  dict.SetByDottedPath(kTipMonthlyChoicesPath,
                       VectorToList(parameters.monthly_tip_choices));
  dict.Set(kPayoutStatusKey, MapToDict(parameters.payout_status));

  base::Value::Dict wallet_provider_regions_dict;
  for (auto& [wallet_provider, regions] : parameters.wallet_provider_regions) {
    wallet_provider_regions_dict.Set(
        wallet_provider, base::Value::Dict()
                             .Set("allow", VectorToList(regions->allow))
                             .Set("block", VectorToList(regions->block)));
  }

  dict.Set(kWalletProviderRegionsKey, std::move(wallet_provider_regions_dict));
  dict.Set(kVBatDeadlineKey, base::TimeToValue(parameters.vbat_deadline));
  dict.Set(kVBatExpiredKey, parameters.vbat_expired);
  dict.Set(kTosVersion, parameters.tos_version);

  engine().SetState(state::kParameters, base::Value(std::move(dict)));
}

}  // namespace brave_rewards::internal
