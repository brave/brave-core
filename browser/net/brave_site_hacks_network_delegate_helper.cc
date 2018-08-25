/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include <string>

#include "base/base64url.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "brave/common/network_constants.h"
#include "brave/common/shield_exceptions.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/referrer.h"
#include "extensions/common/url_pattern.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request.h"
#include "ui/base/resource/resource_bundle.h"

using content::BrowserThread;
using content::Referrer;
using namespace net::registry_controlled_domains;

namespace brave {

std::string GetGoogleTagManagerPolyfillJS() {
  static std::string base64_output;
  if (base64_output.length() != 0)  {
    return base64_output;
  }
  std::string str = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
    IDR_BRAVE_TAG_MANAGER_POLYFILL).as_string();
  base64_output.reserve(180);
  Base64UrlEncode(str, base::Base64UrlEncodePolicy::OMIT_PADDING, &base64_output);
  base64_output = std::string(kJSDataURLPrefix) + base64_output;
  return base64_output;
}

std::string GetGoogleTagServicesPolyfillJS() {
  static std::string base64_output;
  if (base64_output.length() != 0)  {
    return base64_output;
  }
  std::string str = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
    IDR_BRAVE_TAG_SERVICES_POLYFILL).as_string();
  base64_output.reserve(4668);
  Base64UrlEncode(str, base::Base64UrlEncodePolicy::OMIT_PADDING, &base64_output);
  base64_output = std::string(kJSDataURLPrefix) + base64_output;
  return base64_output;
}

bool GetPolyfillForAdBlock(bool allow_brave_shields, bool allow_ads,
    const GURL& tab_origin, const GURL& gurl, GURL *new_url) {
  // Polyfills which are related to adblock should only apply when shields are up
  if (!allow_brave_shields || allow_ads) {
    return false;
  }

  static URLPattern tag_manager(URLPattern::SCHEME_ALL, kGoogleTagManagerPattern);
  static URLPattern tag_services(URLPattern::SCHEME_ALL, kGoogleTagServicesPattern);
  if (tag_manager.MatchesURL(gurl)) {
    std::string&& data_url = GetGoogleTagManagerPolyfillJS();
    *new_url = GURL(data_url);
    return true;
  }

  if (tag_services.MatchesURL(gurl)) {
    std::string&& data_url = GetGoogleTagServicesPolyfillJS();
    *new_url = GURL(data_url);
    return true;
  }

  return false;
}

int OnBeforeURLRequest_SiteHacksWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  const GURL& url = request->url();

  if (IsEmptyDataURLRedirect(url)) {
    *new_url = GURL(kEmptyDataURI);
    return net::OK;
  }

  if (IsBlockedResource(url)) {
    request->Cancel();
    return net::ERR_ABORTED;
  }

  GURL tab_origin = request->site_for_cookies().GetOrigin();
  bool allow_brave_shields = brave_shields::IsAllowContentSettingFromIO(
      request, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields);
  bool allow_ads = brave_shields::IsAllowContentSettingFromIO(
      request, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kAds);
  if (GetPolyfillForAdBlock(allow_brave_shields, allow_ads,
        tab_origin, url, new_url)) {
    return net::OK;
  }

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

bool IsBlockTwitterSiteHack(net::URLRequest* request,
    net::HttpRequestHeaders* headers) {
  URLPattern redirectURLPattern(URLPattern::SCHEME_ALL, kTwitterRedirectURL);
  URLPattern referrerPattern(URLPattern::SCHEME_ALL, kTwitterReferrer);
  if (redirectURLPattern.MatchesURL(request->url())) {
    std::string referrer;
    if (headers->GetHeader(kRefererHeader, &referrer) &&
        referrerPattern.MatchesURL(GURL(referrer))) {
      return true;
    }
  }
  return false;
}

int ApplyPotentialReferrerBlock(net::URLRequest* request,
    const ResponseCallback& next_callback,
    net::HttpRequestHeaders* headers) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  GURL target_origin = GURL(request->url()).GetOrigin();
  GURL tab_origin = request->site_for_cookies().GetOrigin();
  bool allow_referrers = brave_shields::IsAllowContentSettingFromIO(
      request, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kReferrers);
  bool shields_up = brave_shields::IsAllowContentSettingFromIO(
      request, tab_origin, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields);
  std::string original_referrer;
  headers->GetHeader(kRefererHeader, &original_referrer);
  Referrer new_referrer;
  if (brave_shields::ShouldSetReferrer(allow_referrers, shields_up,
          GURL(original_referrer), tab_origin, request->url(), target_origin,
          Referrer::NetReferrerPolicyToBlinkReferrerPolicy(
              request->referrer_policy()), &new_referrer)) {
      headers->SetHeader(kRefererHeader, new_referrer.url.spec());
  }
  return net::OK;
}

int OnBeforeStartTransaction_SiteHacksWork(net::URLRequest* request,
        net::HttpRequestHeaders* headers,
        const ResponseCallback& next_callback,
        std::shared_ptr<BraveRequestInfo> ctx) {
  CheckForCookieOverride(request->url(),
      URLPattern(URLPattern::SCHEME_ALL, kForbesPattern), headers,
      kForbesExtraCookies);
  if (IsBlockTwitterSiteHack(request, headers)) {
    request->Cancel();
    return net::ERR_ABORTED;
  }
  if (IsUAWhitelisted(request->url())) {
    std::string user_agent;
    if (headers->GetHeader(kUserAgentHeader, &user_agent)) {
      base::ReplaceFirstSubstringAfterOffset(&user_agent, 0, "Chrome", "Brave Chrome");
      headers->SetHeader(kUserAgentHeader, user_agent);
    }
  }
  return ApplyPotentialReferrerBlock(request, next_callback, headers);
}

}  // namespace brave
