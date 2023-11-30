/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_HANDLER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom.h"

class GURL;

namespace content {
class RenderFrameHost;
}

namespace brave_ads {

class AdsService;

class SearchResultAdHandler final {
 public:
  ~SearchResultAdHandler();

  SearchResultAdHandler(const SearchResultAdHandler&) = delete;
  SearchResultAdHandler& operator=(const SearchResultAdHandler&) = delete;
  SearchResultAdHandler(SearchResultAdHandler&&) noexcept = delete;
  SearchResultAdHandler& operator=(SearchResultAdHandler&&) noexcept = delete;

  static std::unique_ptr<SearchResultAdHandler>
  MaybeCreateSearchResultAdHandler(AdsService* ads_service,
                                   const GURL& url,
                                   bool should_trigger_viewed_event);

  void MaybeRetrieveSearchResultAd(
      content::RenderFrameHost* render_frame_host,
      base::OnceCallback<void(std::vector<std::string>)> callback);

  void MaybeTriggerSearchResultAdViewedEvent(const std::string& placement_id);
  void MaybeTriggerSearchResultAdClickedEvent(const GURL& navigation_url);

 private:
  friend class SearchResultAdHandlerTest;

  SearchResultAdHandler(AdsService* ads_service,
                        bool should_trigger_viewed_event);

  void OnRetrieveSearchResultAdEntities(
      mojo::Remote<blink::mojom::DocumentMetadata> document_metadata,
      base::OnceCallback<void(std::vector<std::string>)> callback,
      blink::mojom::WebPagePtr web_page);

  raw_ptr<AdsService> ads_service_ = nullptr;  // NOT OWNED
  bool should_trigger_viewed_event_ = true;

  std::optional<base::flat_map</*placement_id*/ std::string,
                               mojom::SearchResultAdInfoPtr>>
      search_result_ads_;

  base::WeakPtrFactory<SearchResultAdHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_HANDLER_H_
