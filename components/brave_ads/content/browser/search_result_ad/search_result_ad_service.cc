/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"

#include <utility>

#include "base/callback.h"
#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_parsing.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace brave_ads {

SearchResultAdService::AdViewedEventCallbackInfo::AdViewedEventCallbackInfo() =
    default;
SearchResultAdService::AdViewedEventCallbackInfo::AdViewedEventCallbackInfo(
    AdViewedEventCallbackInfo&& info) = default;
SearchResultAdService::AdViewedEventCallbackInfo&
SearchResultAdService::AdViewedEventCallbackInfo::operator=(
    AdViewedEventCallbackInfo&& info) = default;
SearchResultAdService::AdViewedEventCallbackInfo::~AdViewedEventCallbackInfo() =
    default;

SearchResultAdService::SearchResultAdService(AdsService* ads_service)
    : ads_service_(ads_service) {
  DCHECK(ads_service_);
}

SearchResultAdService::~SearchResultAdService() = default;

void SearchResultAdService::MaybeRetrieveSearchResultAd(
    content::RenderFrameHost* render_frame_host,
    SessionID tab_id,
    bool should_trigger_viewed_event) {
  DCHECK(ads_service_);
  DCHECK(render_frame_host);
  DCHECK(tab_id.is_valid());

  if (!should_trigger_viewed_event || !ads_service_->IsEnabled() ||
      !base::FeatureList::IsEnabled(
          features::kSupportBraveSearchResultAdConfirmationEvents) ||
      !brave_search::IsAllowedHost(render_frame_host->GetLastCommittedURL())) {
    if (metadata_request_finished_callback_for_testing_) {
      std::move(metadata_request_finished_callback_for_testing_).Run();
    }
    search_result_ads_[tab_id] = SearchResultAdMap();
    RunAdViewedEventPendingCallbacks(tab_id, /* ads_fetched */ false);
    return;
  }

  mojo::Remote<blink::mojom::DocumentMetadata> document_metadata;
  render_frame_host->GetRemoteInterfaces()->GetInterface(
      document_metadata.BindNewPipeAndPassReceiver());
  DCHECK(document_metadata.is_bound());
  document_metadata.reset_on_disconnect();

  blink::mojom::DocumentMetadata* raw_document_metadata =
      document_metadata.get();
  raw_document_metadata->GetEntities(base::BindOnce(
      &SearchResultAdService::OnRetrieveSearchResultAdEntities,
      weak_factory_.GetWeakPtr(), std::move(document_metadata), tab_id));
}

void SearchResultAdService::OnDidFinishNavigation(SessionID tab_id) {
  // Clear the tab state from the previous load.
  ResetState(tab_id);
  // Now ad viewed events callbacks will be cached before search result JSON-ld
  // is loaded and processed.
  ad_viewed_event_pending_callbacks_[tab_id] =
      std::vector<AdViewedEventCallbackInfo>();
}

void SearchResultAdService::OnTabClosed(SessionID tab_id) {
  // Clear the tab state in memory.
  ResetState(tab_id);
}

void SearchResultAdService::MaybeTriggerSearchResultAdViewedEvent(
    const std::string& creative_instance_id,
    SessionID tab_id,
    base::OnceCallback<void(bool)> callback) {
  DCHECK(ads_service_);
  DCHECK(!creative_instance_id.empty());
  DCHECK(tab_id.is_valid());

  if (!ads_service_->IsEnabled()) {
    std::move(callback).Run(/* event_triggered */ false);
    return;
  }

  // Check if search result ad JSON-LD wasn't processed yet.
  if (!base::Contains(search_result_ads_, tab_id)) {
    // Check if OnDidFinishNavigation was called for tab_id.
    if (!base::Contains(ad_viewed_event_pending_callbacks_, tab_id)) {
      std::move(callback).Run(/* event_triggered */ false);
      return;
    }

    AdViewedEventCallbackInfo callback_info;
    callback_info.creative_instance_id = creative_instance_id;
    callback_info.callback = std::move(callback);
    ad_viewed_event_pending_callbacks_[tab_id].push_back(
        std::move(callback_info));
    return;
  }

  const bool event_triggered =
      QueueSearchResultAdViewedEvent(creative_instance_id, tab_id);
  std::move(callback).Run(event_triggered);
}

void SearchResultAdService::SetMetadataRequestFinishedCallbackForTesting(
    base::OnceClosure callback) {
  metadata_request_finished_callback_for_testing_ = std::move(callback);
}

AdsService* SearchResultAdService::SetAdsServiceForTesting(
    AdsService* ads_service) {
  AdsService* previous_ads_service = ads_service_.get();
  ads_service_ = ads_service;
  return previous_ads_service;
}

void SearchResultAdService::ResetState(SessionID tab_id) {
  DCHECK(tab_id.is_valid());

  for (auto& callback_info : ad_viewed_event_pending_callbacks_[tab_id]) {
    std::move(callback_info.callback).Run(false);
  }
  ad_viewed_event_pending_callbacks_.erase(tab_id);
  search_result_ads_.erase(tab_id);
}

void SearchResultAdService::OnRetrieveSearchResultAdEntities(
    mojo::Remote<blink::mojom::DocumentMetadata> document_metadata,
    SessionID tab_id,
    blink::mojom::WebPagePtr web_page) {
  if (metadata_request_finished_callback_for_testing_) {
    std::move(metadata_request_finished_callback_for_testing_).Run();
  }

  if (!web_page) {
    search_result_ads_[tab_id] = SearchResultAdMap();
    RunAdViewedEventPendingCallbacks(tab_id, /* ads_fetched */ false);
    return;
  }

  SearchResultAdMap search_result_ads =
      ParseWebPageEntities(std::move(web_page));

  search_result_ads_.emplace(tab_id, std::move(search_result_ads));

  RunAdViewedEventPendingCallbacks(tab_id, /* ads_fetched */ true);
}

void SearchResultAdService::RunAdViewedEventPendingCallbacks(SessionID tab_id,
                                                             bool ads_fetched) {
  for (auto& callback_info : ad_viewed_event_pending_callbacks_[tab_id]) {
    bool event_triggered = false;
    if (ads_fetched) {
      event_triggered = QueueSearchResultAdViewedEvent(
          callback_info.creative_instance_id, tab_id);
    }
    if (event_triggered) {
      VLOG(1) << "Triggered search result ad viewed event for "
              << callback_info.creative_instance_id;
    } else {
      VLOG(1) << "Failed to trigger search result ad viewed event for "
              << callback_info.creative_instance_id;
    }
    std::move(callback_info.callback).Run(event_triggered);
  }
  ad_viewed_event_pending_callbacks_.erase(tab_id);
}

bool SearchResultAdService::QueueSearchResultAdViewedEvent(
    const std::string& creative_instance_id,
    SessionID tab_id) {
  DCHECK(!creative_instance_id.empty());
  DCHECK(tab_id.is_valid());

  SearchResultAdMap& ad_map = search_result_ads_[tab_id];
  auto it = ad_map.find(creative_instance_id);
  if (it == ad_map.end()) {
    return false;
  }

  ad_viewed_event_queue_.push_front(std::move(it->second));
  ad_map.erase(creative_instance_id);
  TriggerSearchResultAdViewedEventFromQueue();

  return true;
}

void SearchResultAdService::TriggerSearchResultAdViewedEventFromQueue() {
  DCHECK(ads_service_);
  DCHECK(!ad_viewed_event_queue_.empty() ||
         !trigger_ad_viewed_event_in_progress_);

  if (ad_viewed_event_queue_.empty() || trigger_ad_viewed_event_in_progress_) {
    return;
  }
  trigger_ad_viewed_event_in_progress_ = true;

  ads::mojom::SearchResultAdPtr search_result_ad =
      std::move(ad_viewed_event_queue_.back());
  ad_viewed_event_queue_.pop_back();

  ads_service_->TriggerSearchResultAdEvent(
      std::move(search_result_ad), ads::mojom::SearchResultAdEventType::kViewed,
      base::BindOnce(&SearchResultAdService::OnTriggerSearchResultAdViewedEvent,
                     weak_factory_.GetWeakPtr()));
}

void SearchResultAdService::OnTriggerSearchResultAdViewedEvent(
    const bool success,
    const std::string& placement_id,
    ads::mojom::SearchResultAdEventType ad_event_type) {
  trigger_ad_viewed_event_in_progress_ = false;
  TriggerSearchResultAdViewedEventFromQueue();

  if (!success) {
    VLOG(1) << "Error during processing of search result ad event for "
            << placement_id;
  }
}

}  // namespace brave_ads
