/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom.h"

namespace content {
class RenderFrameHost;
}

namespace brave_ads {

class AdsService;

// Retrieves search result ads from page and handles viewed/clicked events.
class SearchResultAdService : public KeyedService {
 public:
  explicit SearchResultAdService(AdsService* ads_service);
  ~SearchResultAdService() override;

  SearchResultAdService(const SearchResultAdService&) = delete;
  SearchResultAdService& operator=(const SearchResultAdService&) = delete;

  // Retrieves search result ads from the render frame.
  // If should_trigger_viewed_event value is false, then viewed
  // events shouldn't be sent to the ads library.
  void MaybeRetrieveSearchResultAd(content::RenderFrameHost* render_frame_host,
                                   SessionID tab_id,
                                   bool should_trigger_viewed_event);

  // Removes search result ads from the previous page load.
  void OnDidFinishNavigation(SessionID tab_id);

  // Removes search result ads when closing the tab.
  void OnTabClosed(SessionID tab_id);

  // Triggers a search result ad viewed event on a specific tab.
  void MaybeTriggerSearchResultAdViewedEvent(
      const std::string& creative_instance_id,
      SessionID tab_id,
      base::OnceCallback<void(bool)> callback);

  void SetMetadataRequestFinishedCallbackForTesting(base::OnceClosure callback);

  AdsService* SetAdsServiceForTesting(AdsService* ads_service);

 private:
  struct AdViewedEventCallbackInfo {
    AdViewedEventCallbackInfo();
    AdViewedEventCallbackInfo(AdViewedEventCallbackInfo&& info);
    AdViewedEventCallbackInfo& operator=(AdViewedEventCallbackInfo&& info);
    ~AdViewedEventCallbackInfo();

    std::string creative_instance_id;
    base::OnceCallback<void(bool)> callback;
  };

  void ResetState(SessionID tab_id);

  void OnRetrieveSearchResultAdEntities(
      mojo::Remote<blink::mojom::DocumentMetadata> document_metadata,
      SessionID tab_id,
      blink::mojom::WebPagePtr web_page);

  void RunAdViewedEventPendingCallbacks(SessionID tab_id, bool ads_fetched);

  bool QueueSearchResultAdViewedEvent(const std::string& creative_instance_id,
                                      SessionID tab_id);

  void TriggerSearchResultAdViewedEventFromQueue();

  void OnTriggerSearchResultAdViewedEvent(
      bool success,
      const std::string& placement_id,
      ads::mojom::SearchResultAdEventType ad_event_type);

  raw_ptr<AdsService> ads_service_ = nullptr;

  std::map<SessionID, std::map<std::string, ads::mojom::SearchResultAdInfoPtr>>
      search_result_ads_;

  std::map<SessionID, std::vector<AdViewedEventCallbackInfo>>
      ad_viewed_event_pending_callbacks_;

  base::circular_deque<ads::mojom::SearchResultAdInfoPtr>
      ad_viewed_event_queue_;

  bool trigger_ad_viewed_event_in_progress_ = false;

  base::OnceClosure metadata_request_finished_callback_for_testing_;

  base::WeakPtrFactory<SearchResultAdService> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_H_
