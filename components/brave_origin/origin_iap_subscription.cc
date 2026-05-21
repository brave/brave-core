/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/origin_iap_subscription.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_origin/pref_names.h"
#include "components/prefs/pref_service.h"

namespace {

inline constexpr char kDefaultPackage[] = "com.brave.browser";
inline constexpr char kProductId[] = "brave.origin.perpetual";

}  // namespace

namespace brave_origin {

OriginIAPSubscription::OriginIAPSubscription(PrefService* prefs)
    : prefs_(prefs) {}

OriginIAPSubscription::~OriginIAPSubscription() = default;

void OriginIAPSubscription::GetPurchaseTokenOrderId(
    GetPurchaseTokenOrderIdCallback callback) {
  std::string order_id_string;
  std::string purchase_token_string;
  std::string package_string = kDefaultPackage;
  std::string product_id_string = kProductId;

  auto* purchase_token =
      prefs_->FindPreference(prefs::kBraveOriginPurchaseTokenAndroid);
  if (purchase_token && !purchase_token->IsDefaultValue()) {
    purchase_token_string =
        prefs_->GetString(prefs::kBraveOriginPurchaseTokenAndroid);
  }

  auto* package = prefs_->FindPreference(prefs::kBraveOriginPackageNameAndroid);
  if (package && !package->IsDefaultValue()) {
    package_string = prefs_->GetString(prefs::kBraveOriginPackageNameAndroid);
  }

  auto* product_id =
      prefs_->FindPreference(prefs::kBraveOriginProductIdAndroid);
  if (product_id && !product_id->IsDefaultValue()) {
    product_id_string = prefs_->GetString(prefs::kBraveOriginProductIdAndroid);
  }

  auto* order_id = prefs_->FindPreference(prefs::kBraveOriginOrderIdAndroid);
  if (order_id && !order_id->IsDefaultValue()) {
    order_id_string = prefs_->GetString(prefs::kBraveOriginOrderIdAndroid);
  }

  base::DictValue response;
  response.Set("type", "android");
  response.Set("raw_receipt", purchase_token_string);
  response.Set("package", package_string);
  response.Set("subscription_id", product_id_string);

  std::string response_json;
  base::JSONWriter::Write(response, &response_json);
  std::move(callback).Run(base::Base64Encode(response_json), order_id_string);
}

void OriginIAPSubscription::SetLinkStatus(int32_t status) {
  prefs_->SetInteger(prefs::kBraveOriginSubscriptionLinkStatusAndroid, status);
}

}  // namespace brave_origin
