/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/search_result_ad/search_result_ad_redirect_throttle.h"

#include <utility>

#include "brave/browser/brave_ads/search_result_ad/search_result_ad_tab_helper.h"
#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_util.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "services/network/public/cpp/request_mode.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace brave_ads {

// static
std::unique_ptr<SearchResultAdRedirectThrottle>
SearchResultAdRedirectThrottle::MaybeCreateThrottleFor(
    const network::ResourceRequest& request,
    const content::WebContents::Getter& wc_getter) {
  if (!request.request_initiator ||
      !ui::PageTransitionCoreTypeIs(
          static_cast<ui::PageTransition>(request.transition_type),
          ui::PAGE_TRANSITION_LINK) ||
      request.resource_type !=
          static_cast<int>(blink::mojom::ResourceType::kMainFrame) ||
      request.method != net::HttpRequestHeaders::kGetMethod) {
    return nullptr;
  }

  if (!brave_search::IsAllowedHost(request.request_initiator->GetURL()) ||
      !IsSearchResultAdClickedConfirmationUrl(request.url)) {
    return nullptr;
  }

  return std::make_unique<SearchResultAdRedirectThrottle>(wc_getter);
}

SearchResultAdRedirectThrottle::SearchResultAdRedirectThrottle(
    const content::WebContents::Getter& wc_getter)
    : wc_getter_(wc_getter) {
  DCHECK(wc_getter_);
}

SearchResultAdRedirectThrottle::~SearchResultAdRedirectThrottle() = default;

void SearchResultAdRedirectThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  DCHECK(request);
  DCHECK(request->request_initiator);
  DCHECK_EQ(request->resource_type,
            static_cast<int>(blink::mojom::ResourceType::kMainFrame));
  DCHECK_EQ(request->method, net::HttpRequestHeaders::kGetMethod);
  DCHECK(brave_search::IsAllowedHost(request->request_initiator->GetURL()));

  const std::string creative_instance_id =
      GetClickedSearchResultAdCreativeInstanceId(request->url);
  if (creative_instance_id.empty()) {
    return;
  }

  content::WebContents* web_contents = wc_getter_.Run();
  if (!web_contents) {
    return;
  }

  content::WebContents* original_web_contents =
      web_contents->GetFirstWebContentsInLiveOriginalOpenerChain();
  if (original_web_contents) {
    web_contents = original_web_contents;
  }

  SearchResultAdTabHelper* search_result_ad_tab_helper =
      SearchResultAdTabHelper::FromWebContents(web_contents);
  if (!search_result_ad_tab_helper) {
    return;
  }

  const absl::optional<GURL> search_result_ad_target_url =
      search_result_ad_tab_helper->MaybeTriggerSearchResultAdClickedEvent(
          creative_instance_id);
  if (!search_result_ad_target_url) {
    return;
  }
  DCHECK(search_result_ad_target_url->is_valid());
  DCHECK(search_result_ad_target_url->SchemeIs(url::kHttpsScheme));

  request->url = *search_result_ad_target_url;
}

}  // namespace brave_ads
