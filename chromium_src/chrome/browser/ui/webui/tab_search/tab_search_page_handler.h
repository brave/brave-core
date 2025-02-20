/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

class SessionID;

class TabSearchPageHandler;
using TabSearchPageHandler_BraveImpl = TabSearchPageHandler;

#define TabSearchPageHandler TabSearchPageHandler_ChromiumImpl
#define MaybeShowUI                      \
  NotUsed();                             \
  friend TabSearchPageHandler_BraveImpl; \
  void MaybeShowUI

#include "src/chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#undef TabSearchPageHandler
#undef MaybeShowUI

namespace ai_chat {
struct Tab;
}  // namespace ai_chat

class TabSearchPageHandler : public TabSearchPageHandler_ChromiumImpl {
 public:
  TabSearchPageHandler(
      mojo::PendingReceiver<tab_search::mojom::PageHandler> receiver,
      mojo::PendingRemote<tab_search::mojom::Page> page,
      content::WebUI* web_ui,
      TopChromeWebUIController* webui_controller,
      MetricsReporter* metrics_reporter);
  ~TabSearchPageHandler() override;
  TabSearchPageHandler(const TabSearchPageHandler&) = delete;
  TabSearchPageHandler& operator=(const TabSearchPageHandler&) = delete;

  // tab_search::mojom::PageHandler:
  void GetSuggestedTopics(GetSuggestedTopicsCallback callback) override;
  void GetFocusTabs(const std::string& topic,
                    GetFocusTabsCallback callback) override;
  void UndoFocusTabs(UndoFocusTabsCallback callback) override;

  void SetAIChatEngineForTesting(
      std::unique_ptr<ai_chat::EngineConsumer> ai_chat_engine) {
    ai_chat_engine_ = std::move(ai_chat_engine);
  }

  ai_chat::EngineConsumer* GetAIChatEngineForTesting() {
    return ai_chat_engine_.get();
  }

 private:
  void OnGetFocusTabs(GetFocusTabsCallback callback,
                      const std::vector<std::string>& tab_ids);
  void OnGetSuggestedTopics(GetSuggestedTopicsCallback callback,
                            const std::vector<std::string>& topics);

  ai_chat::EngineConsumer* MaybeGetAIEngineForTabFocus();
  std::vector<ai_chat::Tab> GetTabsForAIEngine();

  struct TabInfo {
    int tab_id;
    int index;
  };

  base::flat_map<SessionID, std::vector<TabInfo>> focus_tabs_info_;

  std::unique_ptr<ai_chat::EngineConsumer> ai_chat_engine_;

  base::WeakPtrFactory<TabSearchPageHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_
