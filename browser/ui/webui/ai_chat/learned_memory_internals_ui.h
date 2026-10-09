// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEARNED_MEMORY_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEARNED_MEMORY_INTERNALS_UI_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/learned_memory_internals.mojom.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

class Profile;

namespace ai_chat {

class LearnedMemoryInternalsUI;

// brave://leo-memory-internals, while the learned memory feature is on.
class LearnedMemoryInternalsUIConfig
    : public content::DefaultWebUIConfig<LearnedMemoryInternalsUI> {
 public:
  LearnedMemoryInternalsUIConfig();

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

// Shows the Dreaming runs of the profile: each request to the decision model
// and to the LLM, the answers, and the decisions made from them.
class LearnedMemoryInternalsUI : public ui::MojoWebUIController,
                                 public mojom::LearnedMemoryInternals {
 public:
  explicit LearnedMemoryInternalsUI(content::WebUI* web_ui);
  LearnedMemoryInternalsUI(const LearnedMemoryInternalsUI&) = delete;
  LearnedMemoryInternalsUI& operator=(const LearnedMemoryInternalsUI&) =
      delete;
  ~LearnedMemoryInternalsUI() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::LearnedMemoryInternals> receiver);

  // mojom::LearnedMemoryInternals:
  void GetDreamingRuns(GetDreamingRunsCallback callback) override;

 private:
  raw_ptr<Profile> profile_;
  mojo::Receiver<mojom::LearnedMemoryInternals> receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEARNED_MEMORY_INTERNALS_UI_H_
