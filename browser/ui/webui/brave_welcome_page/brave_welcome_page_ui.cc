// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page_ui.h"

#include "brave/browser/resources/brave_welcome_page/grit/brave_welcome_page_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace {

inline constexpr char kBraveWelcomePageHost[] = "welcome-new";

}  // namespace

BraveWelcomePageUI::BraveWelcomePageUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kBraveWelcomePageHost);

  webui::SetupWebUIDataSource(source, kBraveWelcomePageGenerated,
                              IDR_BRAVE_WELCOME_PAGE_HTML);
}

BraveWelcomePageUI::~BraveWelcomePageUI() = default;

BraveWelcomePageUIConfig::BraveWelcomePageUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveWelcomePageHost) {}

bool BraveWelcomePageUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  auto* profile = Profile::FromBrowserContext(browser_context);
  return !profile->IsGuestSession();
}
