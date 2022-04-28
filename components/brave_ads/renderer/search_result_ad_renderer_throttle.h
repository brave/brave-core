/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_SEARCH_RESULT_AD_RENDERER_THROTTLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_SEARCH_RESULT_AD_RENDERER_THROTTLE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/brave_ads_host.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace blink {
class WebURLRequest;
}  // namespace blink

namespace brave_ads {

class SearchResultAdRendererThrottle : public blink::URLLoaderThrottle {
 public:
  static std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottle(
      int render_frame_id,
      const blink::WebURLRequest& request);

  SearchResultAdRendererThrottle(
      mojo::PendingRemote<brave_ads::mojom::BraveAdsHost> brave_ads_remote,
      std::string creative_instance_id);
  ~SearchResultAdRendererThrottle() override;

  SearchResultAdRendererThrottle(const SearchResultAdRendererThrottle&) =
      delete;
  SearchResultAdRendererThrottle& operator=(
      const SearchResultAdRendererThrottle&) = delete;

  // Implements blink::URLLoaderThrottle:
  void DetachFromCurrentSequence() override;
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  void OnMaybeTriggerAdViewedEvent(
      mojo::Remote<brave_ads::mojom::BraveAdsHost> brave_ads_remote,
      bool event_triggered);

  mojo::PendingRemote<brave_ads::mojom::BraveAdsHost> brave_ads_pending_remote_;
  std::string creative_instance_id_;
  base::WeakPtrFactory<SearchResultAdRendererThrottle> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_SEARCH_RESULT_AD_RENDERER_THROTTLE_H_
