/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_protocol_navigation_throttle.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "base/ranges/algorithm.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/referrer.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"

using content::NavigationHandle;
using content::NavigationThrottle;
using content::WebContents;

namespace brave_rewards {

RewardsProtocolNavigationThrottle::RewardsProtocolNavigationThrottle(
    NavigationHandle* handle)
    : NavigationThrottle(handle) {
  CHECK(handle);
}

RewardsProtocolNavigationThrottle::~RewardsProtocolNavigationThrottle() =
    default;

NavigationThrottle::ThrottleCheckResult
RewardsProtocolNavigationThrottle::WillStartRequest() {
  return MaybeRedirect();
}

NavigationThrottle::ThrottleCheckResult
RewardsProtocolNavigationThrottle::WillRedirectRequest() {
  return MaybeRedirect();
}

bool IsValidWalletProviderRedirect(
    const GURL& referrer_url,
    const GURL& redirect_url,
    const std::map<std::string, std::vector<GURL>>& allowed_referrer_urls) {
  if (!referrer_url.is_valid() || !referrer_url.SchemeIs(url::kHttpsScheme) ||
      !redirect_url.is_valid()) {
    LOG(ERROR) << "Input validation failed!";
    return false;
  }

  std::string wallet_provider;
  const auto redirect_path_segments =
      base::SplitString(redirect_url.path_piece(), "/", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_NONEMPTY);
  if (!redirect_path_segments.empty()) {
    wallet_provider = redirect_path_segments[0];
  }

  if (base::ranges::none_of(
          allowed_referrer_urls.contains(wallet_provider)
              ? allowed_referrer_urls.at(wallet_provider)
              : std::vector<GURL>{},
          [&](std::string_view host_piece) {
            return referrer_url.DomainIs(host_piece);
          },
          &GURL::host_piece)) {
    LOG(ERROR) << referrer_url.host_piece() << " was trying to redirect to "
               << redirect_url.scheme_piece() << "://"
               << redirect_url.host_piece() << redirect_url.path_piece()
               << ", but it's not allowed.";
    return false;
  }

  return true;
}

GURL TransformUrl(const GURL& url) {
  DCHECK(url.is_valid());

  return GURL(base::StrCat(
      {"chrome", url::kStandardSchemeSeparator, "rewards/", url.host(), "/",
       base::TrimString(url.path(), "/", base::TrimPositions::TRIM_LEADING),
       url.has_query() ? "?" + base::EscapeExternalHandlerValue(url.query())
                       : ""}));
}

void MaybeLoadRewardsURL(const GURL& redirect_url, WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  static const auto kAllowedReferrerUrls{[] {
    std::map<std::string, std::vector<GURL>> allowed_urls{
        {"bitflyer",
         {GURL(BUILDFLAG(BITFLYER_PRODUCTION_URL)),
          GURL(BUILDFLAG(BITFLYER_SANDBOX_URL))}},
        {"gemini",
         {GURL(BUILDFLAG(GEMINI_PRODUCTION_OAUTH_URL)),
          GURL(BUILDFLAG(GEMINI_SANDBOX_OAUTH_URL))}},
        {"uphold",
         {GURL(BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)),
          GURL(BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL))}},
        {"zebpay",
         {GURL(BUILDFLAG(ZEBPAY_PRODUCTION_OAUTH_URL)),
          GURL(BUILDFLAG(ZEBPAY_SANDBOX_OAUTH_URL))}}};

    for (const auto& [wallet_provider, urls] : allowed_urls) {
      DCHECK(base::ranges::none_of(
          urls,
          [](const GURL& url) { return !url.is_valid() || !url.has_host(); }))
          << wallet_provider << " has malformed referrer URL(s)!";
    }

    return allowed_urls;
  }()};

  if (const auto transformed_url = TransformUrl(redirect_url);
      IsValidWalletProviderRedirect(web_contents->GetURL(), transformed_url,
                                    kAllowedReferrerUrls)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(
                       [](base::WeakPtr<WebContents> web_contents,
                          const GURL& transformed_url) {
                         if (!web_contents) {
                           return;
                         }
                         web_contents->GetController().LoadURL(
                             transformed_url, content::Referrer(),
                             ui::PAGE_TRANSITION_AUTO_TOPLEVEL, "");
                       },
                       web_contents->GetWeakPtr(), transformed_url));
  }
}

NavigationThrottle::ThrottleCheckResult
RewardsProtocolNavigationThrottle::MaybeRedirect() {
  WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents || !navigation_handle()->IsInPrimaryMainFrame()) {
    return NavigationThrottle::PROCEED;
  }

  GURL original_url = navigation_handle()->GetURL();
  if (original_url.SchemeIs("rewards")) {
    MaybeLoadRewardsURL(original_url, web_contents);
    return NavigationThrottle::CANCEL;
  }

  return NavigationThrottle::PROCEED;
}

const char* RewardsProtocolNavigationThrottle::GetNameForLogging() {
  return "RewardsProtocolNavigationThrottle";
}

}  // namespace brave_rewards
