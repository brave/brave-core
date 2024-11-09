// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/email_aliases/email_aliases_bubble_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/settings/brave_email_aliases_handler.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/email_aliases/browser/resources/grit/email_aliases_bubble_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/common/url_constants.h"

namespace email_aliases {

EmailAliasesBubbleUI::EmailAliasesBubbleUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  auto* source = CreateAndAddWebUIDataSource(
      web_ui, kEmailAliasesBubbleHost, kEmailAliasesBubbleGenerated,
      kEmailAliasesBubbleGeneratedSize, IDR_EMAIL_ALIASES_BUBBLE_HTML);
  web_ui->AddMessageHandler(std::make_unique<BraveEmailAliasesHandler>());
  DCHECK(source);
}

EmailAliasesBubbleUI::~EmailAliasesBubbleUI() = default;

std::string EmailAliasesBubbleUI::GetWebUIName() {
  return kEmailAliasesBubbleHost;
}

bool EmailAliasesBubbleUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return true;
}

bool EmailAliasesBubbleUIConfig::ShouldAutoResizeHost() {
  return true;
}

WEB_UI_CONTROLLER_TYPE_IMPL(EmailAliasesBubbleUI)

}  // namespace email_aliases
