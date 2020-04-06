/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/url_context.h"

#include <memory>
#include <string>

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"

namespace brave {

namespace {

std::string GetUploadData(const network::ResourceRequest& request) {
  std::string upload_data;
  if (!request.request_body) {
    return {};
  }
  const auto* elements = request.request_body->elements();
  for (const network::DataElement& element : *elements) {
    if (element.type() == network::mojom::DataElementType::kBytes) {
      upload_data.append(element.bytes(), element.length());
    }
  }

  return upload_data;
}

}  // namespace

BraveRequestInfo::BraveRequestInfo() = default;

BraveRequestInfo::BraveRequestInfo(const GURL& url) : request_url(url) {}

BraveRequestInfo::~BraveRequestInfo() = default;

// static
void BraveRequestInfo::FillCTX(const network::ResourceRequest& request,
                               int render_process_id,
                               int frame_tree_node_id,
                               uint64_t request_identifier,
                               content::BrowserContext* browser_context,
                               std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ctx->request_identifier = request_identifier;
  ctx->request_url = request.url;
  // TODO(iefremov): Replace GURL with Origin
  ctx->initiator_url =
      request.request_initiator.value_or(url::Origin()).GetURL();

  ctx->referrer = request.referrer;
  ctx->referrer_policy = request.referrer_policy;

  ctx->resource_type =
      static_cast<blink::mojom::ResourceType>(request.resource_type);

  ctx->is_webtorrent_disabled =
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
      !webtorrent::IsWebtorrentEnabled(browser_context);
#else
      true;
#endif

  ctx->render_frame_id = request.render_frame_id;
  ctx->render_process_id = render_process_id;
  ctx->frame_tree_node_id = frame_tree_node_id;

  // TODO(iefremov): remove tab_url. Change tab_origin from GURL to Origin.
  // ctx->tab_url = request.top_frame_origin;
  if (request.trusted_params) {
    ctx->tab_origin =
        request.trusted_params->network_isolation_key.GetTopFrameOrigin()
            .value_or(url::Origin())
            .GetURL();
  }
  // TODO(iefremov): We still need this for WebSockets, currently
  // |AddChannelRequest| provides only old-fashioned |site_for_cookies|.
  // (See |BraveProxyingWebSocket|).
  if (ctx->tab_origin.is_empty()) {
    ctx->tab_origin = brave_shields::BraveShieldsWebContentsObserver::
                          GetTabURLFromRenderFrameInfo(ctx->render_process_id,
                                                       ctx->render_frame_id,
                                                       ctx->frame_tree_node_id)
                              .GetOrigin();
  }

  Profile* profile = Profile::FromBrowserContext(browser_context);
  ctx->allow_brave_shields =
      brave_shields::GetBraveShieldsEnabled(profile, ctx->tab_origin);
  ctx->allow_ads = brave_shields::GetAdControlType(profile, ctx->tab_origin) ==
                   brave_shields::ControlType::ALLOW;
  ctx->allow_http_upgradable_resource =
      !brave_shields::GetHTTPSEverywhereEnabled(profile, ctx->tab_origin);
  ctx->allow_referrers =
      brave_shields::AllowReferrers(profile, ctx->tab_origin);
  ctx->upload_data = GetUploadData(request);
}

}  // namespace brave
