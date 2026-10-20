// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_transfer/brave_transfer_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_transfer/brave_transfer_service_factory.h"
#include "brave/browser/ui/webui/brave_transfer/brave_transfer_page_handler.h"
#include "brave/components/brave_transfer/core/features.h"
#include "brave/components/brave_transfer/core/pref_names.h"
#include "brave/components/brave_transfer/resources/page/grit/brave_transfer_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

namespace brave_transfer {

namespace {

void CreateAndAddDataSource(Profile* profile) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kBraveTransferHost);

  // The page bundle does not create Trusted Types policies.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome://resources chrome://theme;");

  source->UseStringsJs();
  source->SetDefaultResource(IDR_BRAVE_TRANSFER_HTML);
  source->AddResourcePaths(kBraveTransferGenerated);

  source->AddLocalizedStrings(webui::kBraveTransferStrings);
  source->AddBoolean("isIOS", false);
  source->AddBoolean("isMobile", false);

  content::URLDataSource::Add(profile, std::make_unique<ThemeSource>(profile));
}

}  // namespace

bool BraveTransferUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  if (!features::IsBraveTransferEnabled()) {
    return false;
  }
  Profile* profile = Profile::FromBrowserContext(browser_context);
  return profile->IsRegularProfile() &&
         profile->GetPrefs()->GetBoolean(prefs::kEnabled);
}

BraveTransferUI::BraveTransferUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  CreateAndAddDataSource(Profile::FromWebUI(web_ui));
}

BraveTransferUI::~BraveTransferUI() = default;

void BraveTransferUI::BindInterface(
    mojo::PendingReceiver<mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void BraveTransferUI::CreatePageHandler(
    mojo::PendingRemote<mojom::Page> page,
    mojo::PendingReceiver<mojom::PageHandler> handler) {
  BraveTransferService* service =
      BraveTransferServiceFactory::GetForProfile(Profile::FromWebUI(web_ui()));
  if (!service) {
    return;
  }
  page_handler_ = std::make_unique<BraveTransferPageHandler>(
      std::move(handler), std::move(page), service, web_ui()->GetWebContents());
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveTransferUI)

}  // namespace brave_transfer
