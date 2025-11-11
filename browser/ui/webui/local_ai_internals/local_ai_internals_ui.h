// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"

namespace local_ai {

// Trusted WebUI at chrome://local-ai-internals
class LocalAIInternalsUI : public content::WebUIController {
 public:
  explicit LocalAIInternalsUI(content::WebUI* web_ui);
  LocalAIInternalsUI(const LocalAIInternalsUI&) = delete;
  LocalAIInternalsUI& operator=(const LocalAIInternalsUI&) = delete;
  ~LocalAIInternalsUI() override;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class LocalAIInternalsUIConfig
    : public content::DefaultWebUIConfig<LocalAIInternalsUI> {
 public:
  LocalAIInternalsUIConfig();
  ~LocalAIInternalsUIConfig() override = default;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_UI_WEBUI_LOCAL_AI_INTERNALS_LOCAL_AI_INTERNALS_UI_H_
