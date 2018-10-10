/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <string>

#include "base/base64url.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/network_constants.h"
#include "brave/common/shield_exceptions.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"
#include "ui/base/resource/resource_bundle.h"

using content::ResourceType;

namespace {

  bool IsImageResourceType(ResourceType resource_type) {
    return resource_type == content::RESOURCE_TYPE_FAVICON ||
      resource_type == content::RESOURCE_TYPE_IMAGE;
  }
  GURL GetBlankDataURLForResourceType(ResourceType resource_type) {
    return GURL(IsImageResourceType(resource_type) ?
        kEmptyImageDataURI : kEmptyDataURI);
  }

}  // namespace

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

void OnBeforeURLRequest_AdBlockTPCheckWork(
    net::URLRequest* request,
    GURL* new_url,
    std::shared_ptr<BraveRequestInfo> ctx) {
  // Proper content settings can't be looked up, so do nothing.
  GURL tab_origin = request->site_for_cookies().GetOrigin();
  if (tab_origin.is_empty() || !tab_origin.has_host()) {
    return;
  }
  DCHECK(ctx->request_identifier != 0);
  if (!g_brave_browser_process->tracking_protection_service()->
      ShouldStartRequest(request->url(), ctx->resource_type, tab_origin.host())) {
    ctx->new_url_spec = GetBlankDataURLForResourceType(ctx->resource_type).spec();
  } else if (!g_brave_browser_process->ad_block_service()->ShouldStartRequest(
           request->url(), ctx->resource_type, tab_origin.host()) ||
       !g_brave_browser_process->ad_block_regional_service()
            ->ShouldStartRequest(request->url(), ctx->resource_type,
                                 tab_origin.host())) {
    ctx->new_url_spec = GetBlankDataURLForResourceType(ctx->resource_type).spec();
  }
}

void OnBeforeURLRequest_OnBeforeURLRequest_AdBlockTPPostCheckWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!ctx->new_url_spec.empty() &&
    ctx->new_url_spec != request->url().spec()) {
    *new_url = GURL(ctx->new_url_spec);
    // TODO: If we ever want to differentiate ads from tracking library
    // counts, then use brave_shields::kTrackers below.
    brave_shields::DispatchBlockedEventFromIO(request,
        brave_shields::kAds);
  }

  next_callback.Run();
}

int OnBeforeURLRequest_AdBlockTPWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  const GURL& url = request->url();
  GURL tab_origin = request->site_for_cookies().GetOrigin();

  // Get global shields, adblock, and TP settings for the tab_origin
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

  // These should probably move to our ad block lists
  if (IsEmptyDataURLRedirect(url) || IsBlockedResource(url)) {
    *new_url = GURL(kEmptyDataURI);
    return net::OK;
  }

  // Proper content settings can't be looked up, so do nothing.
  auto* request_info = content::ResourceRequestInfo::ForRequest(request);
  if (tab_origin.is_empty() || !allow_brave_shields || allow_ads || !request_info) {
    return net::OK;
  }

  ctx->request_url = request->url();
  ctx->resource_type = request_info->GetResourceType();

  g_brave_browser_process->ad_block_service()->
        GetTaskRunner()->PostTaskAndReply(FROM_HERE,
          base::Bind(&OnBeforeURLRequest_AdBlockTPCheckWork,
              base::Unretained(request), new_url, ctx),
          base::Bind(base::IgnoreResult(
              &OnBeforeURLRequest_OnBeforeURLRequest_AdBlockTPPostCheckWork),
              base::Unretained(request),
              new_url, next_callback, ctx));

  return net::ERR_IO_PENDING;
}

}  // namespace brave
