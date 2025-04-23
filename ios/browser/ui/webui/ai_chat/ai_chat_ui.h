// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AICHAT_AI_CHAT_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AICHAT_AI_CHAT_UI_H_

#include <memory>
#include <string>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "components/prefs/pref_service.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class ProfileIOS;

class AIChatUI : public web::WebUIIOSController {
 public:
  explicit AIChatUI(web::WebUIIOS* web_ui, const GURL& url);
  AIChatUI(const AIChatUI&) = delete;
  AIChatUI& operator=(const AIChatUI&) = delete;
  ~AIChatUI() override;

  void BindInterfaceUIHandler(
      mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver);
  void BindInterfaceChatService(
      mojo::PendingReceiver<ai_chat::mojom::Service> receiver);
  void BindInterfaceParentUIFrame(
      mojo::PendingReceiver<ai_chat::mojom::ParentUIFrame>
          parent_ui_frame_receiver);
  void BindInterfaceTabTracker(
      mojo::PendingReceiver<ai_chat::mojom::TabTrackerService>
          pending_receiver);

 private:
  std::unique_ptr<ai_chat::AIChatUIPageHandler> page_handler_;
  raw_ptr<ProfileIOS> profile_ = nullptr;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AICHAT_AI_CHAT_UI_H_
