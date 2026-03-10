// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_INTERNAL_AI_CHAT_INTERNAL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_INTERNAL_AI_CHAT_INTERNAL_UI_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

class Profile;

class AIChatInternalUI : public ui::MojoWebUIController {
 public:
  explicit AIChatInternalUI(content::WebUI* web_ui, std::string_view host);
  AIChatInternalUI(const AIChatInternalUI&) = delete;
  AIChatInternalUI& operator=(const AIChatInternalUI&) = delete;
  ~AIChatInternalUI() override;

  void BindInterface(mojo::PendingReceiver<ai_chat::mojom::Service> receiver);

 private:
  raw_ptr<Profile> profile_ = nullptr;
  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_INTERNAL_AI_CHAT_INTERNAL_UI_H_
