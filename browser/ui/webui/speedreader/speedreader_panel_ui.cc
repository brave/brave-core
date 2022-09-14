// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_panel_ui.h"

#include <utility>

#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/speedreader/speedreader_panel_handler_impl.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/speedreader/resources/panel/grit/brave_speedreader_panel_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"

SpeedreaderPanelUI::SpeedreaderPanelUI(content::WebUI* web_ui,
                                       const std::string& name)
    : ui::MojoBubbleWebUIController(web_ui, false),
      profile_(Profile::FromWebUI(web_ui)) {
  browser_ = chrome::FindLastActiveWithProfile(profile_);

  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, name, kBraveSpeedreaderPanelGenerated,
      kBraveSpeedreaderPanelGeneratedSize, IDR_SPEEDREADER_UI_HTML);

  AddBackgroundColorToSource(source, web_ui->GetWebContents());
}

SpeedreaderPanelUI::~SpeedreaderPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(SpeedreaderPanelUI)

void SpeedreaderPanelUI::BindInterface(
    mojo::PendingReceiver<speedreader::mojom::PanelFactory> receiver) {
  panel_factory_.reset();
  panel_factory_.Bind(std::move(receiver));
}

void SpeedreaderPanelUI::CreateInterfaces(
    mojo::PendingReceiver<speedreader::mojom::PanelHandler> panel_handler,
    mojo::PendingReceiver<speedreader::mojom::PanelDataHandler>
        panel_data_handler) {
  panel_handler_ = std::make_unique<SpeedreaderPanelHandlerImpl>(
      std::move(panel_handler), this);
  panel_data_handler_ = std::make_unique<SpeedreaderPanelDataHandlerImpl>(
      std::move(panel_data_handler),
      browser_->tab_strip_model()->GetActiveWebContents());
}
