// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_ads/core/browser/internals/ads_internals_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_service.h"

namespace {
constexpr char kDiagnosticIdKey[] = "diagnosticId";
constexpr char kEntriesKey[] = "entries";
}  // namespace

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
    mojo::PendingReceiver<bat_ads::mojom::AdsInternals>
        ads_internals_pending_receiver) {
  if (ads_internals_receiver_.is_bound()) {
    ads_internals_receiver_.reset();
  }

  ads_internals_receiver_.Bind(std::move(ads_internals_pending_receiver));
}

///////////////////////////////////////////////////////////////////////////////

void AdsInternalsHandler::CreateAdsInternalsPageHandler(
    mojo::PendingRemote<bat_ads::mojom::AdsInternalsPage>
        ads_internals_page_pending_remote) {
  ads_internals_page_remote_ = mojo::Remote<bat_ads::mojom::AdsInternalsPage>(
      std::move(ads_internals_page_pending_remote));

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

void AdsInternalsHandler::ClearAdsData(brave_ads::ResultCallback callback) {
  if (!ads_service_) {
    return std::move(callback).Run(/*success=*/false);
  }

  ads_service_->ClearData(std::move(callback));
}

void AdsInternalsHandler::GetDiagnostics(GetDiagnosticsCallback callback) {
  if (!ads_service_) {
    return OnGetDiagnostics(std::move(callback),
                            /*diagnostic_entries=*/std::nullopt);
  }

  ads_service_->GetDiagnostics(
      base::BindOnce(&AdsInternalsHandler::OnGetDiagnostics,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsInternalsHandler::SetDiagnosticId(const std::string& diagnostic_id) {
  if (!base::Uuid::ParseCaseInsensitive(diagnostic_id).is_valid()) {
    return;
  }

  prefs_->SetString(brave_ads::prefs::kDiagnosticId, diagnostic_id);
}

void AdsInternalsHandler::GetInternalsCallback(
    GetAdsInternalsCallback callback,
    std::optional<base::DictValue> dict) {
  // `dict` can be nullopt in the following cases:
  // - `bat_ads::mojom::BatAds` associated remote is not bound.
  // - A database query fails.
  std::string json;
  CHECK(base::JSONWriter::Write(std::move(dict).value_or(base::DictValue{}),
                                &json));
  std::move(callback).Run(json);
}

void AdsInternalsHandler::OnGetDiagnostics(
    GetDiagnosticsCallback callback,
    std::optional<base::ListValue> diagnostic_entries) {
  base::DictValue dict;
  dict.Set(kDiagnosticIdKey,
           prefs_->GetString(brave_ads::prefs::kDiagnosticId));
  if (diagnostic_entries) {
    dict.Set(kEntriesKey, std::move(*diagnostic_entries));
  }

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  std::move(callback).Run(json);
}

void AdsInternalsHandler::OnBraveRewardsEnabledPrefChanged(
    const std::string& /*path*/) {
  UpdateBraveRewardsEnabled();
}

void AdsInternalsHandler::UpdateBraveRewardsEnabled() {
  if (!ads_internals_page_remote_) {
    return;
  }

  const bool is_enabled = prefs_->GetBoolean(brave_rewards::prefs::kEnabled);
  ads_internals_page_remote_->UpdateBraveRewardsEnabled(is_enabled);
}
