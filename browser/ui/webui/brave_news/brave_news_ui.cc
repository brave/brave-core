// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_news/brave_news_ui.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/ui/webui/brave_sanitized_image_source.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/resources/grit/brave_news_sidebar_generated_map.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

BraveNewsUI::BraveNewsUI(content::WebUI* web_ui)
    : UntrustedTopChromeWebUIController(web_ui) {
  // UntrustedTopChromeWebUIController clears all bindings; re-enable Mojo so
  // the page can talk to the BraveNewsController over the interface broker.
  web_ui->SetBindings(
      content::BindingsPolicySet({content::BindingsPolicyValue::kMojoWebUi}));

  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  controller_ = brave_news::BraveNewsControllerFactory::GetForBrowserContext(
      browser_context);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(browser_context, kBraveNewsURL);
  webui::SetupWebUIDataSource(source, kBraveNewsSidebarGenerated,
                              IDR_BRAVE_NEWS_PANEL_HTML);
  source->AddLocalizedStrings(webui::kBraveNewsStrings);
  source->AddBoolean(
      "featureFlagBraveNewsFeedV2Enabled",
      base::FeatureList::IsEnabled(brave_news::features::kBraveNewsFeedUpdate));
  source->AddBoolean("featureFlagSearchWidget", false);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome-untrusted://resources chrome-untrusted://brave-image "
      "blob: 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src chrome-untrusted://resources chrome-untrusted://theme 'self' "
      "'unsafe-inline';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src chrome-untrusted://resources 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome-untrusted://resources 'self';");

  auto* profile = Profile::FromBrowserContext(browser_context);
  content::URLDataSource::Add(profile,
                              std::make_unique<BraveSanitizedImageSource>(
                                  profile, /*serve_untrusted=*/true));
  content::URLDataSource::Add(profile, std::make_unique<ThemeSource>(
                                           profile, /*serve_untrusted=*/true));
}

BraveNewsUI::~BraveNewsUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewsUI)

void BraveNewsUI::BindInterface(
    mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver) {
  if (!controller_) {
    return;
  }
  controller_->Bind(std::move(receiver));
}

BraveNewsUIConfig::BraveNewsUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIUntrustedScheme,
                                  kBraveNewsHost) {}

bool BraveNewsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  const bool enabled =
      base::FeatureList::IsEnabled(brave_news::features::kBraveNewsSidebar);
  return enabled;
}
