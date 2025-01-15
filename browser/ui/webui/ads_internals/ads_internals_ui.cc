// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ads_internals/ads_internals_ui.h"

#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/resources/grit/ads_internals_generated_map.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

AdsInternalsUI::AdsInternalsUI(content::WebUI* const web_ui,
                               const std::string& name,
                               brave_ads::AdsService* ads_service,
                               PrefService& prefs)
    : content::WebUIController(web_ui), handler_(ads_service, prefs) {
  CreateAndAddWebUIDataSource(web_ui, name, kAdsInternalsGenerated,
                              IDR_ADS_INTERNALS_HTML);
}

AdsInternalsUI::~AdsInternalsUI() = default;

void AdsInternalsUI::BindInterface(
    mojo::PendingReceiver<bat_ads::mojom::AdsInternals> pending_receiver) {
  handler_.BindInterface(std::move(pending_receiver));
}

///////////////////////////////////////////////////////////////////////////////

WEB_UI_CONTROLLER_TYPE_IMPL(AdsInternalsUI)
