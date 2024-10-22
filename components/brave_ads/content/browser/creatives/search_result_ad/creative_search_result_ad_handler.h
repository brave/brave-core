/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_HANDLER_H_

#include <memory>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_extractor.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom-forward.h"

class GURL;

namespace content {
class RenderFrameHost;
}  // namespace content

namespace brave_ads {

using ExtractCreativeAdPlacementIdsFromWebPageCallback =
    base::OnceCallback<void(std::vector<mojom::CreativeSearchResultAdInfoPtr>
                                creative_search_result_ads)>;

class AdsService;

class CreativeSearchResultAdHandler final {
 public:
  ~CreativeSearchResultAdHandler();

  CreativeSearchResultAdHandler(const CreativeSearchResultAdHandler&) = delete;
  CreativeSearchResultAdHandler& operator=(
      const CreativeSearchResultAdHandler&) = delete;
  CreativeSearchResultAdHandler(CreativeSearchResultAdHandler&&) noexcept =
      delete;
  CreativeSearchResultAdHandler& operator=(
      CreativeSearchResultAdHandler&&) noexcept = delete;

  static std::unique_ptr<CreativeSearchResultAdHandler> MaybeCreate(
      AdsService* ads_service,
      const GURL& url,
      bool should_trigger_creative_ad_viewed_events);

  void MaybeExtractCreativeAdPlacementIdsFromWebPage(
      content::RenderFrameHost* render_frame_host,
      ExtractCreativeAdPlacementIdsFromWebPageCallback callback);

  void MaybeTriggerCreativeAdViewedEvent(
      mojom::CreativeSearchResultAdInfoPtr creative_search_result_ad);

 private:
  friend class BraveAdsCreativeSearchResultAdHandlerTest;

  CreativeSearchResultAdHandler(AdsService* ads_service,
                                bool should_trigger_creative_ad_viewed_events);

  void MaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      mojo::Remote<blink::mojom::DocumentMetadata>
          mojom_document_metadata_remote,
      ExtractCreativeAdPlacementIdsFromWebPageCallback callback,
      blink::mojom::WebPagePtr mojom_web_page);

  const raw_ptr<AdsService> ads_service_;  // NOT OWNED

  const bool should_trigger_creative_ad_viewed_events_;

  base::WeakPtrFactory<CreativeSearchResultAdHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_HANDLER_H_
