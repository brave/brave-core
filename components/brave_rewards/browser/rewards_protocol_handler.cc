/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_protocol_handler.h"

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/ranges/algorithm.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/referrer.h"

namespace brave_rewards {

GURL TransformUrl(const GURL& url) {
  DCHECK(url.is_valid());

  return GURL(base::StrCat(
      {"chrome://rewards/",
       base::TrimString(url.path(), "/", base::TrimPositions::TRIM_LEADING),
       url.has_query() ? "?" + base::EscapeExternalHandlerValue(url.query())
                       : ""}));
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
               << redirect_url.scheme_piece() << ":"
               << redirect_url.path_piece() << ", but it's not allowed.";
    return false;
  }

  return true;
}

void LoadRewardsURL(const GURL& redirect_url,
                    content::WebContents* web_contents,
                    ui::PageTransition page_transition) {
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

  if (IsValidWalletProviderRedirect(web_contents->GetURL(), redirect_url,
                                    kAllowedReferrerUrls)) {
    web_contents->GetController().LoadURL(
        TransformUrl(redirect_url), content::Referrer(), page_transition, "");
  }
}

bool IsRewardsProtocol(const GURL& url) {
  return url.SchemeIs("rewards");
}

void HandleRewardsProtocol(const GURL& url,
                           content::WebContents* web_contents,
                           ui::PageTransition page_transition) {
  CHECK(IsRewardsProtocol(url));

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&LoadRewardsURL, url, web_contents, page_transition));
}

}  // namespace brave_rewards
