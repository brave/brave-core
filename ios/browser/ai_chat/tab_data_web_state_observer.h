// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_TAB_DATA_WEB_STATE_OBSERVER_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_TAB_DATA_WEB_STATE_OBSERVER_H_

#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"

namespace web {
class WebState;
}  // namespace web

namespace ai_chat {

class TabTrackerService;

class TabDataWebStateObserver
    : public web::WebStateUserData<TabDataWebStateObserver>,
      public web::WebStateObserver {
 public:
  TabDataWebStateObserver(web::WebState* web_state, TabTrackerService& service);
  ~TabDataWebStateObserver() override;

  // web::WebStateObserver
  void TitleWasSet(web::WebState* web_state) override;
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void WebStateDestroyed(web::WebState* web_state) override;

 private:
  friend class web::WebStateUserData<TabDataWebStateObserver>;

  void UpdateTab();

  raw_ptr<web::WebState> web_state_;
  raw_ref<TabTrackerService> service_;
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_TAB_DATA_WEB_STATE_OBSERVER_H_
