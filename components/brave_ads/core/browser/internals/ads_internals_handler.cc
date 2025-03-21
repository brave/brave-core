// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_ads/core/browser/internals/ads_internals_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_service.h"

AdsInternalsHandler::AdsInternalsHandler(brave_ads::AdsService* ads_service,
                                         PrefService& prefs)
    : ads_service_(ads_service), prefs_(prefs) {
  pref_change_registrar_.Init(&*prefs_);
  pref_change_registrar_.Add(
      brave_rewards::prefs::kEnabled,
      base::BindRepeating(
          &AdsInternalsHandler::OnBraveRewardsEnabledPrefChanged,
          weak_ptr_factory_.GetWeakPtr()));
}

AdsInternalsHandler::~AdsInternalsHandler() = default;

void AdsInternalsHandler::BindInterface(
    mojo::PendingReceiver<bat_ads::mojom::AdsInternals> pending_receiver) {
  if (receiver_.is_bound()) {
    receiver_.reset();
  }

  receiver_.Bind(std::move(pending_receiver));
}

///////////////////////////////////////////////////////////////////////////////

void AdsInternalsHandler::CreateAdsInternalsPageHandler(
    mojo::PendingRemote<bat_ads::mojom::AdsInternalsPage> page_pending_remote) {
  page_remote_ = mojo::Remote<bat_ads::mojom::AdsInternalsPage>(
      std::move(page_pending_remote));

  UpdateBraveRewardsEnabled();
}

void AdsInternalsHandler::GetAdsInternals(GetAdsInternalsCallback callback) {
  if (!ads_service_) {
    return std::move(callback).Run(/*ads_internals=*/"{}");
  }

  ads_service_->GetInternals(
      base::BindOnce(&AdsInternalsHandler::GetInternalsCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsInternalsHandler::ClearAdsData(brave_ads::ClearDataCallback callback) {
  if (!ads_service_) {
    return std::move(callback).Run(/*success=*/false);
  }

  ads_service_->ClearData(std::move(callback));
}

void AdsInternalsHandler::GetInternalsCallback(
    GetAdsInternalsCallback callback,
    std::optional<base::Value::Dict> internals) {
  // `value` can be nullopt in the following cases:
  // - `bat_ads::mojom::BatAds` associated remote is not bound.
  // - A database query fails.
  std::string json;
  CHECK(base::JSONWriter::Write(
      std::move(internals).value_or(base::Value::Dict{}), &json));
  std::move(callback).Run(json);
}

void AdsInternalsHandler::OnBraveRewardsEnabledPrefChanged(
    const std::string& /*path*/) {
  UpdateBraveRewardsEnabled();
}

void AdsInternalsHandler::UpdateBraveRewardsEnabled() {
  if (!page_remote_) {
    return;
  }

  const bool is_enabled = prefs_->GetBoolean(brave_rewards::prefs::kEnabled);
  page_remote_->UpdateBraveRewardsEnabled(is_enabled);
}
