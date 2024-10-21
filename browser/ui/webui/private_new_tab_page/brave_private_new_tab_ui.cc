// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/private_new_tab_page/brave_private_new_tab_ui.h"

#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/private_new_tab_page/brave_private_new_tab_page_handler.h"
#include "brave/components/brave_private_new_tab/resources/page/grit/brave_private_new_tab_generated_map.h"
#include "brave/components/brave_private_new_tab_ui/common/constants.h"
#include "brave/components/brave_private_new_tab_ui/common/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui_data_source.h"

bool BravePrivateNewTabUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  return profile->IsIncognitoProfile() || profile->IsTor() ||
         profile->IsGuestSession();
}

BravePrivateNewTabUI::BravePrivateNewTabUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui, false) {
  Profile* profile = Profile::FromWebUI(web_ui);

  web_ui->OverrideTitle(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_NEW_INCOGNITO_TAB_TITLE));

  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, chrome::kChromeUINewTabHost, kBravePrivateNewTabGenerated,
      kBravePrivateNewTabGeneratedSize, IDR_BRAVE_PRIVATE_NEW_TAB_HTML);

  for (const auto& str : brave_private_new_tab::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    source->AddString(str.name, l10n_str);
  }

  source->AddBoolean("isWindowTor", profile->IsTor());

  AddBackgroundColorToSource(source, web_ui->GetWebContents());
}

BravePrivateNewTabUI::~BravePrivateNewTabUI() = default;

void BravePrivateNewTabUI::BindInterface(
    mojo::PendingReceiver<brave_private_new_tab::mojom::PageHandler> receiver) {
  private_tab_page_handler_ = std::make_unique<BravePrivateNewTabPageHandler>(
      Profile::FromWebUI(web_ui()), web_ui()->GetWebContents(),
      std::move(receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BravePrivateNewTabUI)
