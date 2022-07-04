// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

BraveNewsTabHelper::BraveNewsTabHelper(content::WebContents* contents)
    : content::WebContentsUserData<BraveNewsTabHelper>(*contents) {}

BraveNewsTabHelper::~BraveNewsTabHelper() = default;

bool BraveNewsTabHelper::has_feed() {
  return false;
}

bool BraveNewsTabHelper::is_subscribed() {
  return false;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsTabHelper);
