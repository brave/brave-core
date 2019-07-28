/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/base64url.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "brave/common/shield_exceptions.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/url_pattern.h"
#include "ui/base/resource/resource_bundle.h"

using content::ResourceType;

namespace brave {

std::string GetGoogleAnalyticsPolyfillJS() {
  static std::string base64_output;
  if (base64_output.length() != 0)  {
    return base64_output;
  }
  std::string str = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
      IDR_BRAVE_GOOGLE_ANALYTICS_POLYFILL).as_string();
  Base64UrlEncode(str, base::Base64UrlEncodePolicy::OMIT_PADDING,
      &base64_output);
  base64_output = std::string(kJSDataURLPrefix) + base64_output;
  return base64_output;
}

std::string GetGoogleTagManagerPolyfillJS() {
  static std::string base64_output;
  if (base64_output.length() != 0)  {
    return base64_output;
  }
  std::string str = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
      IDR_BRAVE_TAG_MANAGER_POLYFILL).as_string();
  Base64UrlEncode(str, base::Base64UrlEncodePolicy::OMIT_PADDING,
      &base64_output);
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
  Base64UrlEncode(str, base::Base64UrlEncodePolicy::OMIT_PADDING,
      &base64_output);
  base64_output = std::string(kJSDataURLPrefix) + base64_output;
  return base64_output;
}

bool GetPolyfillForAdBlock(bool allow_brave_shields, bool allow_ads,
    const GURL& tab_origin, const GURL& gurl, std::string* new_url_spec) {
  // Polyfills which are related to adblock should only apply when shields
  // are up.
  if (!allow_brave_shields || allow_ads) {
    return false;
  }

  static URLPattern analytics(URLPattern::SCHEME_ALL,
      kGoogleAnalyticsPattern);
  static URLPattern tag_manager(URLPattern::SCHEME_ALL,
      kGoogleTagManagerPattern);
  static URLPattern tag_services(URLPattern::SCHEME_ALL,
      kGoogleTagServicesPattern);
  if (analytics.MatchesURL(gurl)) {
    std::string&& data_url = GetGoogleAnalyticsPolyfillJS();
    *new_url_spec = data_url;
    return true;
  }

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

void OnBeforeURLRequestAdBlockTP(
    std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // If the following info isn't available, then proper content settings can't
  // be looked up, so do nothing.
  if (ctx->tab_origin.is_empty() || !ctx->tab_origin.has_host() ||
      ctx->request_url.is_empty()) {
    return;
  }
  DCHECK_NE(ctx->request_identifier, 0UL);

  bool did_match_exception = false;
  const brave_shields::BlockDecision* block_decision = nullptr;
  std::string tab_host = ctx->tab_origin.host();
  if (!g_brave_browser_process->ad_block_service()->ShouldStartRequest(
          ctx->request_url, ctx->resource_type, tab_host, &did_match_exception,
          &ctx->cancel_request_explicitly, &block_decision)) {
    ctx->blocked_by = kAdBlocked;
  } else if (!did_match_exception &&
             !g_brave_browser_process->ad_block_regional_service_manager()
                  ->ShouldStartRequest(ctx->request_url, ctx->resource_type,
                                       tab_host, &did_match_exception,
                                       &ctx->cancel_request_explicitly,
                                       &block_decision)) {
    ctx->blocked_by = kAdBlocked;
  } else if (!did_match_exception &&
             !g_brave_browser_process->ad_block_custom_filters_service()
                  ->ShouldStartRequest(ctx->request_url, ctx->resource_type,
                                       tab_host, &did_match_exception,
                                       &ctx->cancel_request_explicitly,
                                       &block_decision)) {
    ctx->blocked_by = kAdBlocked;
  }

  if (ctx->blocked_by == kAdBlocked) {
    brave_shields::DispatchBlockedEventFromIO(ctx->request_url,
        ctx->render_frame_id, ctx->render_process_id, ctx->frame_tree_node_id,
        brave_shields::kAds);
  }
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

  // Most blocked resources have been moved to our ad block lists.
  // This is only for special cases like the PDFjs ping which can
  // occur before the ad block lists are fully loaded.
  if (IsBlockedResource(ctx->request_url)) {
    ctx->new_url_spec = kEmptyDataURI;

    return net::OK;
  }

  // If the following info isn't available, then proper content settings can't
  // be looked up, so do nothing.
  if (ctx->tab_origin.is_empty() || !ctx->allow_brave_shields ||
      ctx->allow_ads ||
      ctx->resource_type == BraveRequestInfo::kInvalidResourceType) {
    return net::OK;
  }

  OnBeforeURLRequestAdBlockTP(ctx);

  return net::OK;
}

}  // namespace brave
