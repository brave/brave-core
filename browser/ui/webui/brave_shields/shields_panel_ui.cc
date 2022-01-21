// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"

#include <utility>

#include "base/bind.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/resources/panel/grit/brave_shields_panel_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"

ShieldsPanelUI::ShieldsPanelUI(content::WebUI* web_ui)
    : ui::MojoBubbleWebUIController(web_ui, true) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kShieldsPanelHost);

  source->AddLocalizedStrings(brave_shields::kLocalizedStrings);

  webui::SetupWebUIDataSource(source,
                              base::make_span(kBraveShieldsPanelGenerated,
                                              kBraveShieldsPanelGeneratedSize),
                              IDR_SHIELDS_PANEL_HTML);

  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                source);
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

  panel_handler_ =
      std::make_unique<ShieldsPanelHandler>(std::move(panel_receiver), this);

  data_handler_ = std::make_unique<ShieldsPanelDataHandler>(
      std::move(data_handler_receiver), this);
}
