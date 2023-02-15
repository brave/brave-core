/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEB_PAGE_BACKGROUND_COLOR_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_WEB_PAGE_BACKGROUND_COLOR_TAB_HELPER_H_

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// Set base background color for WebContents if needed.
class WebPageBackgroundColorTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebPageBackgroundColorTabHelper> {
 public:
  ~WebPageBackgroundColorTabHelper() override;

  WebPageBackgroundColorTabHelper(const WebPageBackgroundColorTabHelper&) =
      delete;
  WebPageBackgroundColorTabHelper& operator=(
      const WebPageBackgroundColorTabHelper&) = delete;

 private:
  explicit WebPageBackgroundColorTabHelper(content::WebContents* web_contents);

  friend class content::WebContentsUserData<WebPageBackgroundColorTabHelper>;

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEB_PAGE_BACKGROUND_COLOR_TAB_HELPER_H_
