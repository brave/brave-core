// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/ai_chat/tab_data_web_state_observer.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/ios/browser/api/ai_chat/tab_tracker_service_factory.h"
#include "components/tabs/public/tab_interface.h"
#include "ios/chrome/browser/shared/model/browser/browser.h"
#include "ios/chrome/browser/shared/model/browser/browser_list.h"
#include "ios/chrome/browser/shared/model/browser/browser_list_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#include "ios/web/public/favicon/favicon_url.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"

namespace ai_chat {

namespace {

mojom::TabDataPtr CreateTabDataFromWebState(web::WebState* web_state) {
  // If the WebState is in a WebStateList, it's Brave's fake `NativeWebState`
  // used for syncing. So ignore it.
  BrowserList* browser_list = BrowserListFactory::GetForProfile(
      ProfileIOS::FromBrowserState(web_state->GetBrowserState()));
  if (browser_list) {
    for (Browser* browser :
         browser_list->BrowsersOfType(BrowserList::BrowserType::kAll)) {
      WebStateList* web_state_list = browser->GetWebStateList();
      if (web_state_list && web_state_list->GetIndexOfWebState(web_state) !=
                                WebStateList::kInvalidIndex) {
        return nullptr;
      }
    }
  }

  if (auto* last_committed_item =
          web_state->GetNavigationManager()->GetLastCommittedItem();
      last_committed_item != nullptr) {
    auto tab = mojom::TabData::New();
    tab->content_id = last_committed_item->GetUniqueID();
    tab->title = base::UTF16ToUTF8(web_state->GetTitle());
    tab->url = web_state->GetLastCommittedURL();
    return tab;
  }

  return nullptr;
}

}  // namespace

TabDataWebStateObserver::TabDataWebStateObserver(web::WebState* web_state)
    : tab_handle_(web_state->GetUniqueIdentifier().identifier()),
      web_state_(web_state),
      service_(*TabTrackerServiceFactory::GetForProfile(
          ProfileIOS::FromBrowserState(web_state->GetBrowserState()))) {
  web_state_->AddObserver(this);
  GetWebStateList()[tab_handle_] = {web_state_, false};
}

TabDataWebStateObserver::~TabDataWebStateObserver() {
  web_state_->RemoveObserver(this);
  service_->UpdateTab(tab_handle_, nullptr);
  GetWebStateList().erase(tab_handle_);
}

void TabDataWebStateObserver::WasShown(web::WebState* web_state) {
  SetActiveTab(web_state, true);
  UpdateTab();
}

void TabDataWebStateObserver::WasHidden(web::WebState* web_state) {
  SetActiveTab(web_state, false);
  UpdateTab();
}

void TabDataWebStateObserver::DidStartNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  UpdateTab();
}

void TabDataWebStateObserver::DidRedirectNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  UpdateTab();
}

void TabDataWebStateObserver::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  UpdateTab();
}

void TabDataWebStateObserver::DidStartLoading(web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::DidStopLoading(web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::PageLoaded(
    web::WebState* web_state,
    web::PageLoadCompletionStatus load_completion_status) {
  UpdateTab();
}

void TabDataWebStateObserver::DidChangeBackForwardState(
    web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::TitleWasSet(web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::DidChangeVisibleSecurityState(
    web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::FaviconUrlUpdated(
    web::WebState* web_state,
    const std::vector<web::FaviconURL>& candidates) {
  UpdateTab();
}

void TabDataWebStateObserver::PermissionStateChanged(
    web::WebState* web_state,
    web::Permission permission) {
  UpdateTab();
}

void TabDataWebStateObserver::RenderProcessGone(web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::WebStateRealized(web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::WebStateDestroyed(web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::UpdateTab() {
  auto tab = CreateTabDataFromWebState(web_state_.get());
  if (tab) {
    tab->id = tab_handle_;
    service_->UpdateTab(tab_handle_, std::move(tab));
  }
}

web::WebState* TabDataWebStateObserver::GetActiveTab() {
  for (auto& pair : GetWebStateList()) {
    if (pair.second.is_active) {
      return pair.second.web_state;
    }
  }
  return nullptr;
}

void TabDataWebStateObserver::SetActiveTab(web::WebState* web_state,
                                           bool active) {
  for (auto& pair : GetWebStateList()) {
    if (pair.first == web_state->GetUniqueIdentifier().identifier()) {
      pair.second.is_active = active;
    } else {
      pair.second.is_active = false;
    }
  }
}

web::WebState* TabDataWebStateObserver::GetTabById(std::int32_t tab_id) {
  for (auto& pair : GetWebStateList()) {
    if (pair.first == tab_id) {
      return pair.second.web_state;
    }
  }
  return nullptr;
}

base::flat_map<std::int32_t, TabDataWebStateObserver::TabInfo>&
TabDataWebStateObserver::GetWebStateList() {
  static base::NoDestructor<base::flat_map<std::int32_t, TabInfo>> map;
  return *map;
}

}  // namespace ai_chat
