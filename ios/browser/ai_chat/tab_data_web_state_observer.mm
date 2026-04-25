// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ai_chat/tab_data_web_state_observer.h"

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_id.h"

namespace ai_chat {

namespace {

mojom::TabDataPtr CreateTabDataFromWebState(web::WebState* web_state) {
  auto tab = mojom::TabData::New();
  tab->id = web_state->GetUniqueIdentifier().identifier();
  if (auto* item = web_state->GetNavigationManager()->GetLastCommittedItem()) {
    tab->content_id = item->GetUniqueID();
  }
  tab->title = base::UTF16ToUTF8(web_state->GetTitle());
  tab->url = web_state->GetLastCommittedURL();
  return tab;
}

}  // namespace

TabDataWebStateObserver::TabDataWebStateObserver(web::WebState* web_state,
                                                 TabTrackerService& service)
    : web_state_(web_state), service_(service) {
  web_state_->AddObserver(this);
}

TabDataWebStateObserver::~TabDataWebStateObserver() {
  web_state_ = nullptr;
}

void TabDataWebStateObserver::TitleWasSet(web::WebState* web_state) {
  UpdateTab();
}

void TabDataWebStateObserver::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!navigation_context->HasCommitted()) {
    return;
  }
  UpdateTab();
}

void TabDataWebStateObserver::WebStateDestroyed(web::WebState* web_state) {
  service_->UpdateTab(web_state->GetUniqueIdentifier().identifier(), nullptr);
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

void TabDataWebStateObserver::UpdateTab() {
  service_->UpdateTab(web_state_->GetUniqueIdentifier().identifier(),
                      CreateTabDataFromWebState(web_state_));
}

}  // namespace ai_chat
