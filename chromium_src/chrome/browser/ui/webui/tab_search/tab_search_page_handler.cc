/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#include <memory>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/grit/brave_components_strings.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/history_embeddings_search.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/sessions/core/session_id.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/browser/history_embeddings/open_tab_search.h"
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

#define TabSearchPageHandler TabSearchPageHandler_ChromiumImpl
#include <chrome/browser/ui/webui/tab_search/tab_search_page_handler.cc>
#undef TabSearchPageHandler

TabSearchPageHandler::TabSearchPageHandler(
    mojo::PendingReceiver<tab_search::mojom::PageHandler> receiver,
    mojo::PendingRemote<tab_search::mojom::Page> page,
    content::WebUI* web_ui,
    TopChromeWebUIController* webui_controller,
    MetricsReporter* metrics_reporter)
    : TabSearchPageHandler_ChromiumImpl(std::move(receiver),
                                        std::move(page),
                                        web_ui,
                                        webui_controller,
                                        metrics_reporter) {
#if BUILDFLAG(ENABLE_AI_CHAT)
  Profile* profile = Profile::FromWebUI(web_ui_);
  brave_pref_change_registrar_.Init(profile->GetPrefs());
  brave_pref_change_registrar_.Add(
      ai_chat::prefs::kBraveAIChatTabOrganizationEnabled,
      base::BindRepeating(
          &TabSearchPageHandler::OnTabOrganizationFeaturePrefChanged,
          base::Unretained(this), profile));
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
}

TabSearchPageHandler::~TabSearchPageHandler() = default;

#if BUILDFLAG(ENABLE_AI_CHAT)
void TabSearchPageHandler::OnTabOrganizationFeaturePrefChanged(
    Profile* profile) {
  page_->TabFocusEnabledChanged(
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile) &&
      ai_chat::features::IsTabOrganizationEnabled() &&
      profile->GetPrefs()->GetBoolean(
          ai_chat::prefs::kBraveAIChatTabOrganizationEnabled));
  page_->TabFocusShowFREChanged(!profile->GetPrefs()->HasPrefPath(
      ai_chat::prefs::kBraveAIChatTabOrganizationEnabled));
}

std::vector<ai_chat::Tab> TabSearchPageHandler::GetTabsForAIEngine() {
  std::vector<ai_chat::Tab> tabs;
  auto profile_data = CreateProfileData();
  for (const auto& window : profile_data->windows) {
    for (const auto& tab : window->tabs) {
      if (!tab->url.SchemeIsHTTPOrHTTPS()) {
        continue;
      }

      // It should be safe to use url::Origin::Create() here as we are only
      // handling HTTP/HTTPS tab URLs.
      tabs.push_back(ai_chat::Tab(base::NumberToString(tab->tab_id), tab->title,
                                  url::Origin::Create(tab->url)));
    }
  }

  return tabs;
}

tab_search::mojom::ErrorPtr TabSearchPageHandler::GetError(
    ai_chat::mojom::APIError api_error) {
  Profile* profile = Profile::FromWebUI(web_ui_);
  ai_chat::AIChatService* ai_chat_service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile);
  CHECK(ai_chat_service);

  tab_search::mojom::ErrorPtr error = tab_search::mojom::Error::New();
  if (api_error == ai_chat::mojom::APIError::RateLimitReached) {
    bool is_premium = ai_chat_service->IsPremiumStatus();
    error->message =
        is_premium
            ? l10n_util::GetStringUTF8(IDS_CHAT_UI_ERROR_RATE_LIMIT)
            : l10n_util::GetStringUTF8(IDS_CHAT_UI_RATE_LIMIT_REACHED_DESC);
    error->rate_limited_info =
        tab_search::mojom::RateLimitedInfo::New(is_premium);
  } else if (api_error == ai_chat::mojom::APIError::ConnectionIssue) {
    error->message = l10n_util::GetStringUTF8(IDS_CHAT_UI_ERROR_NETWORK);
  } else {
    error->message = l10n_util::GetStringUTF8(IDS_CHAT_UI_ERROR_INTERNAL);
  }

  return error;
}

void TabSearchPageHandler::GetSuggestedTopics(
    GetSuggestedTopicsCallback callback) {
  std::vector<ai_chat::Tab> tabs = GetTabsForAIEngine();
  ai_chat::AIChatService* ai_chat_service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(
          Profile::FromWebUI(web_ui_));
  // Must be available as related UI is only shown if the service is
  // available.
  CHECK(ai_chat_service);
  ai_chat_service->GetSuggestedTopics(
      tabs,
      base::BindOnce(&TabSearchPageHandler::OnGetSuggestedTopics,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void TabSearchPageHandler::OnGetSuggestedTopics(
    GetSuggestedTopicsCallback callback,
    base::expected<std::vector<std::string>, ai_chat::mojom::APIError> result) {
  if (!result.has_value()) {
    std::move(callback).Run({}, GetError(result.error()));
    return;
  }

  std::move(callback).Run(*result, nullptr);
}

void TabSearchPageHandler::GetFocusTabs(const std::string& topic,
                                        GetFocusTabsCallback callback) {
  original_tabs_info_by_window_.clear();

  ai_chat::AIChatService* ai_chat_service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(
          Profile::FromWebUI(web_ui_));
  // Must be available as related UI is only shown if the service is
  // available.
  CHECK(ai_chat_service);
  std::vector<ai_chat::Tab> tabs = GetTabsForAIEngine();
  ai_chat_service->GetFocusTabs(
      tabs, topic,
      base::BindOnce(&TabSearchPageHandler::OnGetFocusTabs,
                     weak_ptr_factory_.GetWeakPtr(), topic,
                     std::move(callback)));
}

void TabSearchPageHandler::OnGetFocusTabs(
    const std::string& topic,
    GetFocusTabsCallback callback,
    base::expected<std::vector<std::string>, ai_chat::mojom::APIError> result) {
  if (!result.has_value()) {
    std::move(callback).Run(false, GetError(result.error()));
    return;
  }

  // Move all tabs from normal browser window to a new window.
  std::vector<tabs::TabInterface*> tabs_before_move;
  for (const auto& tab_id_str : *result) {
    int tab_id = 0;
    if (!base::StringToInt(tab_id_str, &tab_id)) {
      continue;
    }

    tabs::TabInterface* tab = GetTabInterface(tab_id);
    if (!tab) {
      continue;
    }

    // Store all tab before move to preserve the index.
    tabs_before_move.push_back(tab);
    // Store old window ID (session ID), tab ID, and tab strip index for
    // undo.
    int tab_index =
        tab->GetBrowserWindowInterface()->GetTabStripModel()->GetIndexOfTab(
            tab);
    original_tabs_info_by_window_[tab->GetBrowserWindowInterface()
                                      ->GetSessionID()]
        .push_back({tab_id, tab_index});
  }

  if (tabs_before_move.empty()) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  auto create_params = Browser::CreateParams(Profile::FromWebUI(web_ui_), true);
  create_params.user_title = topic;
  Browser* new_browser = Browser::Create(create_params);
  for (auto* tab : tabs_before_move) {
    int tab_index =
        tab->GetBrowserWindowInterface()->GetTabStripModel()->GetIndexOfTab(
            tab);

    std::unique_ptr<tabs::TabModel> detached_tab_model =
        tab->GetBrowserWindowInterface()
            ->GetTabStripModel()
            ->DetachTabAtForInsertion(tab_index);
    new_browser->tab_strip_model()->AppendTab(std::move(detached_tab_model),
                                              false /* foreground */);
  }
  BrowserWindow::FromBrowser(new_browser)->Show();

  std::move(callback).Run(true, nullptr);
}

void TabSearchPageHandler::UndoFocusTabs(UndoFocusTabsCallback callback) {
  for (auto& iter : original_tabs_info_by_window_) {
    // Find the browser with the session ID (key).
    Browser* target = nullptr;
    GlobalBrowserCollection::GetInstance()->ForEach(
        [&target, &iter, this](BrowserWindowInterface* bwi) {
          Browser* browser = bwi->GetBrowserForMigrationOnly();
          if (!ShouldTrackBrowser(profile_, browser)) {
            return true;
          }

          if (browser->session_id() == iter.first) {
            target = browser;
          }
          return target == nullptr;
        });

    if (!target) {
      continue;
    }

    // Sort the tabs info by index in ascending order to ensure tabs are
    // inserted in the correct order.
    std::ranges::sort(iter.second, [](const TabInfo& a, const TabInfo& b) {
      return a.index < b.index;
    });

    for (const auto& tab_info : iter.second) {
      // Find the moved tab.
      tabs::TabInterface* tab = GetTabInterface(tab_info.tab_id);
      if (!tab) {
        continue;
      }

      int tab_index =
          tab->GetBrowserWindowInterface()->GetTabStripModel()->GetIndexOfTab(
              tab);

      std::unique_ptr<tabs::TabModel> detached_tab_model =
          tab->GetBrowserWindowInterface()
              ->GetTabStripModel()
              ->DetachTabAtForInsertion(tab_index);
      target->tab_strip_model()->InsertDetachedTabAt(
          tab_info.index, std::move(detached_tab_model), AddTabTypes::ADD_NONE);
    }
  }

  original_tabs_info_by_window_.clear();
  std::move(callback).Run();
}

void TabSearchPageHandler::OpenLeoGoPremiumPage() {
  OpenURLInNewTab(GURL(ai_chat::kLeoGoPremiumUrl));
}

void TabSearchPageHandler::OpenLearnMorePage() {
  OpenURLInNewTab(GURL(ai_chat::kTabOrganizationLearnMoreUrl));
}

void TabSearchPageHandler::OpenURLInNewTab(const GURL& url) {
  NavigateParams params(Profile::FromWebUI(web_ui_), url,
                        ui::PageTransition::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  Navigate(&params);
}

void TabSearchPageHandler::SetTabFocusEnabled() {
  Profile::FromWebUI(web_ui_)->GetPrefs()->SetBoolean(
      ai_chat::prefs::kBraveAIChatTabOrganizationEnabled, true);
}

void TabSearchPageHandler::GetTabFocusShowFRE(
    GetTabFocusShowFRECallback callback) {
  std::move(callback).Run(!Profile::FromWebUI(web_ui_)->GetPrefs()->HasPrefPath(
      ai_chat::prefs::kBraveAIChatTabOrganizationEnabled));
}
#else   // !BUILDFLAG(ENABLE_AI_CHAT)
// Stub implementations when AI Chat is disabled
void TabSearchPageHandler::GetSuggestedTopics(
    GetSuggestedTopicsCallback callback) {
  std::move(callback).Run({}, nullptr);
}

void TabSearchPageHandler::GetFocusTabs(const std::string& topic,
                                        GetFocusTabsCallback callback) {
  std::move(callback).Run(false, nullptr);
}

void TabSearchPageHandler::UndoFocusTabs(UndoFocusTabsCallback callback) {
  std::move(callback).Run();
}

void TabSearchPageHandler::OpenLeoGoPremiumPage() {}

void TabSearchPageHandler::OpenLearnMorePage() {}

void TabSearchPageHandler::SetTabFocusEnabled() {}

void TabSearchPageHandler::GetTabFocusShowFRE(
    GetTabFocusShowFRECallback callback) {
  std::move(callback).Run(false);
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_LOCAL_AI)
void TabSearchPageHandler::SearchTabsByContent(
    const std::string& query,
    SearchTabsByContentCallback callback) {
  Profile* profile = Profile::FromWebUI(web_ui_);
  // The history-embeddings setting is off for this profile, so there's
  // no on-device ranker to consult.
  if (!history_embeddings::IsHistoryEmbeddingsEnabledForProfile(profile)) {
    std::move(callback).Run({});
    return;
  }
  history_embeddings::HistoryEmbeddingsSearch* embeddings_search =
      embeddings_search_for_testing_
          ? embeddings_search_for_testing_.get()
          : HistoryEmbeddingsServiceFactory::GetForProfile(profile);
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  // Empty query, or one of the keyed services we depend on is unavailable
  // for this profile (e.g. incognito).
  if (query.empty() || !embeddings_search || !history_service) {
    std::move(callback).Run({});
    return;
  }

  history_embeddings::SearchOpenTabsByContent(
      profile, history_service, embeddings_search, query, std::move(callback),
      &query_url_task_tracker_);
}
#else   // !BUILDFLAG(ENABLE_LOCAL_AI)
void TabSearchPageHandler::SearchTabsByContent(
    const std::string& query,
    SearchTabsByContentCallback callback) {
  std::move(callback).Run({});
}
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)
