// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_REWRITER_AI_REWRITER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_REWRITER_AI_REWRITER_UI_H_

#include <string>

#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
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

  // mojom::AIRewriterPageHandler:
  void Close() override;
  void OpenSettings() override;
  void GetInitialText(GetInitialTextCallback callback) override;

 private:
  raw_ptr<Profile> profile_ = nullptr;
  bool dialog_closed_ = false;

  mojo::Receiver<mojom::AIRewriterPageHandler> receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace ai_rewriter

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_REWRITER_AI_REWRITER_UI_H_
