// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_news_tab_helper.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

BraveNewsTabHelper::BraveNewsTabHelper(content::WebContents* contents)
    : content::WebContentsUserData<BraveNewsTabHelper>(*contents),
      controller_(
          brave_news::BraveNewsControllerFactory::GetControllerForContext(
              contents->GetBrowserContext())) {}

BraveNewsTabHelper::~BraveNewsTabHelper() = default;

bool BraveNewsTabHelper::has_feed() {
  auto publisher = controller_->publisher_controller()->GetPublisherForSite(
      GetWebContents().GetLastCommittedURL());
  if (!publisher)
    return false;

  LOG(ERROR) << "Publisher: " << publisher->publisher_name
             << "Site: " << publisher->site_url;
  return true;
}

bool BraveNewsTabHelper::is_subscribed() {
  return false;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsTabHelper);
