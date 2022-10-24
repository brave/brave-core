/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/search_result_ad/search_result_ad_navigation_throttle.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/browser/brave_ads/search_result_ad/search_result_ad_tab_helper.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_util.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/origin.h"

namespace brave_ads {

// static
std::unique_ptr<SearchResultAdNavigationThrottle>
SearchResultAdNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* handle) {
  DCHECK(handle);

  content::WebContents* web_contents = handle->GetWebContents();
  if (!web_contents || !web_contents->GetBrowserContext() ||
      web_contents->GetBrowserContext()->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(
          features::kSupportBraveSearchResultAdConfirmationEvents)) {
    return {};
  }
  return base::WrapUnique(new SearchResultAdNavigationThrottle(handle));
}

SearchResultAdNavigationThrottle::SearchResultAdNavigationThrottle(
    content::NavigationHandle* handle)
    : content::NavigationThrottle(handle) {}

SearchResultAdNavigationThrottle::~SearchResultAdNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
SearchResultAdNavigationThrottle::WillStartRequest() {
  const auto& initiator_origin = navigation_handle()->GetInitiatorOrigin();
  if (!navigation_handle()->IsInPrimaryMainFrame() ||
      !ui::PageTransitionCoreTypeIs(navigation_handle()->GetPageTransition(),
                                    ui::PAGE_TRANSITION_LINK) ||
      !IsSearchResultAdClickedConfirmationUrl(navigation_handle()->GetURL()) ||
      !initiator_origin ||
      !brave_search::IsAllowedHost(initiator_origin->GetURL())) {
    return content::NavigationThrottle::PROCEED;
  }

  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents) {
    return content::NavigationThrottle::PROCEED;
  }

  const absl::optional<GURL> search_result_ad_target_url =
      GetSearchResultAdTargetUrl(web_contents, navigation_handle()->GetURL());
  if (!search_result_ad_target_url) {
    return content::NavigationThrottle::PROCEED;
  }
  DCHECK(search_result_ad_target_url->is_valid());
  DCHECK(search_result_ad_target_url->SchemeIs(url::kHttpsScheme));

  LoadSearchResultAdTargetUrl(web_contents, *search_result_ad_target_url);

  return content::NavigationThrottle::CANCEL;
}

const char* SearchResultAdNavigationThrottle::GetNameForLogging() {
  return "SearchResultAdNavigationThrottle";
}

absl::optional<GURL>
SearchResultAdNavigationThrottle::GetSearchResultAdTargetUrl(
    content::WebContents* web_contents,
    const GURL& navigation_url) const {
  DCHECK(web_contents);

  const std::string creative_instance_id =
      GetClickedSearchResultAdCreativeInstanceId(navigation_url);
  if (creative_instance_id.empty()) {
    return absl::nullopt;
  }

  content::WebContents* search_result_ad_web_contents = web_contents;
  if (content::WebContents* original_web_contents =
          web_contents->GetFirstWebContentsInLiveOriginalOpenerChain()) {
    search_result_ad_web_contents = original_web_contents;
  }

  SearchResultAdTabHelper* search_result_ad_tab_helper =
      SearchResultAdTabHelper::FromWebContents(search_result_ad_web_contents);
  if (!search_result_ad_tab_helper) {
    return absl::nullopt;
  }

  return search_result_ad_tab_helper->MaybeTriggerSearchResultAdClickedEvent(
      creative_instance_id);
}

void SearchResultAdNavigationThrottle::LoadSearchResultAdTargetUrl(
    content::WebContents* web_contents,
    const GURL& search_result_ad_target_url) const {
  DCHECK(web_contents);

  content::OpenURLParams params =
      content::OpenURLParams::FromNavigationHandle(navigation_handle());
  params.url = search_result_ad_target_url;
  params.transition = ui::PAGE_TRANSITION_CLIENT_REDIRECT;

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> web_contents,
                        const content::OpenURLParams& params) {
                       if (!web_contents)
                         return;
                       web_contents->OpenURL(params);
                     },
                     web_contents->GetWeakPtr(), std::move(params)));
}

}  // namespace brave_ads
