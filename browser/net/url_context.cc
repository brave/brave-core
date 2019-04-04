/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "brave/browser/net/url_context.h"

#include <memory>
#include <string>

#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/resource_request_info.h"

namespace brave {

BraveRequestInfo::BraveRequestInfo() {
}

BraveRequestInfo::~BraveRequestInfo() {
}

void BraveRequestInfo::FillCTXFromRequest(const net::URLRequest* request,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  ctx->request_identifier = request->identifier();
  ctx->request_url = request->url();
  if (request->initiator().has_value()) {
    ctx->initiator_url = request->initiator()->GetURL();
  }
  auto* request_info = content::ResourceRequestInfo::ForRequest(request);
  if (request_info) {
    ctx->resource_type = request_info->GetResourceType();
  }
  brave_shields::GetRenderFrameInfo(request,
                                    &ctx->render_frame_id,
                                    &ctx->render_process_id,
                                    &ctx->frame_tree_node_id);
  if (!request->site_for_cookies().is_empty()) {
    ctx->tab_url = request->site_for_cookies();
  } else {
    // We can not always use site_for_cookies since it can be empty in certain
    // cases. See the comments in url_request.h
    ctx->tab_url = brave_shields::BraveShieldsWebContentsObserver::
        GetTabURLFromRenderFrameInfo(ctx->render_process_id,
                                     ctx->render_frame_id,
                                     ctx->frame_tree_node_id).GetOrigin();
  }
  ctx->tab_origin = ctx->tab_url.GetOrigin();
  ctx->allow_brave_shields = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields) &&
    !request->site_for_cookies().SchemeIs(kChromeExtensionScheme);
  ctx->allow_ads = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kAds);
  ctx->allow_http_upgradable_resource =
      brave_shields::IsAllowContentSettingFromIO(request, ctx->tab_origin,
          ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kHTTPUpgradableResources);
  ctx->allow_1p_cookies = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, GURL("https://firstParty/"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  ctx->allow_3p_cookies = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies);
  ctx->request = request;
}

}  // namespace brave
