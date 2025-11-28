// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/email_aliases/email_aliases_panel_ui.h"

#include "brave/browser/email_aliases/email_aliases_service_factory.h"
#include "brave/browser/resources/settings/grit/brave_settings_resources.h"
#include "brave/browser/ui/webui/settings/brave_settings_localized_strings_provider.h"
#include "brave/components/constants/webui_url_constants.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

namespace {

constexpr webui::ResourcePath kEmailAliasesPanelResources[] = {
    {"email_aliases_panel.bundle.js",
     IDR_EMAIL_ALIASES_EMAIL_ALIASES_PANEL_BUNDLE_JS},
};
}

EmailAliasesPanelUI::EmailAliasesPanelUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui) {
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
}

EmailAliasesPanelUI::~EmailAliasesPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(EmailAliasesPanelUI)

void EmailAliasesPanelUI::SetHandlerDelegate(
    email_aliases::mojom::EmailAliasesPanelHandler* delegate) {
  CHECK(!delegate_ && delegate);
  delegate_ = delegate;
}

void EmailAliasesPanelUI::BindInterface(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  email_aliases::EmailAliasesServiceFactory::BindForProfile(
      profile, std::move(receiver));
}

void EmailAliasesPanelUI ::BindInterface(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesPanelHandler>
        receiver) {
  panel_handler_.reset();
  panel_handler_.Bind(std::move(receiver));
}

void EmailAliasesPanelUI::OnAliasCreated(const std::string& email) {
  if (delegate_) {
    delegate_->OnAliasCreated(email);
  }
}

void EmailAliasesPanelUI::OnManageAliases() {
  if (delegate_) {
    delegate_->OnManageAliases();
  }
}
void EmailAliasesPanelUI::OnCancelAliasCreation() {
  if (delegate_) {
    delegate_->OnCancelAliasCreation();
  }
}

EmailAliasesPanelUIConfig::EmailAliasesPanelUIConfig()
    : content::DefaultWebUIConfig<EmailAliasesPanelUI>(content::kChromeUIScheme,
                                                       kEmailAliasesPanelHost) {
}
