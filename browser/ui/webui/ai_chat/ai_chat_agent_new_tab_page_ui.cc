// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_agent_new_tab_page_ui.h"

#include "brave/browser/resources/ai_chat_agent_new_tab_page/grit/ai_chat_agent_new_tab_page_generated_map.h"
#include "brave/browser/resources/ai_chat_agent_new_tab_page/grit/ai_chat_agent_new_tab_page_static_resources.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

AIChatAgentNewTabPageUI::AIChatAgentNewTabPageUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  CHECK(profile->IsAIChatAgent());
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      profile, chrome::kChromeUINewTabHost);
  webui::SetupWebUIDataSource(
      source, kAiChatAgentNewTabPageGenerated,
      IDR_AI_CHAT_AGENT_NEW_TAB_PAGE_STATIC_AI_CHAT_AGENT_NEW_TAB_PAGE_HTML);

  AddBackgroundColorToSource(source, web_ui->GetWebContents());
}

AIChatAgentNewTabPageUI::~AIChatAgentNewTabPageUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(AIChatAgentNewTabPageUI)
