// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ads/ads_internals_ui.h"

#include <cstddef>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/browser/resources/grit/ads_internals_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/brave_ads/ads_service_factory_ios.h"
#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ui/base/webui/resource_path.h"

namespace {

web::WebUIIOSDataSource* CreateAndAddWebUIDataSource(
    web::WebUIIOS* web_ui,
    const std::string& name,
    base::span<const webui::ResourcePath> resource_paths,
    int html_resource_id) {
  web::WebUIIOSDataSource* source = web::WebUIIOSDataSource::Create(name);
  web::WebUIIOSDataSource::Add(ProfileIOS::FromWebUIIOS(web_ui), source);
  source->UseStringsJs();

  // Add required resources.
  source->AddResourcePaths(resource_paths);
  source->SetDefaultResource(html_resource_id);
  return source;
}

ProfileIOS* GetProfile(web::WebUIIOS* web_ui) {
  ProfileIOS* const profile = ProfileIOS::FromWebUIIOS(web_ui);
  CHECK(profile);
  return profile;
}

}  // namespace

AdsInternalsUI::AdsInternalsUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()),
      handler_(brave_ads::AdsServiceFactoryIOS::GetForBrowserState(
                   GetProfile(web_ui)),
               GetProfile(web_ui)->GetPrefs()) {
  CreateAndAddWebUIDataSource(web_ui, url.host(),
                              base::span(kAdsInternalsGenerated),
                              IDR_ADS_INTERNALS_HTML);

  // Bind Mojom Interface
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&AdsInternalsUI::BindInterface,
                          weak_ptr_factory_.GetWeakPtr()));
}

AdsInternalsUI::~AdsInternalsUI() {}

void AdsInternalsUI::BindInterface(
    mojo::PendingReceiver<bat_ads::mojom::AdsInternals> pending_receiver) {
  handler_.BindInterface(std::move(pending_receiver));
}
