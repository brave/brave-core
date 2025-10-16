// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_localized_strings.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/resources/panel/grit/brave_shields_panel_generated_map.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "net/base/features.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/webui/webui_util.h"

// Cache active Browser instance's TabStripModel to give
// to ShieldsPanelDataHandler when this is created because
// CreatePanelHandler() is run in async.
ShieldsPanelUI::ShieldsPanelUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui, true),
      profile_(Profile::FromWebUI(web_ui)) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kShieldsPanelHost);

  for (const auto& str : brave_shields::kLocalizedStrings) {
    std::u16string l10n_str = l10n_util::GetStringUTF16(str.id);
    source->AddString(str.name, l10n_str);
  }

  source->AddBoolean("isAdvancedViewEnabled", profile_->GetPrefs()->GetBoolean(
                                                  kShieldsAdvancedViewEnabled));

  source->AddBoolean(
      "isHttpsByDefaultEnabled",
      base::FeatureList::IsEnabled(net::features::kBraveHttpsByDefault));

  source->AddBoolean(
      "showStrictFingerprintingMode",
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveShowStrictFingerprintingMode));

  source->AddBoolean("isTorProfile", profile_->IsTor());

  source->AddBoolean("isBraveForgetFirstPartyStorageFeatureEnabled",
                     base::FeatureList::IsEnabled(
                         net::features::kBraveForgetFirstPartyStorage));

  source->AddBoolean(
      "isWebcompatExceptionsServiceEnabled",
      base::FeatureList::IsEnabled(
          webcompat::features::kBraveWebcompatExceptionsService));

  content::URLDataSource::Add(
      profile_, std::make_unique<FaviconSource>(
                    profile_, chrome::FaviconUrlFormat::kFavicon2));
  content::URLDataSource::Add(profile_,
                              std::make_unique<ThemeSource>(profile_));
  webui::SetupWebUIDataSource(source, kBraveShieldsPanelGenerated,
                              IDR_SHIELDS_PANEL_HTML);
}

ShieldsPanelUI::~ShieldsPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(ShieldsPanelUI)

void ShieldsPanelUI::BindInterface(
    mojo::PendingReceiver<brave_shields::mojom::PanelHandlerFactory> receiver) {
  panel_factory_receiver_.reset();
  panel_factory_receiver_.Bind(std::move(receiver));
}

void ShieldsPanelUI::CreatePanelHandler(
    mojo::PendingReceiver<brave_shields::mojom::PanelHandler> panel_receiver,
    mojo::PendingReceiver<brave_shields::mojom::DataHandler>
        data_handler_receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  CHECK(profile);
  panel_handler_ = std::make_unique<ShieldsPanelHandler>(
      std::move(panel_receiver), this, profile);
  auto* browser = webui::GetBrowserWindowInterface(web_ui()->GetWebContents());
  CHECK(browser);
  data_handler_ = std::make_unique<ShieldsPanelDataHandler>(
      std::move(data_handler_receiver), this, browser->GetTabStripModel());
}

ShieldsPanelUIConfig::ShieldsPanelUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme, kShieldsPanelHost) {
}

bool ShieldsPanelUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return true;
}

bool ShieldsPanelUIConfig::ShouldAutoResizeHost() {
  return true;
}
