/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_info/page_info_cookies_content_view.h"

#include "chrome/browser/ui/views/page_info/page_info_main_view.h"

// We need to override `control_state` kHidden, or the layout will DCHECK since
// we hide the cookie container. We can't copy or mutate the original
// `cookie_info` passed into `SetCookieInfo` since it is const, and it's copy
// constructor is deleted, and copying the fields manually is prone to bugs when
// new fields are added.
#define BRAVE_PAGE_INFO_COOKIES_CONTENT_VIEW_SET_THIRD_PARTY_COOKIES_INFO \
  controls_state = CookieControlsState::kHidden;

#define SetCookieInfo SetCookieInfo_ChromiumImpl
#include <chrome/browser/ui/views/page_info/page_info_cookies_content_view.cc>
#undef SetCookieInfo
#undef BRAVE_PAGE_INFO_COOKIES_CONTENT_VIEW_SET_THIRD_PARTY_COOKIES_INFO

void PageInfoCookiesContentView::SetCookieInfo(
    const CookiesNewInfo& cookie_info) {
  SetCookieInfo_ChromiumImpl(cookie_info);

  // Hide cookies description and link to settings.
  cookies_description_label_->SetVisible(false);
  third_party_cookies_container_->SetVisible(false);

  // Remove separator.
  // cookies_buttons_container_view_'s children are:
  // [0]: separator
  // [1]: on-site data button row, which we want to keep
  if (cookies_buttons_container_view_) {
    if (cookies_buttons_container_view_->children().size() > 0) {
      // Setting `cookies_dialog_button_` to nullptr as removing the first child
      // view below will result in this pointer being invalidated.
      cookies_dialog_button_ = nullptr;

      cookies_buttons_container_view_->RemoveChildViewT(
          cookies_buttons_container_view_->children()[0]);
    }
  }
  PreferredSizeChanged();
}
