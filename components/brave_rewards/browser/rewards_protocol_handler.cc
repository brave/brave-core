/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_protocol_handler.h"

#include <string>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"

namespace {

bool IsRewardsEnabled(content::BrowserContext* browser_context) {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (!profile) {
    return false;
  }

  return profile->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled);
}

GURL TranslateUrl(const GURL& url) {
  if (!url.is_valid()) {
    return GURL();
  }

  std::string path = url.path();
  std::string query;

  if (url.has_query()) {
    query = base::StrCat({
      "?",
      net::EscapeExternalHandlerValue(url.query())
    });
  }

  base::ReplaceFirstSubstringAfterOffset(&path, 0, "/", "");

  return GURL(
      base::StrCat({
        "chrome://rewards",
        path,
        query
      }));
}

void LoadOrLaunchRewardsURL(
    const GURL& url,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents) {
    return;
  }

  if (IsRewardsEnabled(web_contents->GetBrowserContext())) {
    const auto new_url = TranslateUrl(url);
    web_contents->GetController().LoadURL(new_url, content::Referrer(),
        page_transition, std::string());
  }
}

}  // namespace

namespace brave_rewards {

bool HandleRewardsProtocol(
    const GURL& url,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  if (url.SchemeIs(kRewardsScheme)) {
    base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::UI},
        base::BindOnce(&LoadOrLaunchRewardsURL, url, web_contents_getter,
        page_transition, has_user_gesture));
    return true;
  }

  return false;
}

}  // namespace brave_rewards
