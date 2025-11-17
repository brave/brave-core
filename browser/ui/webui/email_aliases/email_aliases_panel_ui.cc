// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/email_aliases/email_aliases_panel_ui.h"

#include "brave/browser/email_aliases/email_aliases_service_factory.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources.h"
#include "brave/browser/ui/webui/email_aliases/email_aliases_panel_handler.h"
#include "brave/browser/ui/webui/settings/brave_settings_localized_strings_provider.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ui/webui/brave_color_change_listener/brave_color_change_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

namespace {

const webui::ResourcePath kEmailAliasesPanelResources[] = {
    {"email_aliases_panel.bundle.js",
     IDR_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_BUNDLE_JS},
};
}

EmailAliasesPanelUI::EmailAliasesPanelUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui) {
  auto* profile = Profile::FromWebUI(web_ui);
  auto* source =
      content::WebUIDataSource::CreateAndAdd(profile, kEmailAliasesPanelHost);
  settings::BraveAddLocalizedStrings(source, profile);
  // Allow styled-components and theming in the Top Chrome panel.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome://resources chrome://theme;");
  webui::SetupWebUIDataSource(source, kEmailAliasesPanelResources,
                              IDR_EMAIL_ALIASES_PANEL_HTML);
  content::URLDataSource::Add(profile, std::make_unique<ThemeSource>(profile));

  // Ensure the bubble auto-resizes to content like other panels.
  if (auto embedder_ptr = embedder()) {
    embedder_ptr->ShowUI();
  }
}

EmailAliasesPanelUI::~EmailAliasesPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(EmailAliasesPanelUI)

void EmailAliasesPanelUI::BindInterface(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  email_aliases::EmailAliasesServiceFactory::BindForProfile(
      profile, std::move(receiver));
}

void EmailAliasesPanelUI ::BindInterface(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesPanelHandler>
        receiver) {
  panel_handler_ =
      std::make_unique<EmailAliasesPanelHandler>(this, std::move(receiver));
}

void EmailAliasesPanelUI::BindInterface(
    mojo::PendingReceiver<color_change_listener::mojom::PageHandler>
        pending_receiver) {
  ui::BraveColorChangeHandler::BindInterface(web_ui()->GetWebContents(),
                                             std::move(pending_receiver));
}

bool EmailAliasesPanelUIConfig::ShouldAutoResizeHost() {
  return true;
}
