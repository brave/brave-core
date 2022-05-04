/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom.h"

namespace content {
class RenderFrameHost;
}

namespace brave_ads {

class AdsService;

using SearchResultAdsList = std::vector<ads::mojom::SearchResultAdPtr>;

class SearchResultAdService : public KeyedService {
 public:
  explicit SearchResultAdService(AdsService* ads_service);
  ~SearchResultAdService() override;

  SearchResultAdService(const SearchResultAdService&) = delete;
  SearchResultAdService& operator=(const SearchResultAdService&) = delete;

  void MaybeRetrieveSearchResultAd(content::RenderFrameHost* render_frame_host);

  void SetMetadataRequestFinishedCallbackForTesting(base::OnceClosure callback);

  AdsService* SetAdsServiceForTesting(AdsService* ads_service);

 private:
  void OnRetrieveSearchResultAdEntities(
      mojo::Remote<blink::mojom::DocumentMetadata> document_metadata,
      blink::mojom::WebPagePtr web_page);

  void TriggerSearchResultAdViewedEvents(SearchResultAdsList search_result_ads);

  void OnTriggerSearchResultAdViewedEvents(
      SearchResultAdsList search_result_ads,
      bool success,
      const std::string& placement_id,
      ads::mojom::SearchResultAdEventType ad_event_type);

  raw_ptr<AdsService> ads_service_ = nullptr;

  base::OnceClosure metadata_request_finished_callback_for_testing_;

  base::WeakPtrFactory<SearchResultAdService> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_SERVICE_H_
