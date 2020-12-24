/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ftx/ftx_protocol_handler.h"

#include <string>
#include <map>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/browser/ftx/ftx_service_factory.h"
#include "brave/common/url_constants.h"
#include "brave/components/ftx/browser/ftx_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "net/base/url_util.h"

namespace {

void LoadNewTabURL(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiating_origin) {
  content::WebContents* web_contents = std::move(web_contents_getter).Run();
  if (!web_contents) {
    return;
  }

  const auto ref_url = web_contents->GetURL();
  if (!ref_url.is_valid()) {
    return;
  }

  GURL allowed_origin_one("https://ftx.us");
  GURL allowed_origin_two("https://ftx.com");

  if (web_contents->GetLastCommittedURL().GetOrigin() != allowed_origin_one ||
      web_contents->GetLastCommittedURL().GetOrigin() != allowed_origin_two ||
      !initiating_origin.has_value() ||
      initiating_origin.value().GetURL() != allowed_origin_one ||
      initiating_origin.value().GetURL() != allowed_origin_two) {
    return;
  }

  std::map<std::string, std::string> parts;
  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    parts[it.GetKey()] = it.GetUnescapedValue();
  }
  if (parts.find("code") != parts.end()) {
    std::string auth_token = parts["code"];
    /*
    Profile* profile =
        Profile::FromBrowserContext(web_contents->GetBrowserContext());
    FTXServiceFactory::GetInstance()
      ->GetForProfile(profile)
      ->SetAuthToken(auth_token);
    */
  }

  web_contents->GetController().LoadURL(GURL("chrome://newtab?ftxAuth=1"),
      content::Referrer(), page_transition, std::string());
}

}  // namespace

namespace ftx {

void HandleFTXProtocol(const GURL& url,
                           content::WebContents::OnceGetter web_contents_getter,
                           ui::PageTransition page_transition,
                           bool has_user_gesture,
                           const base::Optional<url::Origin>& initiator) {
  DCHECK(IsFTXProtocol(url));
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&LoadNewTabURL, url, std::move(web_contents_getter),
                     page_transition, has_user_gesture, initiator));
}

bool IsFTXProtocol(const GURL& url) {
  return url.SchemeIs(kFTXScheme);
}

}  // namespace ftx
