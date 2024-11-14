// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ads_internals/ads_internals_ui.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/resources/grit/ads_internals_generated_map.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

AdsInternalsUI::AdsInternalsUI(content::WebUI* const web_ui,
                               const std::string& name,
                               brave_ads::AdsService* ads_service,
                               PrefService* prefs)
    : content::WebUIController(web_ui),
      ads_service_(ads_service),
      prefs_(prefs) {
  CHECK(ads_service_);
  CHECK(prefs_);

  CreateAndAddWebUIDataSource(web_ui, name, kAdsInternalsGenerated,
                              kAdsInternalsGeneratedSize,
                              IDR_ADS_INTERNALS_HTML);

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      brave_rewards::prefs::kEnabled,
      base::BindRepeating(&AdsInternalsUI::OnPrefChanged,
                          weak_ptr_factory_.GetWeakPtr()));
}

AdsInternalsUI::~AdsInternalsUI() = default;

void AdsInternalsUI::BindInterface(
    mojo::PendingReceiver<bat_ads::mojom::AdsInternals> pending_receiver) {
  if (receiver_.is_bound()) {
    receiver_.reset();
  }

  receiver_.Bind(std::move(pending_receiver));
}

///////////////////////////////////////////////////////////////////////////////

void AdsInternalsUI::GetAdsInternals(GetAdsInternalsCallback callback) {
  if (!ads_service_) {
    return std::move(callback).Run(/*response=*/"");
  }

  ads_service_->GetInternals(
      base::BindOnce(&AdsInternalsUI::GetInternalsCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsInternalsUI::ClearAdsData(brave_ads::ClearDataCallback callback) {
  if (!ads_service_) {
    return std::move(callback).Run(/*success=*/false);
  }

  ads_service_->ClearData(std::move(callback));
}

void AdsInternalsUI::CreateAdsInternalsPageHandler(
    mojo::PendingRemote<bat_ads::mojom::AdsInternalsPage> page) {
  page_ = mojo::Remote<bat_ads::mojom::AdsInternalsPage>(std::move(page));

  UpdateBraveRewardsEnabled();
}

void AdsInternalsUI::GetInternalsCallback(
    GetAdsInternalsCallback callback,
    std::optional<base::Value::List> value) {
  if (!value) {
    return std::move(callback).Run("");
  }

  std::string json;
  CHECK(base::JSONWriter::Write(*value, &json));
  std::move(callback).Run(json);
}

void AdsInternalsUI::OnPrefChanged(const std::string& path) {
  if (path == brave_rewards::prefs::kEnabled) {
    UpdateBraveRewardsEnabled();
  }
}

void AdsInternalsUI::UpdateBraveRewardsEnabled() {
  if (!page_) {
    return;
  }

  const bool rewards_enabled =
      prefs_->GetBoolean(brave_rewards::prefs::kEnabled);
  page_->OnBraveRewardsEnabledChanged(rewards_enabled);
}

WEB_UI_CONTROLLER_TYPE_IMPL(AdsInternalsUI)
