/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "brave/browser/net/url_context.h"

#include <string>

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "content/public/browser/resource_request_info.h"

namespace brave {

BraveRequestInfo::BraveRequestInfo() {
}

BraveRequestInfo::~BraveRequestInfo() {
}

void BraveRequestInfo::FillCTXFromRequest(net::URLRequest* request,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  ctx->request_identifier = request->identifier();
  ctx->request_url = request->url();
  ctx->tab_origin = request->site_for_cookies().GetOrigin();
  auto* request_info = content::ResourceRequestInfo::ForRequest(request);
  if (request_info) {
    ctx->resource_type = request_info->GetResourceType();
  }
  brave_shields::GetRenderFrameInfo(request, &ctx->render_process_id, &ctx->render_frame_id,
      &ctx->frame_tree_node_id);
  ctx->allow_brave_shields = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields);
  ctx->allow_ads = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kAds);
  ctx->allow_http_upgradable_resource = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kHTTPUpgradableResources);
  ctx->request = request;
}


}  // namespace brave
