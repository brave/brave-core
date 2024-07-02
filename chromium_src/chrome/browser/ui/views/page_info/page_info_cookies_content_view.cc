/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_info/page_info_cookies_content_view.h"

#include "chrome/browser/ui/views/page_info/page_info_main_view.h"

#define SetCookieInfo SetCookieInfo_ChromiumImpl
#include "src/chrome/browser/ui/views/page_info/page_info_cookies_content_view.cc"
#undef SetCookieInfo

void PageInfoCookiesContentView::SetCookieInfo(
    const CookiesNewInfo& cookie_info) {
  // We need to call SetCookieInfo with controls_visible = false, or the layout
  // will DCHECK since we hide the cookie container. We can't copy the existing
  // struct when doing this because its copy constructor is implicitly deleted,
  // so we just copy over the only other setting that's relevant for us.
  CookiesNewInfo mutable_cookie_info;
  mutable_cookie_info.allowed_sites_count = cookie_info.allowed_sites_count;
  mutable_cookie_info.controls_visible = false;
  SetCookieInfo_ChromiumImpl(mutable_cookie_info);

  // Hide cookies description and link to settings.
  cookies_description_label_->SetVisible(false);
  third_party_cookies_container_->SetVisible(false);

  // Remove separator.
  // cookies_buttons_container_view_'s children are:
  // [0]: separator
  // [1]: on-site data button row, which we want to keep
  if (cookies_buttons_container_view_) {
    if (cookies_buttons_container_view_->children().size() > 0) {
      cookies_buttons_container_view_->RemoveChildView(
          cookies_buttons_container_view_->children()[0]);
    }
  }
  PreferredSizeChanged();
}
