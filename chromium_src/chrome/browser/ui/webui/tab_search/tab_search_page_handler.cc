/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/sessions/core/session_id.h"

#define TabSearchPageHandler TabSearchPageHandler_ChromiumImpl
#include "src/chrome/browser/ui/webui/tab_search/tab_search_page_handler.cc"
#undef TabSearchPageHandler

TabSearchPageHandler::TabSearchPageHandler(
    mojo::PendingReceiver<tab_search::mojom::PageHandler> receiver,
    mojo::PendingRemote<tab_search::mojom::Page> page,
    content::WebUI* web_ui,
    TopChromeWebUIController* webui_controller,
    MetricsReporter* metrics_reporter)
    : TabSearchPageHandler_ChromiumImpl(std::move(receiver), std::move(page), web_ui, webui_controller, metrics_reporter) {}

TabSearchPageHandler::~TabSearchPageHandler() = default;

ai_chat::EngineConsumer* TabSearchPageHandler::MaybeGetAIEngineForTabFocus() {
  if (!ai_chat_engine_) {
    Profile* profile = Profile::FromWebUI(web_ui_);
    ai_chat::AIChatService* ai_chat_service = ai_chat::AIChatServiceFactory::GetForBrowserContext(profile);
    if (!ai_chat_service) {
      return nullptr;  // Unsupported context.
    }
    ai_chat_engine_ = ai_chat_service->GetEngineForModel(ai_chat::kClaudeHaikuModelKey);
    CHECK(ai_chat_engine_);
  }

  return ai_chat_engine_.get();
}

std::vector<ai_chat::Tab> TabSearchPageHandler::GetTabsForAIEngine() {
  std::vector<ai_chat::Tab> tabs;
  auto profile_data = CreateProfileData();
  for (const auto& window : profile_data->windows) {
    for (const auto& tab : window->tabs) {
      tabs.push_back(ai_chat::Tab(base::NumberToString(tab->tab_id), tab->title, url::Origin::Create(tab->url).Serialize()));
    }
  }

  return tabs;
}

void TabSearchPageHandler::GetSuggestedTopics(GetSuggestedTopicsCallback callback) {
  auto* engine = MaybeGetAIEngineForTabFocus();
  if (!engine) {
    std::move(callback).Run({});
    return;
  }

  std::vector<ai_chat::Tab> tabs = GetTabsForAIEngine();
  engine->GetSuggestedTopics(tabs, base::BindOnce(&TabSearchPageHandler::OnGetSuggestedTopics, weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void TabSearchPageHandler::OnGetSuggestedTopics(
    GetSuggestedTopicsCallback callback,
    const std::vector<std::string>& topics) {
  std::move(callback).Run(topics);
}

void TabSearchPageHandler::GetFocusTabs(const std::string& topic, GetFocusTabsCallback callback) {
  auto* engine = MaybeGetAIEngineForTabFocus();
  if (!engine) {
    std::move(callback).Run(false);
    return;
  }

  focus_tabs_info_.clear();

  std::vector<ai_chat::Tab> tabs = GetTabsForAIEngine();
  ai_chat_engine_->GetFocusTabs(tabs, topic,
      base::BindOnce(&TabSearchPageHandler::OnGetFocusTabs, weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void TabSearchPageHandler::OnGetFocusTabs(
    GetFocusTabsCallback callback,
    const std::vector<std::string>& tab_ids) {
  // Move all tabs from normal browser window to a new window.
  std::vector<TabDetails> tab_details_before_move;
  for (const auto& tab_id_str : tab_ids) {
    int tab_id = 0;
    if (!base::StringToInt(tab_id_str, &tab_id)) {
      continue;
    }

    std::optional<TabDetails> details = GetTabDetails(tab_id);
    if (!details || !details->browser->is_type_normal()) {
      continue;
    }

    // Store all details before move to preserve the index.
    tab_details_before_move.push_back(*details);
    // Store old window ID (session ID), tab ID, and tab strip index for
    // undo.
    focus_tabs_info_[details->browser->session_id()].push_back({tab_id, details->GetIndex()});
  }

  if (tab_details_before_move.empty()) {
    std::move(callback).Run(false);
    return;
  }

  Browser* new_browser = Browser::Create(Browser::CreateParams(Profile::FromWebUI(web_ui_), true));
  for (const auto& details : tab_details_before_move) {
    std::unique_ptr<tabs::TabModel> tab =
        details.browser->tab_strip_model()->DetachTabAtForInsertion(details.GetIndex());
    new_browser->tab_strip_model()->AppendTab(std::move(tab), false /* foreground */);
  }
  new_browser->window()->Show();

  std::move(callback).Run(true);
}

void TabSearchPageHandler::UndoFocusTabs(UndoFocusTabsCallback callback) {
  for (auto& iter : focus_tabs_info_) {
    // Find the browser with the session ID (key).
    Browser* target = nullptr;
    for (Browser* browser : *BrowserList::GetInstance()) {
      if (!ShouldTrackBrowser(browser)) {
        continue;
      }

      if (browser->session_id() == iter.first) {
        target = browser;
        break;
      }
    }

    if (!target) {
      continue;
    }


    // Sort the tabs info by index in ascending order to ensure tabs are
    // inserted in the correct order.
    base::ranges::sort(iter.second, [](const TabInfo& a, const TabInfo& b) {
      return a.index < b.index;
    });

    for (const auto& tab_info : iter.second) {
      // Find the moved tab.
      std::optional<TabDetails> details = GetTabDetails(tab_info.tab_id);
      if (!details) {
        continue;
      }

      std::unique_ptr<tabs::TabModel> tab =
          details->browser->tab_strip_model()->DetachTabAtForInsertion(details->GetIndex());
      target->tab_strip_model()->InsertDetachedTabAt(tab_info.index, std::move(tab), AddTabTypes::ADD_NONE);
    }
  }

  std::move(callback).Run();
}
