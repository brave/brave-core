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
#include "brave/browser/ui/webui/instant_service_message_handler.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_new_tab/resources/grit/brave_new_tab_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "chrome/browser/search/instant_service_factory.h"
#include "components/grit/brave_components_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

BraveNewTabUI::BraveNewTabUI(content::WebUI* web_ui, const std::string& name)
    : WebUIController(web_ui),
      profile_(Profile::FromWebUI(web_ui)),
      instant_service_(InstantServiceFactory::GetForProfile(profile_)) {

  content::WebUIDataSource* source = CreateBasicUIHTMLSource(profile_, name,
      kBraveNewTabGenerated, kBraveNewTabGeneratedSize, IDR_BRAVE_NEW_TAB_HTML);
  web_ui->AddMessageHandler(base::WrapUnique(
    BraveNewTabMessageHandler::Create(source, profile_)));
  web_ui->AddMessageHandler(base::WrapUnique(
    InstantServiceMessageHandler::Create(source, profile_, instant_service_)));
  content::WebUIDataSource::Add(profile_, source);
  web_ui->OverrideTitle(l10n_util::GetStringUTF16(IDS_NEW_TAB_TITLE));

  // OnNewTabPageOpened refreshes the most visited entries while
  // UpdateMostVisitedInfo triggers a call to MostVisitedInfoChanged.
  instant_service_->OnNewTabPageOpened();
  instant_service_->UpdateMostVisitedInfo();
}

BraveNewTabUI::~BraveNewTabUI() {
}
