/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/hello_world/hello_world_ui.h"

#include "base/containers/span.h"
#include "base/version_info/version_info.h"
#include "brave/components/hello_world/resources/grit/hello_world_resources.h"
#include "brave/components/hello_world/resources/grit/hello_world_resources_map.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

HelloWorldUI::HelloWorldUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kChromeUIHelloWorldHost);

  webui::SetupWebUIDataSource(source, kHelloWorldResources,
                              IDR_HELLO_WORLD_HELLO_WORLD_HTML);

  source->AddString("platform", version_info::GetOSType());
}

HelloWorldUI::~HelloWorldUI() = default;
