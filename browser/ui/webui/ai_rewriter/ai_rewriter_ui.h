// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_REWRITER_AI_REWRITER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_REWRITER_AI_REWRITER_UI_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/untrusted_web_ui_controller.h"

class Profile;

namespace ai_rewriter {

class AIRewriterUI : public ConstrainedWebDialogUI,
                     public mojom::AIRewriterPageHandler {
 public:
  static constexpr std::string GetWebUIName() { return "AIRewriterPanel"; }

  explicit AIRewriterUI(content::WebUI* web_ui);
  AIRewriterUI(const AIRewriterUI&) = delete;
  AIRewriterUI& operator=(const AIRewriterUI&) = delete;
  ~AIRewriterUI() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::AIRewriterPageHandler> service);

  void set_initial_text(std::string& initial_text) {
    initial_text_ = initial_text;
  }

  // mojom::AIRewriterPageHandler:
  void SetPage(mojo::PendingRemote<mojom::AIRewriterPage> remove) override;
  void Close() override;
  void OpenSettings() override;
  void GetInitialText(GetInitialTextCallback callback) override;
  void RewriteText(const std::string& text,
                   ai_chat::mojom::ActionType action,
                   const std::string& instructions,
                   RewriteTextCallback callback) override;
  void GetActionMenuList(GetActionMenuListCallback callback) override;
  void InsertTextAndClose(const std::string& text,
                          InsertTextAndCloseCallback callback) override;

 private:
  AIRewriterDialogDelegate* GetDialogDelegate();
  content::WebContents* GetTargetContents();

  void OnRewriteSuggestionGenerated(const std::string& rewrite_event);

  raw_ptr<Profile> profile_ = nullptr;
  bool dialog_closed_ = false;
  std::string initial_text_;

  std::unique_ptr<ai_chat::EngineConsumer> ai_engine_;

  mojo::Receiver<mojom::AIRewriterPageHandler> receiver_{this};
  mojo::Remote<mojom::AIRewriterPage> page_;

  base::WeakPtrFactory<AIRewriterUI> weak_ptr_factory_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace ai_rewriter

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_REWRITER_AI_REWRITER_UI_H_
