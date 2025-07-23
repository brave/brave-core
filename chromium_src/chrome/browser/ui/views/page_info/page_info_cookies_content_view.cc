/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_info/page_info_cookies_content_view.h"

#include "chrome/browser/ui/views/page_info/page_info_main_view.h"

#define PageInfoCookiesContentView PageInfoCookiesContentView_ChromiumImpl
#include "src/chrome/browser/ui/views/page_info/page_info_cookies_content_view.cc"
#undef PageInfoCookiesContentView

void PageInfoCookiesContentView::SetCookieInfo(
    const CookiesNewInfo& cookie_info) {
  PageInfoCookiesContentView_ChromiumImpl::SetCookieInfo(cookie_info);

  // Hide cookies description and link to settings.
  cookies_description_label_->SetVisible(false);
  third_party_cookies_container_->SetVisible(false);

  // Remove separator.
  // cookies_buttons_container_view_'s children are:
  // [0]: separator
  // [1]: on-site data button row, which we want to keep
  if (cookies_buttons_container_view_) {
    if (cookies_buttons_container_view_->children().size() > 0) {
      cookies_buttons_container_view_->RemoveChildViewT(
          cookies_buttons_container_view_->children()[0]);
    }
  }
  PreferredSizeChanged();
}

void PageInfoCookiesContentView::SetThirdPartyCookiesInfo(
    CookieControlsState controls_state,
    CookieControlsEnforcement enforcement,
    CookieBlocking3pcdStatus blocking_status,
    base::Time expiration) {
  // Passing `kHidden` always to make sure we hide the third-party cookies, but
  // also avoid a CHECK down the line, due to having set other controls as
  // not visible.
  PageInfoCookiesContentView_ChromiumImpl::SetThirdPartyCookiesInfo(
      CookieControlsState::kHidden, enforcement, blocking_status, expiration);
}
