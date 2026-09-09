// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_ads/core/browser/internals/ads_internals_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_service.h"
#include "components/variations/service/variations_service.h"

namespace {
constexpr char kDiagnosticIdKey[] = "diagnosticId";
constexpr char kVariationsCountryCodeKey[] = "variationsCountryCode";
constexpr char kNtpSponsoredImagesComponentIdKey[] =
    "ntpSponsoredImagesComponentId";
constexpr char kCountryResourceComponentIdKey[] = "countryResourceComponentId";
constexpr char kLanguageResourceComponentIdKey[] =
    "languageResourceComponentId";
constexpr char kNtpSponsoredImagesLoadedKey[] = "ntpSponsoredImagesLoaded";
constexpr char kNtpSponsoredImagesManifestVersionKey[] =
    "ntpSponsoredImagesManifestVersion";
constexpr char kIsInitializedKey[] = "isInitialized";
}  // namespace

AdsInternalsHandler::AdsInternalsHandler(
    brave_ads::AdsService* ads_service,
    PrefService& prefs,
    variations::VariationsService* variations_service,
    GetComponentIdCallback get_ntp_sponsored_images_component_id_callback,
    GetComponentIdCallback get_country_resource_component_id_callback,
    GetComponentIdCallback get_language_resource_component_id_callback,
    GetIsSponsoredImagesLoadedCallback get_is_sponsored_images_loaded_callback,
    GetComponentIdCallback get_ntp_sponsored_images_manifest_version_callback)
    : ads_service_(ads_service),
      prefs_(prefs),
      variations_service_(variations_service),
      get_ntp_sponsored_images_component_id_callback_(
          std::move(get_ntp_sponsored_images_component_id_callback)),
      get_country_resource_component_id_callback_(
          std::move(get_country_resource_component_id_callback)),
      get_language_resource_component_id_callback_(
          std::move(get_language_resource_component_id_callback)),
      get_is_sponsored_images_loaded_callback_(
          std::move(get_is_sponsored_images_loaded_callback)),
      get_ntp_sponsored_images_manifest_version_callback_(
          std::move(get_ntp_sponsored_images_manifest_version_callback)) {
  pref_change_registrar_.Init(&*prefs_);
  pref_change_registrar_.Add(
      brave_rewards::prefs::kEnabled,
      base::BindRepeating(
          &AdsInternalsHandler::OnBraveRewardsEnabledPrefChanged,
          weak_ptr_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      brave_rewards::prefs::kExternalWalletType,
      base::BindRepeating(
          &AdsInternalsHandler::OnBraveRewardsWalletConnectedPrefChanged,
          weak_ptr_factory_.GetWeakPtr()));

  if (ads_service_) {
    ads_service_->AddObserver(this);
  }
}

AdsInternalsHandler::~AdsInternalsHandler() {
  if (ads_service_) {
    ads_service_->RemoveObserver(this);
  }
}

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
  UpdateBraveRewardsWalletConnected();
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
  base::DictValue dict = BuildDiagnosticsDict();

  if (!ads_service_) {
    return WriteDiagnosticsJson(std::move(callback), std::move(dict));
  }

  ads_service_->GetDiagnostics(base::BindOnce(
      &AdsInternalsHandler::OnGetDiagnostics, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(dict)));
}

void AdsInternalsHandler::EvaluateConditionMatcher(
    const std::string& pref_path,
    const std::string& condition,
    const std::optional<std::string>& test_value,
    EvaluateConditionMatcherCallback callback) {
  if (!ads_service_) {
    return std::move(callback).Run(/*current_value=*/"Unknown",
                                   /*matches=*/"N/A");
  }

  ads_service_->EvaluateConditionMatcher(
      pref_path, condition, test_value,
      base::BindOnce(
          [](EvaluateConditionMatcherCallback callback,
             std::string current_value, std::string matches) {
            std::move(callback).Run(current_value, matches);
          },
          std::move(callback)));
}

void AdsInternalsHandler::SetDiagnosticId(const std::string& diagnostic_id) {
  // Empty clears the diagnostic ID; anything else must be a valid UUID.
  if (!diagnostic_id.empty() &&
      !base::Uuid::ParseCaseInsensitive(diagnostic_id).is_valid()) {
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

base::DictValue AdsInternalsHandler::BuildDiagnosticsDict() const {
  base::DictValue dict;
  dict.Set(kDiagnosticIdKey,
           prefs_->GetString(brave_ads::prefs::kDiagnosticId));
  dict.Set(kIsInitializedKey, ads_service_ && ads_service_->IsInitialized());
  std::string variations_country_code =
      variations_service_
          ? base::ToUpperASCII(variations_service_->GetLatestCountry())
          : std::string();
  if (variations_country_code.empty()) {
    // No variations country yet. Fall back to the device's locale-derived
    // country so the field isn't blank, but flag it as a fallback so it's
    // not mistaken for a real variations country reading.
    variations_country_code = base::StrCat(
        {base::ToUpperASCII(brave_ads::CurrentCountryCode()), " (Fallback)"});
  }
  dict.Set(kVariationsCountryCodeKey, variations_country_code);
  if (get_ntp_sponsored_images_component_id_callback_) {
    if (const std::optional<std::string> component_id =
            get_ntp_sponsored_images_component_id_callback_.Run()) {
      dict.Set(kNtpSponsoredImagesComponentIdKey, *component_id);
    }
  }
  if (get_country_resource_component_id_callback_) {
    if (const std::optional<std::string> component_id =
            get_country_resource_component_id_callback_.Run()) {
      dict.Set(kCountryResourceComponentIdKey, *component_id);
    }
  }
  if (get_language_resource_component_id_callback_) {
    if (const std::optional<std::string> component_id =
            get_language_resource_component_id_callback_.Run()) {
      dict.Set(kLanguageResourceComponentIdKey, *component_id);
    }
  }
  if (get_is_sponsored_images_loaded_callback_) {
    dict.Set(kNtpSponsoredImagesLoadedKey,
             get_is_sponsored_images_loaded_callback_.Run());
  }
  if (get_ntp_sponsored_images_manifest_version_callback_) {
    if (const std::optional<std::string> manifest_version =
            get_ntp_sponsored_images_manifest_version_callback_.Run()) {
      dict.Set(kNtpSponsoredImagesManifestVersionKey, *manifest_version);
    }
  }

  return dict;
}

void AdsInternalsHandler::OnGetDiagnostics(
    GetDiagnosticsCallback callback,
    base::DictValue dict,
    std::optional<base::DictValue> diagnostics) {
  if (diagnostics) {
    dict.Merge(std::move(*diagnostics));
  }

  WriteDiagnosticsJson(std::move(callback), std::move(dict));
}

void AdsInternalsHandler::WriteDiagnosticsJson(GetDiagnosticsCallback callback,
                                               base::DictValue dict) {
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

void AdsInternalsHandler::OnBraveRewardsWalletConnectedPrefChanged(
    const std::string& /*path*/) {
  UpdateBraveRewardsWalletConnected();
}

void AdsInternalsHandler::UpdateBraveRewardsWalletConnected() {
  if (!ads_internals_page_remote_) {
    return;
  }

  const bool is_connected =
      !prefs_->GetString(brave_rewards::prefs::kExternalWalletType).empty();
  ads_internals_page_remote_->UpdateBraveRewardsWalletConnected(is_connected);
}

void AdsInternalsHandler::OnDidInitializeAdsService() {
  if (ads_internals_page_remote_) {
    ads_internals_page_remote_->UpdateDidInitializeAdsService();
  }
}
