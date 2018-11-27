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
#include "extensions/common/url_pattern.h"
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
    const GURL& tab_origin, const GURL& gurl, std::string* new_url_spec) {
  // Polyfills which are related to adblock should only apply when shields are up
  if (!allow_brave_shields || allow_ads) {
    return false;
  }

  static URLPattern tag_manager(URLPattern::SCHEME_ALL, kGoogleTagManagerPattern);
  static URLPattern tag_services(URLPattern::SCHEME_ALL, kGoogleTagServicesPattern);
  if (tag_manager.MatchesURL(gurl)) {
    std::string&& data_url = GetGoogleTagManagerPolyfillJS();
    *new_url_spec = data_url;
    return true;
  }

  if (tag_services.MatchesURL(gurl)) {
    std::string&& data_url = GetGoogleTagServicesPolyfillJS();
    *new_url_spec = data_url;
    return true;
  }

  return false;
}

void OnBeforeURLRequestAdBlockTPOnTaskRunner(std::shared_ptr<BraveRequestInfo> ctx) {
  // If the following info isn't available, then proper content settings can't
  // be looked up, so do nothing.
  if (ctx->tab_origin.is_empty() || !ctx->tab_origin.has_host() ||
      ctx->request_url.is_empty()) {
    return;
  }
  DCHECK(ctx->request_identifier != 0);
  if (!g_brave_browser_process->tracking_protection_service()->
      ShouldStartRequest(ctx->request_url, ctx->resource_type, ctx->tab_origin.host())) {
    ctx->new_url_spec = GetBlankDataURLForResourceType(ctx->resource_type).spec();
    ctx->blocked_by = kTrackerBlocked;
  } else if (!g_brave_browser_process->ad_block_service()->ShouldStartRequest(
           ctx->request_url, ctx->resource_type, ctx->tab_origin.host()) ||
       !g_brave_browser_process->ad_block_regional_service()
            ->ShouldStartRequest(ctx->request_url, ctx->resource_type,
                                 ctx->tab_origin.host())) {
    ctx->new_url_spec = GetBlankDataURLForResourceType(ctx->resource_type).spec();
    ctx->blocked_by = kAdBlocked;
  }
}

void OnBeforeURLRequestDispatchOnIOThread(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!ctx->new_url_spec.empty() &&
    ctx->new_url_spec != ctx->request_url.spec()) {
    if (ctx->blocked_by == kAdBlocked) {
      brave_shields::DispatchBlockedEventFromIO(ctx->request_url,
          ctx->render_process_id, ctx->render_frame_id, ctx->frame_tree_node_id,
          brave_shields::kAds);
    } else if (ctx->blocked_by == kTrackerBlocked) {
      brave_shields::DispatchBlockedEventFromIO(ctx->request_url,
          ctx->render_process_id, ctx->render_frame_id, ctx->frame_tree_node_id,
          brave_shields::kTrackers);
    }
  }

  next_callback.Run();
}

int OnBeforeURLRequest_AdBlockTPPreWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {

  if (ctx->request_url.is_empty()) {
    return net::OK;
  }

  if (GetPolyfillForAdBlock(ctx->allow_brave_shields, ctx->allow_ads,
        ctx->tab_origin, ctx->request_url, &ctx->new_url_spec)) {
    return net::OK;
  }

  // These should probably move to our ad block lists
  if (IsEmptyDataURLRedirect(ctx->request_url) || IsBlockedResource(ctx->request_url)) {
    ctx->new_url_spec = kEmptyDataURI;
    return net::OK;
  }

  // If the following info isn't available, then proper content settings can't
  // be looked up, so do nothing.
  if (ctx->tab_origin.is_empty() || !ctx->allow_brave_shields || ctx->allow_ads ||
      ctx->resource_type == content::RESOURCE_TYPE_LAST_TYPE) {
    return net::OK;
  }

  g_brave_browser_process->ad_block_service()->
        GetTaskRunner()->PostTaskAndReply(FROM_HERE,
          base::Bind(&OnBeforeURLRequestAdBlockTPOnTaskRunner, ctx),
          base::Bind(base::IgnoreResult(
              &OnBeforeURLRequestDispatchOnIOThread), next_callback, ctx));

  return net::ERR_IO_PENDING;
}

}  // namespace brave
