// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_ui.h"

#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/speedreader/common/constants.h"
#include "brave/components/speedreader/resources/panel/grit/brave_speedreader_toolbar_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/theme_handler.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"

SpeedreaderToolbarUI::SpeedreaderToolbarUI(content::WebUI* web_ui,
                                           const std::string& name)
    : ui::MojoBubbleWebUIController(web_ui, true),
      profile_(Profile::FromWebUI(web_ui)) {
  browser_ = chrome::FindLastActiveWithProfile(profile_);

  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, name, kBraveSpeedreaderToolbarGenerated,
      kBraveSpeedreaderToolbarGeneratedSize, IDR_SPEEDREADER_UI_HTML);

  // Set up the chrome://theme/ source.
  web_ui->AddMessageHandler(std::make_unique<ThemeHandler>());

  for (const auto& str : speedreader::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    source->AddString(str.name, l10n_str);
  }

  AddBackgroundColorToSource(source, web_ui->GetWebContents());
}

SpeedreaderToolbarUI::~SpeedreaderToolbarUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(SpeedreaderToolbarUI)

void SpeedreaderToolbarUI::BindInterface(
    mojo::PendingReceiver<speedreader::mojom::ToolbarFactory> receiver) {
  toolbar_factory_.reset();
  toolbar_factory_.Bind(std::move(receiver));
}

void SpeedreaderToolbarUI::CreateInterfaces(
    mojo::PendingReceiver<speedreader::mojom::ToolbarDataHandler>
        panel_data_handler,
    mojo::PendingRemote<speedreader::mojom::ToolbarEventsHandler>
        panel_events_handler) {
  toolbar_data_handler_ = std::make_unique<SpeedreaderToolbarDataHandlerImpl>(
      std::move(panel_data_handler), std::move(panel_events_handler), browser_);
}
