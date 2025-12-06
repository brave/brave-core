/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/new_tab_takeover_ui/new_tab_takeover_ui_ios.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/new_tab_takeover/grit/new_tab_takeover_generated_map.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "components/grit/brave_components_resources.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web/public/webui/web_ui_ios_message_handler.h"
#include "url/gurl.h"

namespace {

ProfileIOS* GetProfile(web::WebUIIOS* web_ui) {
  ProfileIOS* const profile = ProfileIOS::FromWebUIIOS(web_ui);
  CHECK(profile);
  return profile;
}

web::WebUIIOSDataSource* CreateAndAddWebUIDataSource(
    web::WebUIIOS* web_ui,
    const std::string& name,
    base::span<const webui::ResourcePath> resource_paths,
    int html_resource_id) {
  auto* source = BraveWebUIIOSDataSource::CreateAndAdd(GetProfile(web_ui),
                                                       kNewTabTakeoverPageHost);

  source->UseStringsJs();
  source->EnableReplaceI18nInJS();

  // Add required resources.
  source->AddResourcePaths(resource_paths);
  source->SetDefaultResource(html_resource_id);

  source->AddString("ntpNewTabTakeoverRichMediaUrl",
                    kNTPNewTabTakeoverRichMediaUrl);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", kNTPNewTabTakeoverRichMediaUrl, ";"}));

  return source;
}

}  // namespace

NewTabTakeoverUIIOS::NewTabTakeoverUIIOS(
    web::WebUIIOS* web_ui,
    const GURL& url,
    ntp_background_images::NTPBackgroundImagesService*
        ntp_background_images_service)
    : web::WebUIIOSController(web_ui, url.GetHost()),
      handler_(ntp_background_images_service) {
  CreateAndAddWebUIDataSource(web_ui, url.GetHost(),
                              base::span(kNewTabTakeoverGenerated),
                              IDR_NEW_TAB_TAKEOVER_HTML);

  // TODO(aseren): Is this needed?
  //  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);

  // Bind Mojom Interface
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&NewTabTakeoverUIIOS::BindInterface,
                          weak_ptr_factory_.GetWeakPtr()));
}

NewTabTakeoverUIIOS::~NewTabTakeoverUIIOS() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      new_tab_takeover::mojom::NewTabTakeover::Name_);
}

void NewTabTakeoverUIIOS::BindInterface(
    mojo::PendingReceiver<new_tab_takeover::mojom::NewTabTakeover>
        pending_receiver) {
  LOG(ERROR) << "FOOBAR.BindInterface";
  handler_.BindInterface(std::move(pending_receiver));
}
