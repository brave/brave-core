/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/renderer/search_result_ad_renderer_throttle.h"

#include <utility>

#include "base/feature_list.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/common/search_result_ad_util.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "content/public/renderer/render_frame.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "url/gurl.h"

namespace brave_ads {

std::unique_ptr<blink::URLLoaderThrottle>
SearchResultAdRendererThrottle::MaybeCreateThrottle(
    int render_frame_id,
    const blink::WebURLRequest& request) {
  if (request.GetRequestContext() != blink::mojom::RequestContextType::FETCH ||
      !base::FeatureList::IsEnabled(
          features::kSupportBraveSearchResultAdConfirmationEvents)) {
    return nullptr;
  }

  absl::optional<blink::WebSecurityOrigin> top_frame_origin =
      request.TopFrameOrigin();
  if (!top_frame_origin) {
    return nullptr;
  }
  const GURL top_frame_origin_url =
      static_cast<url::Origin>(*top_frame_origin).GetURL();
  if (!brave_search::IsAllowedHost(top_frame_origin_url)) {
    return nullptr;
  }

  const GURL requestor_origin_url =
      static_cast<url::Origin>(request.RequestorOrigin()).GetURL();
  if (!brave_search::IsAllowedHost(requestor_origin_url)) {
    return nullptr;
  }

  const GURL url = static_cast<GURL>(request.Url());
  if (!IsSearchResultAdViewedConfirmationUrl(url)) {
    return nullptr;
  }

  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(render_frame_id);
  if (!render_frame || !render_frame->IsMainFrame()) {
    return nullptr;
  }

  mojo::PendingRemote<brave_ads::mojom::BraveAdsHost> brave_ads_pending_remote;
  render_frame->GetBrowserInterfaceBroker()->GetInterface(
      brave_ads_pending_remote.InitWithNewPipeAndPassReceiver());

  auto throttle = std::make_unique<SearchResultAdRendererThrottle>(
      std::move(brave_ads_pending_remote));

  return throttle;
}

SearchResultAdRendererThrottle::SearchResultAdRendererThrottle(
    mojo::PendingRemote<brave_ads::mojom::BraveAdsHost>
        brave_ads_pending_remote)
    : brave_ads_pending_remote_(std::move(brave_ads_pending_remote)) {
  DCHECK(brave_ads_pending_remote_);
}

SearchResultAdRendererThrottle::~SearchResultAdRendererThrottle() = default;

void SearchResultAdRendererThrottle::DetachFromCurrentSequence() {}

void SearchResultAdRendererThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  DCHECK(request);
  DCHECK(request->request_initiator);
  DCHECK(brave_search::IsAllowedHost(request->request_initiator->GetURL()));
  DCHECK_EQ(request->resource_type,
            static_cast<int>(blink::mojom::ResourceType::kXhr));
  DCHECK(request->is_fetch_like_api);

  const std::string creative_instance_id =
      GetViewedSearchResultAdCreativeInstanceId(*request);
  if (creative_instance_id.empty()) {
    brave_ads_pending_remote_.reset();
    return;
  }

  mojo::Remote<brave_ads::mojom::BraveAdsHost> brave_ads_remote(
      std::move(brave_ads_pending_remote_));
  DCHECK(brave_ads_remote.is_bound());
  brave_ads_remote.reset_on_disconnect();

  brave_ads::mojom::BraveAdsHost* raw_brave_ads_remote = brave_ads_remote.get();
  raw_brave_ads_remote->MaybeTriggerAdViewedEvent(
      creative_instance_id,
      base::BindOnce(
          &SearchResultAdRendererThrottle::OnMaybeTriggerAdViewedEvent,
          weak_factory_.GetWeakPtr(), std::move(brave_ads_remote)));

  *defer = true;
}

void SearchResultAdRendererThrottle::OnMaybeTriggerAdViewedEvent(
    mojo::Remote<brave_ads::mojom::BraveAdsHost> /*brave_ads_remote*/,
    bool event_triggered) {
  if (event_triggered) {
    delegate_->CancelWithError(net::ERR_ABORTED);
  } else {
    delegate_->Resume();
  }
}

}  // namespace brave_ads
