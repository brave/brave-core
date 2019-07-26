// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_ui.h"

#include <string>

#include "brave/browser/profiles/profile_util.h"
#include "base/memory/ptr_util.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/browser/ui/webui/brave_new_tab_message_handler.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_new_tab/resources/grit/brave_new_tab_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"

BraveNewTabUI::BraveNewTabUI(content::WebUI* web_ui, const std::string& name)
        : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source = CreateBasicUIHTMLSource(profile, name,
      kBraveNewTabGenerated, kBraveNewTabGeneratedSize, IDR_BRAVE_NEW_TAB_HTML);
  web_ui->AddMessageHandler(base::WrapUnique(
    BraveNewTabMessageHandler::Create(source, profile)));
  content::WebUIDataSource::Add(profile, source);
}

BraveNewTabUI::~BraveNewTabUI() {
}
