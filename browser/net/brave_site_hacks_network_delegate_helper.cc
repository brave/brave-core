/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "brave/common/network_constants.h"
#include "brave/common/shield_exceptions.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/referrer.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;
using content::Referrer;

namespace brave {

namespace {

bool ApplyPotentialReferrerBlock(std::shared_ptr<BraveRequestInfo> ctx,
                                 net::URLRequest* request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  GURL target_origin = request->url().GetOrigin();
  GURL tab_origin = ctx->tab_origin;
  if (tab_origin.SchemeIs(kChromeExtensionScheme)) {
    return false;
  }
  bool allow_referrers = brave_shields::IsAllowContentSettingFromIO(
      request, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kReferrers);
  bool shields_up = brave_shields::IsAllowContentSettingFromIO(
      request, tab_origin, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields);
  const std::string original_referrer = request->referrer();
  Referrer new_referrer;
  if (brave_shields::ShouldSetReferrer(allow_referrers, shields_up,
          GURL(original_referrer), tab_origin, request->url(), target_origin,
          Referrer::NetReferrerPolicyToBlinkReferrerPolicy(
              request->referrer_policy()), &new_referrer)) {
    request->SetReferrer(new_referrer.url.spec());
    return true;
  }
  return false;
}

}  // namespace

int OnBeforeURLRequest_SiteHacksWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  ApplyPotentialReferrerBlock(ctx, const_cast<net::URLRequest*>(ctx->request));
  return net::OK;
}

void CheckForCookieOverride(const GURL& url, const URLPattern& pattern,
    net::HttpRequestHeaders* headers, const std::string& extra_cookies) {
  if (pattern.MatchesURL(url)) {
    std::string cookies;
    if (headers->GetHeader(kCookieHeader, &cookies)) {
      cookies = "; ";
    }
    cookies += extra_cookies;
    headers->SetHeader(kCookieHeader, cookies);
  }
}

int OnBeforeStartTransaction_SiteHacksWork(net::URLRequest* request,
        net::HttpRequestHeaders* headers,
        const ResponseCallback& next_callback,
        std::shared_ptr<BraveRequestInfo> ctx) {
  // TODO(bridiver): Fix the Forbes cookie override with enabled NetworkService.
  CheckForCookieOverride(request->url(),
      URLPattern(URLPattern::SCHEME_ALL, kForbesPattern), headers,
      kForbesExtraCookies);
  if (IsUAWhitelisted(request->url())) {
    std::string user_agent;
    if (headers->GetHeader(kUserAgentHeader, &user_agent)) {
      base::ReplaceFirstSubstringAfterOffset(&user_agent, 0,
        "Chrome", "Brave Chrome");
      headers->SetHeader(kUserAgentHeader, user_agent);
      ctx->set_headers.insert(kUserAgentHeader);
    }
  }
  return net::OK;
}

}  // namespace brave
