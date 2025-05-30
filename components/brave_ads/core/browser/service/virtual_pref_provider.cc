/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider.h"

#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/version_info/version_info.h"
#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/skus/browser/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

namespace {

constexpr char kSkuEnvironmentPrefix[] = "skus:";
constexpr char kSkuOrdersKey[] = "orders";
constexpr char kSkuOrderLocationKey[] = "location";
constexpr char kSkuOrderCreatedAtKey[] = "created_at";
constexpr char kSkuOrderExpiresAtKey[] = "expires_at";
constexpr char kSkuOrderLastPaidAtKey[] = "last_paid_at";
constexpr char kSkuOrderStatusKey[] = "status";

std::string StripSkuEnvironmentPrefix(const std::string& environment) {
  const size_t pos = environment.find(':');
  return environment.substr(pos + 1);
}

std::string NormalizeSkuStatus(const std::string& status) {
  // Normalize the status field to use consistent (US) spelling, as the JSON
  // source localizes it (e.g., "cancelled" vs "canceled").
  return status == "cancelled" ? "canceled" : status;
}

base::Value::Dict ParseSkuOrder(const base::Value::Dict& dict) {
  base::Value::Dict order;

  if (const std::string* const created_at =
          dict.FindString(kSkuOrderCreatedAtKey)) {
    order.Set(kSkuOrderCreatedAtKey, *created_at);
  }

  if (const std::string* const expires_at =
          dict.FindString(kSkuOrderExpiresAtKey)) {
    order.Set(kSkuOrderExpiresAtKey, *expires_at);
  }

  if (const std::string* const last_paid_at =
          dict.FindString(kSkuOrderLastPaidAtKey)) {
    order.Set(kSkuOrderLastPaidAtKey, *last_paid_at);
  }

  if (const std::string* const status = dict.FindString(kSkuOrderStatusKey)) {
    const std::string normalized_status = NormalizeSkuStatus(*status);
    order.Set(kSkuOrderStatusKey, normalized_status);
  }

  return order;
}

base::Value::Dict ParseSkuOrders(const base::Value::Dict& dict) {
  base::Value::Dict orders;

  for (const auto [/*id*/ _, value] : dict) {
    const base::Value::Dict* const order = value.GetIfDict();
    if (!order) {
      continue;
    }

    const std::string* const location = order->FindString(kSkuOrderLocationKey);
    if (!location || location->empty()) {
      continue;
    }

    orders.Set(*location, ParseSkuOrder(*order));
  }

  return orders;
}

base::Value::Dict GetSkus(PrefService* local_state) {
  base::Value::Dict skus;

  if (!local_state->FindPreference(skus::prefs::kSkusState)) {
    // No SKUs in local state.
    return skus;
  }

  const base::Value::Dict& skus_state =
      local_state->GetDict(skus::prefs::kSkusState);
  for (const auto [environment, value] : skus_state) {
    if (!environment.starts_with(kSkuEnvironmentPrefix)) {
      continue;
    }

    // Deserialize the SKUs data from a JSON string stored in local state into a
    // dictionary object for further processing.
    std::optional<base::Value::Dict> sku_state =
        base::JSONReader::ReadDict(value.GetString());
    if (!sku_state) {
      continue;
    }

    const base::Value::Dict* const orders = sku_state->FindDict(kSkuOrdersKey);
    if (!orders) {
      continue;
    }

    skus.Set(StripSkuEnvironmentPrefix(environment), ParseSkuOrders(*orders));
  }

  return skus;
}

}  // namespace

VirtualPrefProvider::VirtualPrefProvider(PrefService* local_state,
                                         std::unique_ptr<Delegate> delegate)
    : local_state_(local_state), delegate_(std::move(delegate)) {
  CHECK(local_state_);
  CHECK(delegate_);
}

VirtualPrefProvider::~VirtualPrefProvider() = default;

base::Value::Dict VirtualPrefProvider::GetPrefs() {
  return base::Value::Dict()
      .Set("[virtual]:browser",
           base::Value::Dict()
               .Set("build_channel", delegate_->GetChannel())
               .Set("version", version_info::GetVersionNumber())
               .Set("major_version", GetMajorVersion())
               .Set("minor_version", GetMinorVersion())
               .Set("build_version", GetBuildVersion())
               .Set("patch_version", GetPatchVersion()))
      .Set("[virtual]:operating_system",
           base::Value::Dict()
               .Set("locale",
                    base::Value::Dict()
                        .Set("language",
                             brave_l10n::GetDefaultISOLanguageCodeString())
                        .Set("region",
                             brave_l10n::GetDefaultISOCountryCodeString()))
               .Set("is_mobile_platform", IsMobilePlatform())
               .Set("name", version_info::GetOSType()))
      .Set("[virtual]:search_engine",
           base::Value::Dict().Set("default_name",
                                   delegate_->GetDefaultSearchEngineName()))
      .Set("[virtual]:skus", GetSkus(local_state_));
}

}  // namespace brave_ads
