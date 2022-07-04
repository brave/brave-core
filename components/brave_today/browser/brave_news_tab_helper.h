// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_TAB_HELPER_H_

#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "content/public/browser/web_contents_user_data.h"

class BraveNewsTabHelper
    : public content::WebContentsUserData<BraveNewsTabHelper> {
 public:
  BraveNewsTabHelper(const BraveNewsTabHelper&) = delete;
  BraveNewsTabHelper& operator=(const BraveNewsTabHelper&) = delete;

  ~BraveNewsTabHelper() override;

  bool has_feed();
  bool is_subscribed();

 private:
  explicit BraveNewsTabHelper(content::WebContents* contents);
  raw_ptr<brave_news::BraveNewsController> controller_;

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_TAB_HELPER_H_
