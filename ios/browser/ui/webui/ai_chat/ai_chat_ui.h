// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/bookmarks.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/history.mojom-forward.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

class ProfileIOS;

namespace web {
class WebUIIOS;
}

namespace ai_chat {
class AIChatUIPageHandler;
class BookmarksPageHandler;
class HistoryUIHandler;
}  // namespace ai_chat

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
  void BindInterfaceHistoryUIHandler(
      mojo::PendingReceiver<ai_chat::mojom::HistoryUIHandler> receiver);
  void BindInterfaceBookmarksPageHandler(
      mojo::PendingReceiver<ai_chat::mojom::BookmarksPageHandler> receiver);

 private:
  std::unique_ptr<ai_chat::AIChatUIPageHandler> page_handler_;
  raw_ptr<ProfileIOS> profile_ = nullptr;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_
