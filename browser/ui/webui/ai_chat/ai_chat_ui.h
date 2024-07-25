// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_

#include <memory>
#include <string>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace content {
class BrowserContext;
}

class Profile;

class AIChatUI : public ui::UntrustedWebUIController {
 public:
  explicit AIChatUI(content::WebUI* web_ui);
  AIChatUI(const AIChatUI&) = delete;
  AIChatUI& operator=(const AIChatUI&) = delete;
  ~AIChatUI() override;

  void BindInterface(
      mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver);

  // Set by WebUIContentsWrapperT. TopChromeWebUIController provides default
  // implementation for this but we don't use it.
  void set_embedder(
      base::WeakPtr<TopChromeWebUIController::Embedder> embedder) {
    embedder_ = embedder;
  }

  static constexpr std::string GetWebUIName() { return "AIChatPanel"; }

 private:
  std::unique_ptr<ai_chat::mojom::PageHandler> page_handler_;

  base::WeakPtr<TopChromeWebUIController::Embedder> embedder_;
  raw_ptr<Profile> profile_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedChatUIConfig : public DefaultTopChromeWebUIConfig<AIChatUI> {
 public:
  UntrustedChatUIConfig();
  ~UntrustedChatUIConfig() override = default;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_
