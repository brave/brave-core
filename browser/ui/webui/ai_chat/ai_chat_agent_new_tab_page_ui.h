// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_AGENT_NEW_TAB_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_AGENT_NEW_TAB_PAGE_UI_H_

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "content/public/browser/web_ui_controller.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE));

class AIChatAgentNewTabPageUI : public content::WebUIController {
 public:
  explicit AIChatAgentNewTabPageUI(content::WebUI* web_ui);
  ~AIChatAgentNewTabPageUI() override;
  AIChatAgentNewTabPageUI(const AIChatAgentNewTabPageUI&) = delete;
  AIChatAgentNewTabPageUI& operator=(const AIChatAgentNewTabPageUI&) = delete;

 private:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_AGENT_NEW_TAB_PAGE_UI_H_
