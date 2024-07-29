// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_ui.h"

#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/speedreader/common/constants.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/resources/panel/grit/brave_speedreader_toolbar_generated_map.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/prefs/prefs_tab_helper.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/utils.h"
#endif

SpeedreaderToolbarUI::SpeedreaderToolbarUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui, true),
      profile_(Profile::FromWebUI(web_ui)) {
  content::HostZoomMap::Get(web_ui->GetWebContents()->GetSiteInstance())
      ->SetZoomLevelForHostAndScheme(content::kChromeUIScheme,
                                     kSpeedreaderPanelHost, 0);

  browser_ = chrome::FindLastActiveWithProfile(profile_);

  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, kSpeedreaderPanelHost, kBraveSpeedreaderToolbarGenerated,
      kBraveSpeedreaderToolbarGeneratedSize, IDR_SPEEDREADER_UI_HTML);

  for (const auto& str : speedreader::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    source->AddString(str.name, l10n_str);
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  source->AddBoolean("aiChatFeatureEnabled",
                     ai_chat::IsAIChatEnabled(profile_->GetPrefs()) &&
                         profile_->IsRegularProfile());
#else
  source->AddBoolean("aiChatFeatureEnabled", false);
#endif
  source->AddBoolean("ttsEnabled",
                     speedreader::features::IsSpeedreaderEnabled() &&
                         speedreader::kSpeedreaderTTS.Get());
  PrefsTabHelper::CreateForWebContents(web_ui->GetWebContents());
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
        toolbar_data_handler,
    mojo::PendingRemote<speedreader::mojom::ToolbarEventsHandler>
        toolbar_events_handler) {
  toolbar_data_handler_ = std::make_unique<SpeedreaderToolbarDataHandlerImpl>(
      browser_, std::move(toolbar_data_handler),
      std::move(toolbar_events_handler));
}

SpeedreaderToolbarUIConfig::SpeedreaderToolbarUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                  kSpeedreaderPanelHost) {}

bool SpeedreaderToolbarUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return true;
}

bool SpeedreaderToolbarUIConfig::ShouldAutoResizeHost() {
  return true;
}
