/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_handler.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_parsing.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace brave_ads {

SearchResultAdHandler::SearchResultAdHandler(AdsService* ads_service,
                                             bool should_trigger_viewed_event)
    : ads_service_(ads_service),
      should_trigger_viewed_event_(should_trigger_viewed_event) {
  DCHECK(ads_service_);
}

SearchResultAdHandler::~SearchResultAdHandler() = default;

// static
std::unique_ptr<SearchResultAdHandler>
SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
    AdsService* ads_service,
    const GURL& url,
    bool should_trigger_viewed_event) {
  if (!ads_service || !ads_service->IsEnabled() ||
      !base::FeatureList::IsEnabled(
          features::kSupportBraveSearchResultAdConfirmationEvents) ||
      !brave_search::IsAllowedHost(url)) {
    return {};
  }

  return base::WrapUnique(
      new SearchResultAdHandler(ads_service, should_trigger_viewed_event));
}

void SearchResultAdHandler::MaybeRetrieveSearchResultAd(
    content::RenderFrameHost* render_frame_host) {
  DCHECK(render_frame_host);
  DCHECK(ads_service_);

  if (!ads_service_->IsEnabled()) {
    return;
  }

  mojo::Remote<blink::mojom::DocumentMetadata> document_metadata;
  render_frame_host->GetRemoteInterfaces()->GetInterface(
      document_metadata.BindNewPipeAndPassReceiver());
  DCHECK(document_metadata.is_bound());
  document_metadata.reset_on_disconnect();

  blink::mojom::DocumentMetadata* raw_document_metadata =
      document_metadata.get();
  raw_document_metadata->GetEntities(
      base::BindOnce(&SearchResultAdHandler::OnRetrieveSearchResultAdEntities,
                     weak_factory_.GetWeakPtr(), std::move(document_metadata)));
}

absl::optional<GURL>
SearchResultAdHandler::MaybeTriggerSearchResultAdClickedEvent(
    const std::string& creative_instance_id) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(ads_service_);

  if (!ads_service_->IsEnabled() || !search_result_ads_) {
    return absl::nullopt;
  }

  auto it = search_result_ads_->find(creative_instance_id);
  if (it == search_result_ads_->end()) {
    return absl::nullopt;
  }

  const ads::mojom::SearchResultAdInfoPtr& search_result_ad = it->second;
  DCHECK(search_result_ad);
  DCHECK(search_result_ad->target_url.is_valid() &&
         search_result_ad->target_url.SchemeIs(url::kHttpsScheme));

  ads_service_->TriggerSearchResultAdEvent(
      search_result_ad->Clone(), ads::mojom::SearchResultAdEventType::kClicked);

  return search_result_ad->target_url;
}

void SearchResultAdHandler::OnRetrieveSearchResultAdEntities(
    mojo::Remote<blink::mojom::DocumentMetadata> document_metadata,
    blink::mojom::WebPagePtr web_page) {
  if (!web_page) {
    return;
  }

  search_result_ads_ = ParseWebPageEntities(std::move(web_page));

  if (search_result_ads_ && should_trigger_viewed_event_) {
    for (const auto& [key, value] : *search_result_ads_) {
      TriggerSearchResultAdViewedEvent(key);
    }
  }
}

void SearchResultAdHandler::TriggerSearchResultAdViewedEvent(
    const std::string& creative_instance_id) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(search_result_ads_);
  DCHECK(ads_service_);

  if (!ads_service_->IsEnabled()) {
    return;
  }

  auto it = search_result_ads_->find(creative_instance_id);
  if (it == search_result_ads_->end()) {
    return;
  }

  const ads::mojom::SearchResultAdInfoPtr& search_result_ad = it->second;
  DCHECK(search_result_ad);

  ads_service_->TriggerSearchResultAdEvent(
      search_result_ad->Clone(), ads::mojom::SearchResultAdEventType::kViewed);

  return;
}

}  // namespace brave_ads
