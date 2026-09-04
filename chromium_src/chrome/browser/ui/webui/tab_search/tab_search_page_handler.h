/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/history_embeddings/content/open_tab_passages.h"
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

namespace history_embeddings {
class HistoryEmbeddingsSearch;
}

class SessionID;

class TabSearchPageHandler;
using TabSearchPageHandler_BraveImpl = TabSearchPageHandler;

#define TabSearchPageHandler TabSearchPageHandler_ChromiumImpl
// Inject friend declaration via CreateProfileData (private method) so our
// Brave subclass can access private members like page_, web_ui_, etc.
#define CreateProfileData                \
  NotUsed();                             \
  friend TabSearchPageHandler_BraveImpl; \
  tab_search::mojom::ProfileDataPtr CreateProfileData

#include <chrome/browser/ui/webui/tab_search/tab_search_page_handler.h>  // IWYU pragma: export

#undef TabSearchPageHandler
#undef CreateProfileData

#if BUILDFLAG(ENABLE_AI_CHAT)
namespace ai_chat {
struct Tab;
namespace mojom {
enum class APIError;
}  // namespace mojom
}  // namespace ai_chat
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

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
  void OpenLearnMorePage() override;

  void SetTabFocusEnabled() override;
  void GetTabFocusShowFRE(GetTabFocusShowFRECallback callback) override;
  void SearchTabsByContent(const std::string& query,
                           SearchTabsByContentCallback callback) override;

#if BUILDFLAG(ENABLE_AI_CHAT)
  void SetOriginalTabsInfoByWindowForTesting(
      const base::flat_map<SessionID, std::vector<TabInfo>>&
          original_tabs_info_by_window) {
    original_tabs_info_by_window_ = original_tabs_info_by_window;
  }
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_LOCAL_AI)
  // Overrides the `HistoryEmbeddingsSearch` used by `SearchTabsByContent` so
  // tests can avoid standing up a real `HistoryEmbeddingsService`.
  // Pass nullopt to drop the override and go back to the profile's service.
  void SetEmbeddingsSearchForTesting(
      std::optional<base::WeakPtr<history_embeddings::HistoryEmbeddingsSearch>>
          search) {
    embeddings_search_for_testing_ = std::move(search);
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  // Stands in for the passage read in `GetTabsForAIEngine`. The read needs a
  // real `HistoryEmbeddingsService`, which the factory only builds behind
  // signin and opt-in gating that a browser test can't satisfy.
  using TabPassagesFetcher =
      base::RepeatingCallback<void(const std::vector<GURL>&,
                                   history_embeddings::TabPassagesCallback)>;
  void SetTabPassagesFetcherForTesting(TabPassagesFetcher fetcher) {
    tab_passages_fetcher_for_testing_ = std::move(fetcher);
  }
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

 private:
  void OpenURLInNewTab(const GURL& url);

#if BUILDFLAG(ENABLE_AI_CHAT)
  void OnGetFocusTabs(const std::string& topic,
                      GetFocusTabsCallback callback,
                      base::expected<std::vector<std::string>,
                                     ai_chat::mojom::APIError> result);
  void OnGetSuggestedTopics(GetSuggestedTopicsCallback callback,
                            base::expected<std::vector<std::string>,
                                           ai_chat::mojom::APIError> result);
  void OnTabOrganizationFeaturePrefChanged(Profile* profile);
  tab_search::mojom::ErrorPtr GetError(ai_chat::mojom::APIError error);

  using TabsForAIEngineCallback =
      base::OnceCallback<void(std::vector<ai_chat::Tab>)>;
  // Collects the tabs to describe to the AI engine. Asynchronous because
  // populating each tab's page excerpts means a trip through HistoryService
  // and the history embeddings database.
  void GetTabsForAIEngine(TabsForAIEngineCallback callback);
  void OnTabsReadyForSuggestedTopics(GetSuggestedTopicsCallback callback,
                                     std::vector<ai_chat::Tab> tabs);
  void OnTabsReadyForFocusTabs(const std::string& topic,
                               GetFocusTabsCallback callback,
                               std::vector<ai_chat::Tab> tabs);

  PrefChangeRegistrar brave_pref_change_registrar_;

  // Map from window ID (session ID serves as a unique window ID here as this is
  // only used within a single session) to the list of original tab info for
  // undo last focus tabs action. This is used to move the focus tabs back to
  // their original positions.
  base::flat_map<SessionID, std::vector<TabInfo>> original_tabs_info_by_window_;
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_LOCAL_AI)
 private:
#if BUILDFLAG(ENABLE_AI_CHAT)
  void OnTabPassagesReady(
      std::vector<ai_chat::Tab> tabs,
      TabsForAIEngineCallback callback,
      std::vector<std::vector<std::string>> passages_by_tab);

  // Whether page excerpts may be attached to this profile's tab list.
  // Checked before the passage reads start and again before their results
  // are used, since the reads are asynchronous.
  bool MaySendPageContent(Profile* profile);
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

  // Tracks per-tab HistoryService::QueryURL calls issued by
  // SearchTabsByContent and the passage collection in GetTabsForAIEngine.
  // Destruction cancels any in-flight resolution so a stale search can't fire
  // its embeddings query after the handler is gone.
  base::CancelableTaskTracker query_url_task_tracker_;

  // Set means a test installed an override; the pointer inside may still have
  // been invalidated.
  std::optional<base::WeakPtr<history_embeddings::HistoryEmbeddingsSearch>>
      embeddings_search_for_testing_;

#if BUILDFLAG(ENABLE_AI_CHAT)
  TabPassagesFetcher tab_passages_fetcher_for_testing_;
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

#if BUILDFLAG(ENABLE_AI_CHAT)
 private:
  base::WeakPtrFactory<TabSearchPageHandler> weak_ptr_factory_{this};
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_PAGE_HANDLER_H_
