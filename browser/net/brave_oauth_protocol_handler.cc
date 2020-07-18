/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_oauth_protocol_handler.h"

#include <string>
#include <map>
#include <utility>
#include <vector>

#include "base/optional.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/common/url_constants.h"
#include "brave/components/binance/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
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

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/common/url_constants.h"
#endif

#if BUILDFLAG(GEMINI_ENABLED)
#include "brave/browser/gemini/gemini_service_factory.h"
#include "brave/components/gemini/browser/gemini_service.h"
#endif

namespace {

const std::map<std::string, std::string> redirects = {
  {kBinanceScheme, "chrome://newtab?binanceAuth=1"},
  {kGeminiScheme, "chrome://newtab?geminiAuth=1"},
  {kRewardsScheme, "chrome://rewards"},
};

const std::map<std::string, std::string> origins = {
  {kBinanceScheme, "https://accounts.binance.com"},
  {kGeminiScheme, "https://exchange.gemini.com"},
};

const std::vector<std::string> rewards_origins = {
  "https://uphold.com",
  "https://sandbox.uphold.com"
};

const std::string GetAuthToken(const GURL& url) {
  std::map<std::string, std::string> parts;

  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    parts[it.GetKey()] = it.GetUnescapedValue();
  }

  if (parts.find("code") != parts.end()) {
    return parts["code"];
  } else {
    return std::string();
  }
}

GURL GetRedirectURL(const GURL& url,
                    bool preserve_path = false) {
  if (!url.is_valid()) {
    return GURL();
  }

  const std::string scheme = url.scheme();

  if (!preserve_path) {
    return GURL(redirects.at(scheme));
  }

  std::string query;
  std::string path = url.path();

  if (url.has_query()) {
    query = base::StrCat({
      "?",
      net::EscapeExternalHandlerValue(url.query())
    });
  }

  base::ReplaceFirstSubstringAfterOffset(&path, 0, "/", "");

  return GURL(
      base::StrCat({
        redirects.at(scheme),
        path,
        query
      }));
}

void SetAuthToken(const std::string& auth_token,
                  const std::string& oauth_scheme,
                  content::WebContents* web_contents) {
  if (auth_token.empty()) {
    return;
  }

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

bool IsAllowed(content::WebContents* web_contents,
               const base::Optional<url::Origin>& initiating_origin,
               const std::string& origin) {
  GURL allowed_origin(origin);
  bool unallowed_origin =
    !initiating_origin.has_value() ||
    initiating_origin.value().GetURL() != allowed_origin ||
    web_contents->GetLastCommittedURL().GetOrigin() != allowed_origin;
  return !unallowed_origin;
}

bool ShouldRedirect(content::WebContents* web_contents,
                    const base::Optional<url::Origin>& initiating_origin,
                    const std::string& scheme) {
  if (scheme == kRewardsScheme) {
    for (const auto& origin : rewards_origins) {
      if (IsAllowed(web_contents, initiating_origin, origin)) {
        return true;
      }
    }
    return false;
  }

  return IsAllowed(web_contents, initiating_origin, origins.at(scheme));
}

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

  const std::string oauth_scheme = url.scheme();
  if (!ShouldRedirect(web_contents,
         initiating_origin, oauth_scheme)) {
    return;
  }

  SetAuthToken(
    GetAuthToken(url), oauth_scheme, web_contents);

  const auto redirect_url = GetRedirectURL(url,
    (oauth_scheme == kRewardsScheme));
  web_contents->GetController().LoadURL(redirect_url,
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

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  if (url.SchemeIs(kRewardsScheme)) {
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
