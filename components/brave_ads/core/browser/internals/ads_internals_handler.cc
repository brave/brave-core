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
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_service.h"

AdsInternalsHandler::AdsInternalsHandler(brave_ads::AdsService* ads_service,
                                         PrefService* prefs)
    : ads_service_(ads_service), prefs_(prefs) {
  CHECK(ads_service_);
  CHECK(prefs_);

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      brave_rewards::prefs::kEnabled,
      base::BindRepeating(&AdsInternalsHandler::OnPrefChanged,
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

void AdsInternalsHandler::GetAdsInternals(GetAdsInternalsCallback callback) {
  if (!ads_service_) {
    return std::move(callback).Run(/*response=*/"");
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

void AdsInternalsHandler::CreateAdsInternalsPageHandler(
    mojo::PendingRemote<bat_ads::mojom::AdsInternalsPage> page) {
  page_ = mojo::Remote<bat_ads::mojom::AdsInternalsPage>(std::move(page));

  UpdateBraveRewardsEnabled();
}

void AdsInternalsHandler::GetInternalsCallback(
    GetAdsInternalsCallback callback,
    std::optional<base::Value::List> value) {
  if (!value) {
    return std::move(callback).Run("");
  }

  std::string json;
  CHECK(base::JSONWriter::Write(*value, &json));
  std::move(callback).Run(json);
}

void AdsInternalsHandler::OnPrefChanged(const std::string& path) {
  if (path == brave_rewards::prefs::kEnabled) {
    UpdateBraveRewardsEnabled();
  }
}

void AdsInternalsHandler::UpdateBraveRewardsEnabled() {
  if (!page_) {
    return;
  }

  const bool rewards_enabled =
      prefs_->GetBoolean(brave_rewards::prefs::kEnabled);
  page_->OnBraveRewardsEnabledChanged(rewards_enabled);
}
