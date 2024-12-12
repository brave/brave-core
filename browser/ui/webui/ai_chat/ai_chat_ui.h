// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#else
#include "content/public/browser/webui_config.h"
#endif  // #if !BUILDFLAG(IS_ANDROID)

namespace content {
class BrowserContext;
}

class Profile;

class AIChatUI : public ui::MojoWebUIController {
 public:
  explicit AIChatUI(content::WebUI* web_ui);
  AIChatUI(const AIChatUI&) = delete;
  AIChatUI& operator=(const AIChatUI&) = delete;
  ~AIChatUI() override;

  void BindInterface(
      mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver);
  void BindInterface(mojo::PendingReceiver<ai_chat::mojom::Service> receiver);
  void BindInterface(mojo::PendingReceiver<ai_chat::mojom::ParentUIFrame>
                         parent_ui_frame_receiver);

  // Set by WebUIContentsWrapperT. TopChromeWebUIController provides default
  // implementation for this but we don't use it.
  void set_embedder(
      base::WeakPtr<TopChromeWebUIController::Embedder> embedder) {
    embedder_ = embedder;
  }

  static constexpr std::string GetWebUIName() { return "AIChatPanel"; }

 private:
  std::unique_ptr<ai_chat::AIChatUIPageHandler> page_handler_;

  base::WeakPtr<TopChromeWebUIController::Embedder> embedder_;
  raw_ptr<Profile> profile_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#if !BUILDFLAG(IS_ANDROID)
class AIChatUIConfig : public DefaultTopChromeWebUIConfig<AIChatUI> {
#else
class AIChatUIConfig : public content::WebUIConfig {
#endif  // #if !BUILDFLAG(IS_ANDROID)
 public:
  AIChatUIConfig();
  ~AIChatUIConfig() override = default;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

#if BUILDFLAG(IS_ANDROID)
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
#endif  // #if BUILDFLAG(IS_ANDROID)
};

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_
