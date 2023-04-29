// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "brave/browser/ai_chat/ai_chat.mojom.h"
#include "brave/browser/ai_chat/ai_chat_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class TabStripModel;

class AIChatUIPageHandler : public ai_chat::mojom::PageHandler,
                            public TabStripModelObserver,
                            public AIChatTabHelper::Observer {
 public:
  AIChatUIPageHandler(
      TabStripModel* tab_strip_model,
      Profile* profile,
      mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver);

  AIChatUIPageHandler(const AIChatUIPageHandler&) = delete;
  AIChatUIPageHandler& operator=(const AIChatUIPageHandler&) = delete;

  ~AIChatUIPageHandler() override;

  // ai_chat::mojom::PageHandler:
  void SetClientPage(
      mojo::PendingRemote<ai_chat::mojom::ChatUIPage> page) override;
  void SubmitHumanConversationEntry(const std::string& input) override;
  void GetConversationHistory(GetConversationHistoryCallback callback) override;
  void RequestSummary() override;
  void MarkAgreementAccepted() override;

 private:
  // ChatTabHelper::Observer
  void OnHistoryUpdate() override;
  void OnAPIRequestInProgress(bool in_progress) override;
  void OnRequestSummaryFailed() override;

  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  mojo::Remote<ai_chat::mojom::ChatUIPage> page_;

  raw_ptr<AIChatTabHelper> active_chat_tab_helper_ = nullptr;

  base::ScopedObservation<AIChatTabHelper, AIChatTabHelper::Observer>
      chat_tab_helper_observation_{this};

  raw_ptr<Profile> profile_ = nullptr;
  mojo::Receiver<ai_chat::mojom::PageHandler> receiver_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
