/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/url_context.h"

#include <memory>
#include <string>

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "net/base/isolation_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/origin.h"

namespace brave {

namespace {

std::string GetUploadData(const network::ResourceRequest& request) {
  std::string upload_data;
  if (!request.request_body) {
    return {};
  }
  const auto* elements = request.request_body->elements();
  for (const network::DataElement& element : *elements) {
    if (element.type() == network::mojom::DataElementDataView::Tag::kBytes) {
      const auto& bytes = element.As<network::DataElementBytes>().bytes();
      upload_data.append(bytes.begin(), bytes.end());
    }
  }

  return upload_data;
}

}  // namespace

BraveRequestInfo::BraveRequestInfo() = default;

BraveRequestInfo::BraveRequestInfo(const GURL& url) : request_url(url) {}

BraveRequestInfo::~BraveRequestInfo() = default;

// static
std::shared_ptr<brave::BraveRequestInfo> BraveRequestInfo::MakeCTX(
    const network::ResourceRequest& request,
    int render_process_id,
    int frame_tree_node_id,
    uint64_t request_identifier,
    content::BrowserContext* browser_context,
    std::shared_ptr<brave::BraveRequestInfo> old_ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto ctx = std::make_shared<brave::BraveRequestInfo>();
  ctx->request_identifier = request_identifier;
  ctx->method = request.method;
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

  ctx->frame_tree_node_id = frame_tree_node_id;

  // TODO(iefremov): remove tab_url. Change tab_origin from GURL to Origin.
  // ctx->tab_url = request.top_frame_origin;
  if (request.trusted_params) {
    // TODO(iefremov): Turns out it provides us a not expected value for
    // cross-site top-level navigations. Fortunately for now it is not a problem
    // for shields functionality. We should reconsider this machinery, also
    // given that this is always empty for subresources.
    ctx->network_anonymization_key =
        request.trusted_params->isolation_info.network_anonymization_key();
    ctx->tab_origin = request.trusted_params->isolation_info.top_frame_origin()
                          .value_or(url::Origin())
                          .GetURL();
  }
  // TODO(iefremov): We still need this for WebSockets, currently
  // |AddChannelRequest| provides only old-fashioned |site_for_cookies|.
  // (See |BraveProxyingWebSocket|).
  if (ctx->tab_origin.is_empty()) {
    content::WebContents* contents =
        content::WebContents::FromFrameTreeNodeId(ctx->frame_tree_node_id);
    if (contents) {
      ctx->tab_origin =
          url::Origin::Create(contents->GetLastCommittedURL()).GetURL();
    }
  }

  if (old_ctx) {
    ctx->internal_redirect = old_ctx->internal_redirect;
    ctx->redirect_source = old_ctx->redirect_source;
  }

  Profile* profile = Profile::FromBrowserContext(browser_context);
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  ctx->allow_brave_shields =
      map ? brave_shields::GetBraveShieldsEnabled(map, ctx->tab_origin) : true;
  ctx->allow_ads =
      map ? brave_shields::GetAdControlType(map, ctx->tab_origin) ==
                brave_shields::ControlType::ALLOW
          : false;
  // Currently, "aggressive" mode is registered as a cosmetic filtering control
  // type, even though it can also affect network blocking.
  ctx->aggressive_blocking =
      map ? brave_shields::GetCosmeticFilteringControlType(
                map, ctx->tab_origin) == brave_shields::ControlType::BLOCK
          : false;

  // HACK: after we fix multiple creations of BraveRequestInfo we should
  // use only tab_origin. Since we recreate BraveRequestInfo during consequent
  // stages of navigation, |tab_origin| changes and so does |allow_referrers|
  // flag, which is not what we want for determining referrers.
  ctx->allow_referrers =
      map ? brave_shields::AreReferrersAllowed(
                map, ctx->redirect_source.is_empty() ? ctx->tab_origin
                                                     : ctx->redirect_source)
          : false;
  ctx->upload_data = GetUploadData(request);

  ctx->browser_context = browser_context;

  // TODO(fmarier): remove this once the hacky code in
  // brave_proxying_url_loader_factory.cc is refactored. See
  // BraveProxyingURLLoaderFactory::InProgressRequest::UpdateRequestInfo().
  if (old_ctx) {
    ctx->internal_redirect = old_ctx->internal_redirect;
    ctx->redirect_source = old_ctx->redirect_source;
  }

  ctx->devtools_request_id = request.devtools_request_id;

  return ctx;
}

}  // namespace brave
