/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_handler.h"

#include <iterator>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_extractor.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_url_placement_id_extractor.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

CreativeSearchResultAdHandler::CreativeSearchResultAdHandler(
    AdsService* ads_service,
    const bool should_trigger_creative_ad_viewed_events)
    : ads_service_(ads_service),
      should_trigger_creative_ad_viewed_events_(
          should_trigger_creative_ad_viewed_events) {
  CHECK(ads_service);
}

CreativeSearchResultAdHandler::~CreativeSearchResultAdHandler() = default;

// static
std::unique_ptr<CreativeSearchResultAdHandler>
CreativeSearchResultAdHandler::MaybeCreate(
    AdsService* ads_service,
    const GURL& url,
    const bool should_trigger_creative_ad_viewed_events) {
  if (!ads_service) {
    return nullptr;
  }

  if (!brave_search::IsAllowedHost(url)) {
    return nullptr;
  }

  return base::WrapUnique(new CreativeSearchResultAdHandler(
      ads_service, should_trigger_creative_ad_viewed_events));
}

void CreativeSearchResultAdHandler::
    MaybeExtractCreativeAdPlacementIdsFromWebPage(
        content::RenderFrameHost* render_frame_host,
        ExtractCreativeAdPlacementIdsFromWebPageCallback callback) {
  CHECK(render_frame_host);

  mojo::Remote<blink::mojom::DocumentMetadata> mojom_document_metadata_remote;
  render_frame_host->GetRemoteInterfaces()->GetInterface(
      mojom_document_metadata_remote.BindNewPipeAndPassReceiver());
  CHECK(mojom_document_metadata_remote.is_bound());

  blink::mojom::DocumentMetadata* const raw_mojom_document_metadata =
      mojom_document_metadata_remote.get();
  CHECK(raw_mojom_document_metadata);

  raw_mojom_document_metadata->GetEntities(base::BindOnce(
      &CreativeSearchResultAdHandler::
          MaybeExtractCreativeAdPlacementIdsFromWebPageCallback,
      weak_factory_.GetWeakPtr(), std::move(mojom_document_metadata_remote),
      std::move(callback)));
}

void CreativeSearchResultAdHandler::MaybeTriggerCreativeAdClickedEvent(
    const GURL& url) {
  if (!creative_search_result_ads_) {
    // No creative search result ads are present on the web page.
    return;
  }

  const std::optional<std::string> placement_id =
      MaybeExtractCreativeAdPlacementIdFromUrl(url);
  if (!placement_id || placement_id->empty()) {
    // The URL does not contain a placement id.
    return;
  }

  const auto iter = creative_search_result_ads_->find(*placement_id);
  if (iter == creative_search_result_ads_->cend()) {
    // The placement id does not match any creative search result ad.
    return;
  }
  const auto& [_, creative_search_result_ad] = *iter;
  CHECK(creative_search_result_ad);

  ads_service_->TriggerSearchResultAdEvent(
      creative_search_result_ad->Clone(),
      mojom::SearchResultAdEventType::kClicked,
      /*intentional*/ base::DoNothing());
}

///////////////////////////////////////////////////////////////////////////////

void CreativeSearchResultAdHandler::
    MaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
        mojo::Remote<
            blink::mojom::DocumentMetadata> /*mojom_document_metadata_remote*/,
        ExtractCreativeAdPlacementIdsFromWebPageCallback callback,
        blink::mojom::WebPagePtr mojom_web_page) {
  if (!mojom_web_page) {
    return std::move(callback).Run(/*placement_ids=*/{});
  }

  creative_search_result_ads_ =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page->entities);
  if (!creative_search_result_ads_) {
    return std::move(callback).Run(/*placement_ids=*/{});
  }

  std::vector<std::string> placement_ids;
  base::ranges::transform(*creative_search_result_ads_,
                          std::back_inserter(placement_ids),
                          [](const auto& creative_search_result_ad) {
                            return creative_search_result_ad.first;
                          });

  std::move(callback).Run(std::move(placement_ids));
}

void CreativeSearchResultAdHandler::MaybeTriggerCreativeAdViewedEvent(
    const std::string& placement_id) {
  CHECK(!placement_id.empty());

  if (!should_trigger_creative_ad_viewed_events_) {
    return;
  }

  if (!creative_search_result_ads_) {
    // No creative search result ads are present on the web page.
    return;
  }

  const auto iter = creative_search_result_ads_->find(placement_id);
  if (iter == creative_search_result_ads_->cend()) {
    // The placement id does not match any creative search result ad.
    return;
  }
  const auto& [_, creative_search_result_ad] = *iter;
  CHECK(creative_search_result_ad);

  ads_service_->TriggerSearchResultAdEvent(
      creative_search_result_ad->Clone(),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*intentional*/ base::DoNothing());
}

}  // namespace brave_ads
