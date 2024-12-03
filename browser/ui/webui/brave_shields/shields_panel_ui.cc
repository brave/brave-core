// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"

#include <utility>

#include "brave/browser/ui/brave_browser_window.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_localized_strings.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/resources/panel/grit/brave_shields_panel_generated_map.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "net/base/features.h"

// Cache active Browser instance's TabStripModel to give
// to ShieldsPanelDataHandler when this is created because
// CreatePanelHandler() is run in async.
ShieldsPanelUI::ShieldsPanelUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui, true),
      profile_(Profile::FromWebUI(web_ui)) {
  browser_ = chrome::FindLastActiveWithProfile(profile_);

  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kShieldsPanelHost);

  for (const auto& str : brave_shields::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    source->AddString(str.name, l10n_str);
  }

  source->AddBoolean("isAdvancedViewEnabled", profile_->GetPrefs()->GetBoolean(
                                                  kShieldsAdvancedViewEnabled));

  source->AddBoolean("isHttpsByDefaultEnabled",
                     brave_shields::IsHttpsByDefaultFeatureEnabled());

  source->AddBoolean(
      "showStrictFingerprintingMode",
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveShowStrictFingerprintingMode));

  source->AddBoolean("isTorProfile", profile_->IsTor());

  source->AddBoolean("isForgetFirstPartyStorageEnabled",
                     base::FeatureList::IsEnabled(
                         net::features::kBraveForgetFirstPartyStorage));

  source->AddBoolean(
      "isWebcompatExceptionsServiceEnabled",
      base::FeatureList::IsEnabled(
          webcompat::features::kBraveWebcompatExceptionsService));

  content::URLDataSource::Add(
      profile_, std::make_unique<FaviconSource>(
                    profile_, chrome::FaviconUrlFormat::kFavicon2));

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
  DCHECK(profile);

  panel_handler_ = std::make_unique<ShieldsPanelHandler>(
      std::move(panel_receiver), this,
      static_cast<BraveBrowserWindow*>(browser_->window()), profile);
  data_handler_ = std::make_unique<ShieldsPanelDataHandler>(
      std::move(data_handler_receiver), this, browser_->tab_strip_model());
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
