/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/web/web_state/web_state_native.h"

#include "ios/chrome/browser/sessions/model/ios_chrome_session_tab_helper.h"
#include "ios/chrome/browser/shared/model/browser/browser.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_opener.h"
#include "ios/chrome/browser/tabs/model/synced_window_delegate_browser_agent.h"
#include "ios/web/public/session/crw_navigation_item_storage.h"
#include "ios/web/public/session/crw_session_storage.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/web_state/ui/crw_web_view_navigation_proxy.h"
#include "ios/web/web_state/web_state_impl.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave {

#pragma mark NativeWebState

NativeWebState::NativeWebState(Browser* browser, bool off_the_record)
    : browser_(browser),
      session_id_(SessionID::InvalidValue()),
      web_state_(nullptr),
      web_state_observer_(nullptr) {
  // First setup SessionID for the web-state
  session_id_ =
      SyncedWindowDelegateBrowserAgent::FromBrowser(browser_)->GetSessionId();

  // Create BrowserState
  ChromeBrowserState* browser_state = browser->GetBrowserState();
  if (off_the_record) {
    browser_state = browser_state->GetOffTheRecordChromeBrowserState();
  }

  // Create WebState with parameters
  web::WebState::CreateParams create_params(browser_state);
  create_params.last_active_time = base::Time::Now();
  auto web_state = web::WebState::Create(create_params);
  web_state->ForceRealized();

  // Setup Observers of the WebState
  web_state_ = web_state.get();
  web_state_observer_ = std::make_unique<Observer>(this);

  // Insert the WebState into the Browser && Activate it
  browser_->GetWebStateList()->InsertWebState(
      std::move(web_state), WebStateList::InsertionParams::AtIndex(
                                browser_->GetWebStateList()->count())
                                .Activate());

  // Finally Set the WebState WindowID
  IOSChromeSessionTabHelper::FromWebState(web_state_)->SetWindowID(session_id_);
}

NativeWebState::~NativeWebState() {
  // Cleanup the WebState Observer
  web_state_observer_.reset();

  // Cleanup the WebState
  if (web_state_) {
    int index = browser_->GetWebStateList()->GetIndexOfWebState(web_state_);
    if (index >= 0) {
      browser_->GetWebStateList()->CloseWebStateAt(
          index, WebStateList::ClosingFlags::CLOSE_USER_ACTION);
    }
  }

  // Cleanup everything else in reverse order of construction
  web_state_ = nullptr;
  session_id_ = SessionID::InvalidValue();
  browser_ = nullptr;
}

void NativeWebState::SetTitle(const std::u16string& title) {
  DCHECK(web_state_);

  if (web_state_) {
    web::NavigationManager* navigation_manager =
        web_state_->GetNavigationManager();
    DCHECK(navigation_manager);

    web::NavigationItem* item = navigation_manager->GetPendingItem();

    if (!item) {
      navigation_manager->GetLastCommittedItem();
    }

    if (!item) {
      item = navigation_manager->GetVisibleItem();
    }

    if (item) {
      item->SetTitle(title);
      static_cast<web::WebStateImpl*>(web_state_)->OnTitleChanged();
    }
  }
}

void NativeWebState::SetURL(const GURL& url) {
  DCHECK(web_state_);

  if (web_state_) {
    web::NavigationManagerImpl* navigation_manager =
        static_cast<web::NavigationManagerImpl*>(
            web_state_->GetNavigationManager());
    DCHECK(navigation_manager);

    navigation_manager->AddPendingItem(
        url, web::Referrer(), ui::PAGE_TRANSITION_TYPED,
        web::NavigationInitiationType::BROWSER_INITIATED,
        /*is_post_navigation=*/false, /*is_error_navigation*/ false,
        web::HttpsUpgradeType::kNone);

    navigation_manager->CommitPendingItem();
    static_cast<web::WebStateImpl*>(web_state_)->OnPageLoaded(url, true);
  }
}

base::WeakPtr<web::WebState> NativeWebState::GetWeakWebState() {
  DCHECK(web_state_);
  return web_state_->GetWeakPtr();
}

NativeWebState::Observer::Observer(NativeWebState* state)
    : native_state_(state) {
  DCHECK(native_state_);
  DCHECK(native_state_->web_state_);
  native_state_->web_state_->AddObserver(this);
}

NativeWebState::Observer::~Observer() {
  if (native_state_->web_state_) {
    native_state_->web_state_->RemoveObserver(this);
  }
  native_state_ = nullptr;
}

void NativeWebState::Observer::WebStateDestroyed(web::WebState* web_state) {
  if (web_state == native_state_->web_state_) {
    native_state_->web_state_ = nullptr;
  }
  web_state->RemoveObserver(this);
}

}  // namespace brave
