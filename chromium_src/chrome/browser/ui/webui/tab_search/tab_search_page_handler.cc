/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#include "base/check.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/grit/brave_components_strings.h"
#include "components/sessions/core/session_id.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/origin.h"

#define TabSearchPageHandler TabSearchPageHandler_ChromiumImpl
#include "src/chrome/browser/ui/webui/tab_search/tab_search_page_handler.cc"
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
  pref_change_registrar_.Add(
      ai_chat::prefs::kBraveAIChatTabOrganizationEnabled,
      base::BindRepeating(
          &TabSearchPageHandler::OnTabOrganizationFeaturePrefChanged,
          base::Unretained(this), Profile::FromWebUI(web_ui_)));
}

void TabSearchPageHandler::OnTabOrganizationFeaturePrefChanged(
    Profile* profile) {
  page_->TabOrganizationEnabledChanged(
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile) &&
      ai_chat::features::IsTabOrganizationEnabled() &&
      profile->GetPrefs()->GetBoolean(
          ai_chat::prefs::kBraveAIChatTabOrganizationEnabled));
  page_->ShowFREChanged(!profile->GetPrefs()->HasPrefPath(
      ai_chat::prefs::kBraveAIChatTabOrganizationEnabled));
}

TabSearchPageHandler::~TabSearchPageHandler() = default;

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
  std::vector<TabDetails> tab_details_before_move;
  for (const auto& tab_id_str : *result) {
    int tab_id = 0;
    if (!base::StringToInt(tab_id_str, &tab_id)) {
      continue;
    }

    std::optional<TabDetails> details = GetTabDetails(tab_id);
    if (!details) {
      continue;
    }

    // Store all details before move to preserve the index.
    tab_details_before_move.push_back(*details);
    // Store old window ID (session ID), tab ID, and tab strip index for
    // undo.
    original_tabs_info_by_window_[details->browser->session_id()].push_back(
        {tab_id, details->GetIndex()});
  }

  if (tab_details_before_move.empty()) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  auto create_params = Browser::CreateParams(Profile::FromWebUI(web_ui_), true);
  create_params.user_title = topic;
  Browser* new_browser = Browser::Create(create_params);
  for (const auto& details : tab_details_before_move) {
    std::unique_ptr<tabs::TabModel> tab =
        details.browser->tab_strip_model()->DetachTabAtForInsertion(
            details.GetIndex());
    new_browser->tab_strip_model()->AppendTab(std::move(tab),
                                              false /* foreground */);
  }
  new_browser->window()->Show();

  std::move(callback).Run(true, nullptr);
}

void TabSearchPageHandler::UndoFocusTabs(UndoFocusTabsCallback callback) {
  for (auto& iter : original_tabs_info_by_window_) {
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
    std::ranges::sort(iter.second, [](const TabInfo& a, const TabInfo& b) {
      return a.index < b.index;
    });

    for (const auto& tab_info : iter.second) {
      // Find the moved tab.
      std::optional<TabDetails> details = GetTabDetails(tab_info.tab_id);
      if (!details) {
        continue;
      }

      std::unique_ptr<tabs::TabModel> tab =
          details->browser->tab_strip_model()->DetachTabAtForInsertion(
              details->GetIndex());
      target->tab_strip_model()->InsertDetachedTabAt(
          tab_info.index, std::move(tab), AddTabTypes::ADD_NONE);
    }
  }

  original_tabs_info_by_window_.clear();
  std::move(callback).Run();
}

void TabSearchPageHandler::OpenLeoGoPremiumPage() {
  NavigateParams params(Profile::FromWebUI(web_ui_),
                        GURL(ai_chat::kLeoGoPremiumUrl),
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
