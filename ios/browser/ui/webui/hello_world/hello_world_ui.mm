// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/hello_world/hello_world_ui.h"

#include "base/version_info/version_info.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/hello_world/resources/grit/hello_world_resources.h"
#include "brave/components/hello_world/resources/grit/hello_world_resources_map.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "url/gurl.h"

HelloWorldUI::HelloWorldUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()) {
  web::WebUIIOSDataSource* source = web::WebUIIOSDataSource::Create(url.host());
  web::WebUIIOSDataSource::Add(ProfileIOS::FromWebUIIOS(web_ui), source);
  source->UseStringsJs();
  source->AddResourcePaths(kHelloWorldResources);
  source->SetDefaultResource(IDR_HELLO_WORLD_HELLO_WORLD_HTML);
  source->AddString("platform", std::string(version_info::GetOSType()));
}

HelloWorldUI::~HelloWorldUI() = default;
