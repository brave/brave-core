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
namespace mojom {
enum class APIError;
}  // namespace mojom
}  // namespace ai_chat

// Overrides TabSearchPageHandler to provide Brave-specific functionality.
// See tab_search.mojom in chromium_src for our extended interface. Currently
// it provides APIs needed for our tab organization feature using Leo.
class TabSearchPageHandler : public TabSearchPageHandler_ChromiumImpl {
 public:
  // Used to store info for undo focus tabs.
  struct TabInfo {
    int tab_id;
    int index;
  };

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
  void OpenLeoGoPremiumPage() override;

  void SetAIChatEngineForTesting(
      std::unique_ptr<ai_chat::EngineConsumer> ai_chat_engine) {
    ai_chat_engine_ = std::move(ai_chat_engine);
  }

  ai_chat::EngineConsumer* GetAIChatEngineForTesting() {
    return ai_chat_engine_.get();
  }

  void SetFocusTabsInfoForTesting(
      const base::flat_map<SessionID, std::vector<TabInfo>>& focus_tabs_info) {
    focus_tabs_info_ = focus_tabs_info;
  }

 private:
  void OnGetFocusTabs(GetFocusTabsCallback callback,
                      base::expected<std::vector<std::string>,
                                     ai_chat::mojom::APIError> result);
  void OnGetSuggestedTopics(GetSuggestedTopicsCallback callback,
                            base::expected<std::vector<std::string>,
                                           ai_chat::mojom::APIError> result);
  tab_search::mojom::ErrorPtr GetError(ai_chat::mojom::APIError error);

  ai_chat::EngineConsumer* GetAIEngineForTabFocus();
  std::vector<ai_chat::Tab> GetTabsForAIEngine();

  // Map from session id to the list of tab info for undo last get focus tabs
  // action (i.e. move tabs from the new window back to their original
  // positions).
  base::flat_map<SessionID, std::vector<TabInfo>> focus_tabs_info_;

  // The AI chat engine to interact with Leo server.
  std::unique_ptr<ai_chat::EngineConsumer> ai_chat_engine_;

  base::WeakPtrFactory<TabSearchPageHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_
