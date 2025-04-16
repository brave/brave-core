/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

class SessionID;

class TabSearchPageHandler;
using TabSearchPageHandler_BraveImpl = TabSearchPageHandler;

#define TabSearchPageHandler TabSearchPageHandler_ChromiumImpl
#define MaybeShowUI                      \
  NotUsed();                             \
  friend TabSearchPageHandler_BraveImpl; \
  void MaybeShowUI

#include "src/chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"  // IWYU pragma: export

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
  void SetTabFocusEnabled() override;
  void GetTabFocusShowFRE(GetTabFocusShowFRECallback callback) override;

  void SetOriginalTabsInfoByWindowForTesting(
      const base::flat_map<SessionID, std::vector<TabInfo>>&
          original_tabs_info_by_window) {
    original_tabs_info_by_window_ = original_tabs_info_by_window;
  }

 private:
  void OnGetFocusTabs(const std::string& topic,
                      GetFocusTabsCallback callback,
                      base::expected<std::vector<std::string>,
                                     ai_chat::mojom::APIError> result);
  void OnGetSuggestedTopics(GetSuggestedTopicsCallback callback,
                            base::expected<std::vector<std::string>,
                                           ai_chat::mojom::APIError> result);
  void OnTabOrganizationFeaturePrefChanged(Profile* profile);
  tab_search::mojom::ErrorPtr GetError(ai_chat::mojom::APIError error);

  std::vector<ai_chat::Tab> GetTabsForAIEngine();

  // Map from window ID (session ID serves as a unique window ID here as this is
  // only used within a single session) to the list of original tab info for
  // undo last focus tabs action. This is used to move the focus tabs back to
  // their original positions.
  base::flat_map<SessionID, std::vector<TabInfo>> original_tabs_info_by_window_;

  base::WeakPtrFactory<TabSearchPageHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_
