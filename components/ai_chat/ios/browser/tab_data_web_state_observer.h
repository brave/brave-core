// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_TAB_DATA_WEB_STATE_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_TAB_DATA_WEB_STATE_OBSERVER_H_

#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "ios/web/public/permissions/permissions.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"

namespace web {
class NavigationContext;
class WebState;
}  // namespace web

namespace ai_chat {

class TabTrackerService;

// This class informs the TabTrackerService about changes to tabs (i.e.
// creation, deletion, title/url updates). Each instance of this class is
// associated with a single tab.
class TabDataWebStateObserver
    : public web::WebStateObserver,
      public web::WebStateUserData<TabDataWebStateObserver> {
 public:
  ~TabDataWebStateObserver() override;

  TabDataWebStateObserver(const TabDataWebStateObserver&) = delete;
  TabDataWebStateObserver& operator=(const TabDataWebStateObserver&) = delete;

  static web::WebState* GetActiveTab();
  static web::WebState* GetTabById(std::int32_t tab_id);

 private:
  struct TabInfo {
    raw_ptr<web::WebState> web_state;
    bool is_active;
  };

  friend class web::WebStateUserData<TabDataWebStateObserver>;
  explicit TabDataWebStateObserver(web::WebState* web_state);

  // web::WebStateObserver:
  void WasShown(web::WebState* web_state) override;
  void WasHidden(web::WebState* web_state) override;
  void DidStartNavigation(web::WebState* web_state,
                          web::NavigationContext* navigation_context) override;

  void DidRedirectNavigation(
      web::WebState* web_state,
      web::NavigationContext* navigation_context) override;
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;

  void DidStartLoading(web::WebState* web_state) override;
  void DidStopLoading(web::WebState* web_state) override;

  void PageLoaded(
      web::WebState* web_state,
      web::PageLoadCompletionStatus load_completion_status) override;

  void DidChangeBackForwardState(web::WebState* web_state) override;

  void TitleWasSet(web::WebState* web_state) override;

  void DidChangeVisibleSecurityState(web::WebState* web_state) override;

  void FaviconUrlUpdated(
      web::WebState* web_state,
      const std::vector<web::FaviconURL>& candidates) override;

  void PermissionStateChanged(web::WebState* web_state,
                              web::Permission permission) override;

  void RenderProcessGone(web::WebState* web_state) override;

  void WebStateRealized(web::WebState* web_state) override;

  void WebStateDestroyed(web::WebState* web_state) override;

  void UpdateTab();

  static void SetActiveTab(web::WebState* web_state, bool active);

  static base::flat_map<std::int32_t, TabInfo>& GetWebStateList();

  int32_t tab_handle_ = 0;

  raw_ptr<web::WebState> web_state_;
  raw_ref<TabTrackerService> service_;

  base::WeakPtrFactory<TabDataWebStateObserver> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_TAB_DATA_WEB_STATE_OBSERVER_H_
