/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/search_result_ad_handler.h"

#include <iterator>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/search_result_ad_converting_util.h"
#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/search_result_ad_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "url/gurl.h"

namespace brave_ads {

SearchResultAdHandler::SearchResultAdHandler(
    AdsService* ads_service,
    const bool should_trigger_viewed_event)
    : ads_service_(ads_service),
      should_trigger_viewed_event_(should_trigger_viewed_event) {
  CHECK(ads_service_);
}

SearchResultAdHandler::~SearchResultAdHandler() = default;

// static
std::unique_ptr<SearchResultAdHandler>
SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
    AdsService* ads_service,
    const GURL& url,
    const bool should_trigger_viewed_event) {
  if (!ads_service || !brave_search::IsAllowedHost(url)) {
    return nullptr;
  }

  return base::WrapUnique(
      new SearchResultAdHandler(ads_service, should_trigger_viewed_event));
}

void SearchResultAdHandler::MaybeRetrieveSearchResultAd(
    content::RenderFrameHost* render_frame_host,
    base::OnceCallback<void(std::vector<std::string> placement_ids)> callback) {
  CHECK(render_frame_host);
  CHECK(ads_service_);

  mojo::Remote<blink::mojom::DocumentMetadata> document_metadata;
  render_frame_host->GetRemoteInterfaces()->GetInterface(
      document_metadata.BindNewPipeAndPassReceiver());
  CHECK(document_metadata.is_bound());

  blink::mojom::DocumentMetadata* raw_document_metadata =
      document_metadata.get();
  CHECK(raw_document_metadata);
  raw_document_metadata->GetEntities(
      base::BindOnce(&SearchResultAdHandler::OnRetrieveSearchResultAdEntities,
                     weak_factory_.GetWeakPtr(), std::move(document_metadata),
                     std::move(callback)));
}

void SearchResultAdHandler::MaybeTriggerSearchResultAdClickedEvent(
    const GURL& navigation_url) {
  CHECK(ads_service_);

  if (!search_result_ads_) {
    return;
  }

  const std::optional<std::string> placement_id =
      GetPlacementIdFromSearchResultAdClickedUrl(navigation_url);
  if (!placement_id || placement_id->empty()) {
    return;
  }

  const auto iter = search_result_ads_->find(*placement_id);
  if (iter == search_result_ads_->cend()) {
    return;
  }
  const auto& [_, search_result_ad] = *iter;

  if (!search_result_ad) {
    return;
  }

  ads_service_->TriggerSearchResultAdEvent(
      search_result_ad->Clone(), mojom::SearchResultAdEventType::kClicked,
      /*intentional*/ base::DoNothing());
}

void SearchResultAdHandler::OnRetrieveSearchResultAdEntities(
    mojo::Remote<blink::mojom::DocumentMetadata> /*document_metadata*/,
    base::OnceCallback<void(std::vector<std::string> placement_ids)> callback,
    blink::mojom::WebPagePtr web_page) {
  CHECK(ads_service_);

  if (!web_page) {
    return std::move(callback).Run(/*placement_ids*/ {});
  }

  search_result_ads_ =
      ConvertWebPageEntitiesToSearchResultAds(web_page->entities);

  std::vector<std::string> placement_ids;
  if (search_result_ads_ && should_trigger_viewed_event_) {
    base::ranges::transform(
        *search_result_ads_, std::back_inserter(placement_ids),
        [](const auto& search_result_ad) { return search_result_ad.first; });
  }

  std::move(callback).Run(std::move(placement_ids));
}

void SearchResultAdHandler::MaybeTriggerSearchResultAdViewedEvent(
    const std::string& placement_id) {
  if (!search_result_ads_ || placement_id.empty()) {
    return;
  }

  const auto iter = search_result_ads_->find(placement_id);
  if (iter == search_result_ads_->cend()) {
    return;
  }
  const auto& [_, search_result_ad] = *iter;

  if (!search_result_ad) {
    return;
  }

  ads_service_->TriggerSearchResultAdEvent(
      search_result_ad->Clone(),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*intentional*/ base::DoNothing());
}

}  // namespace brave_ads
