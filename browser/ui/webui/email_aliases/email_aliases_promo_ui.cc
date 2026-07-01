// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/email_aliases/email_aliases_promo_ui.h"

#include "brave/browser/resources/settings/grit/brave_settings_resources.h"
#include "brave/browser/ui/webui/settings/brave_settings_localized_strings_provider.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace {

constexpr webui::ResourcePath kEmailAliasesPromoResources[] = {
    {"email_aliases_promo.bundle.js",
     IDR_EMAIL_ALIASES_EMAIL_ALIASES_PROMO_BUNDLE_JS},
};

inline constexpr char kEmailAliasesPromoHost[] = "email-aliases.promo";

}  // namespace

EmailAliasesPromoUI::EmailAliasesPromoUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui) {
  auto* profile = Profile::FromWebUI(web_ui);
  auto* source =
      content::WebUIDataSource::CreateAndAdd(profile, kEmailAliasesPromoHost);
  settings::BraveAddLocalizedStrings(source, profile);
  // Allow styled-components and theming in the Top Chrome panel.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome://resources chrome://theme;");
  content::URLDataSource::Add(profile, std::make_unique<ThemeSource>(profile));
  webui::SetupWebUIDataSource(source, kEmailAliasesPromoResources,
                              IDR_EMAIL_ALIASES_PROMO_HTML);
}

EmailAliasesPromoUI::~EmailAliasesPromoUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(EmailAliasesPromoUI)

void EmailAliasesPromoUI::SetHandlerDelegate(
    email_aliases::mojom::EmailAliasesPromoHandler* delegate) {
  CHECK(!delegate_ && delegate);
  delegate_ = delegate;
}

void EmailAliasesPromoUI::BindInterface(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesPromoHandler>
        receiver) {
  promo_handler_.reset();
  promo_handler_.Bind(std::move(receiver));
}

void EmailAliasesPromoUI::OnPromoClosed() {
  if (!delegate_) {
    return;
  }

  delegate_->OnPromoClosed();
}

EmailAliasesPromoUIConfig::EmailAliasesPromoUIConfig()
    : content::DefaultWebUIConfig<EmailAliasesPromoUI>(content::kChromeUIScheme,
                                                       kEmailAliasesPromoHost) {
}
