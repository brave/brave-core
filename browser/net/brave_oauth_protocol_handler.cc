/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_oauth_protocol_handler.h"

#include <string>
#include <map>
#include <utility>

#include "base/optional.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/common/url_constants.h"
#include "brave/components/binance/browser/buildflags/buildflags.h"
#include "brave/components/gemini/browser/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/escape.h"
#include "net/base/url_util.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if BUILDFLAG(BINANCE_ENABLED)
#include "brave/browser/binance/binance_service_factory.h"
#include "brave/components/binance/browser/binance_service.h"
#endif

#if BUILDFLAG(GEMINI_ENABLED)
#include "brave/browser/gemini/gemini_service_factory.h"
#include "brave/components/gemini/browser/gemini_service.h"
#endif

namespace {

const std::map<std::string, std::string> redirects = {
  {kBinanceScheme, "chrome://newtab?binanceAuth=1"},
  {kGeminiScheme, "chrome://newtab?geminiAuth=1"},
};

const std::map<std::string, std::string> origins = {
  {kBinanceScheme, "https://accounts.binance.com"},
  {kGeminiScheme, "https://exchange.gemini.com"},
};

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

  std::string oauth_scheme = url.scheme();
  GURL allowed_origin(origins.at(oauth_scheme));

  if (web_contents->GetLastCommittedURL().GetOrigin() != allowed_origin ||
      !initiating_origin.has_value() ||
      initiating_origin.value().GetURL() != allowed_origin) {
    return;
  }

  std::map<std::string, std::string> parts;
  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    parts[it.GetKey()] = it.GetUnescapedValue();
  }

  if (parts.find("code") != parts.end()) {
    std::string auth_token = parts["code"];

    if (oauth_scheme == kBinanceScheme) {
#if BUILDFLAG(BINANCE_ENABLED)
    Profile* profile =
        Profile::FromBrowserContext(
          web_contents->GetBrowserContext());
      BinanceServiceFactory::GetInstance()
        ->GetForProfile(profile)
        ->SetAuthToken(auth_token);
#endif
    } else if (oauth_scheme == kGeminiScheme) {
#if BUILDFLAG(GEMINI_ENABLED)
    Profile* profile =
        Profile::FromBrowserContext(
          web_contents->GetBrowserContext());
      GeminiServiceFactory::GetInstance()
        ->GetForProfile(profile)
        ->SetAuthToken(auth_token);
#endif
    }
  }

  GURL redirect_url(redirects.at(oauth_scheme));
  web_contents->GetController().LoadURL(GURL(redirect_url),
      content::Referrer(), page_transition, std::string());
}

}  // namespace

namespace oauth {

void HandleOauthProtocol(const GURL& url,
                         content::WebContents::OnceGetter web_contents_getter,
                         ui::PageTransition page_transition,
                         bool has_user_gesture,
                         const base::Optional<url::Origin>& initiator) {
  DCHECK(IsOauthProtocol(url));
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&LoadNewTabURL, url, std::move(web_contents_getter),
                     page_transition, has_user_gesture, initiator));
}

bool IsOauthProtocol(const GURL& url) {
#if BUILDFLAG(BINANCE_ENABLED)
  if (url.SchemeIs(kBinanceScheme)) {
    return true;
  }
#endif

#if BUILDFLAG(GEMINI_ENABLED)
  if (url.SchemeIs(kGeminiScheme)) {
    return true;
  }
#endif

  return false;
}

}  // namespace oauth
