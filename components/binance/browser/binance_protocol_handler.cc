/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/binance/browser/binance_protocol_handler.h"

#include <string>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/common/url_constants.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"

namespace {

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
        "chrome://newtab",
        path,
        query
      }));
}

void LoadNewTabURL(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  content::WebContents* web_contents = std::move(web_contents_getter).Run();
  if (!web_contents) {
    return;
  }

  const auto ref_url = web_contents->GetURL();
  if (!ref_url.is_valid()) {
    return;
  }

  // We should only allow binance scheme to be used from accounts.binance.com
  if (!web_contents->GetURL().DomainIs("accounts.binance.com")) {
    return;
  }

  const auto new_url = TranslateUrl(url);
  web_contents->GetController().LoadURL(new_url, content::Referrer(),
      page_transition, std::string());
}

}  // namespace

namespace binance {

void HandleBinanceProtocol(const GURL& url,
                           content::WebContents::OnceGetter web_contents_getter,
                           ui::PageTransition page_transition,
                           bool has_user_gesture) {
  DCHECK(url.SchemeIs(kBinanceScheme));
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&LoadNewTabURL, url, std::move(web_contents_getter),
                     page_transition, has_user_gesture));
}

bool IsBinanceProtocol(const GURL& url) {
  return url.SchemeIs(kBinanceScheme);
}

}  // namespace binance
