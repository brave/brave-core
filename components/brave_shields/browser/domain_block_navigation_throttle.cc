/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/domain_block_navigation_throttle.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/domain_block_controller_client.h"
#include "brave/components/brave_shields/browser/domain_block_page.h"
#include "brave/components/brave_shields/browser/domain_block_tab_storage.h"
#include "brave/components/brave_shields/common/features.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "net/base/net_errors.h"

namespace {

bool ShouldBlockDomainOnTaskRunner(
    brave_shields::AdBlockService* ad_block_service,
    const GURL& url) {
  SCOPED_UMA_HISTOGRAM_TIMER("Brave.DomainBlock.ShouldBlock");
  bool did_match_exception = false;
  bool did_match_rule = false;
  bool did_match_important = false;
  std::string mock_data_url;
  // force aggressive blocking to `true` for domain blocking - these requests
  // are all "first-party", but the throttle is already only called when
  // necessary.
  bool aggressive_blocking = true;
  ad_block_service->ShouldStartRequest(
      url, blink::mojom::ResourceType::kMainFrame, url.host(),
      aggressive_blocking, &did_match_rule, &did_match_exception,
      &did_match_important, &mock_data_url);
  return (did_match_important || (did_match_rule && !did_match_exception));
}

}  // namespace

namespace brave_shields {

// static
std::unique_ptr<DomainBlockNavigationThrottle>
DomainBlockNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    AdBlockService* ad_block_service,
    AdBlockCustomFiltersService* ad_block_custom_filters_service,
    HostContentSettingsMap* content_settings,
    const std::string& locale) {
  if (!ad_block_service || !ad_block_custom_filters_service)
    return nullptr;
  if (!base::FeatureList::IsEnabled(brave_shields::features::kBraveDomainBlock))
    return nullptr;
  return std::make_unique<DomainBlockNavigationThrottle>(
      navigation_handle, ad_block_service, ad_block_custom_filters_service,
      content_settings, locale);
}

DomainBlockNavigationThrottle::DomainBlockNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    AdBlockService* ad_block_service,
    AdBlockCustomFiltersService* ad_block_custom_filters_service,
    HostContentSettingsMap* content_settings,
    const std::string& locale)
    : content::NavigationThrottle(navigation_handle),
      ad_block_service_(ad_block_service),
      ad_block_custom_filters_service_(ad_block_custom_filters_service),
      content_settings_(content_settings),
      locale_(locale) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

DomainBlockNavigationThrottle::~DomainBlockNavigationThrottle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

content::NavigationThrottle::ThrottleCheckResult
DomainBlockNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!ad_block_service_->IsInitialized())
    return content::NavigationThrottle::PROCEED;

  // Don't block subframes
  content::NavigationHandle* handle = navigation_handle();
  if (!handle->IsInMainFrame())
    return content::NavigationThrottle::PROCEED;

  GURL request_url = handle->GetURL();

  // Maybe don't block based on Brave Shields settings
  if (!brave_shields::ShouldDoDomainBlocking(content_settings_, request_url))
    return content::NavigationThrottle::PROCEED;

  // If user has just chosen to proceed on our interstitial, don't show
  // another one.
  content::WebContents* web_contents = handle->GetWebContents();
  DomainBlockTabStorage* tab_storage =
      DomainBlockTabStorage::GetOrCreate(web_contents);
  if (tab_storage->IsProceeding())
    return content::NavigationThrottle::PROCEED;

  // Otherwise, call the ad block service on a task runner to determine whether
  // this domain should be blocked.
  ad_block_service_->GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ShouldBlockDomainOnTaskRunner, ad_block_service_,
                     request_url),
      base::BindOnce(&DomainBlockNavigationThrottle::OnShouldBlockDomain,
                     weak_ptr_factory_.GetWeakPtr()));

  // Since the call to the ad block service is asynchronous, we defer the final
  // decision of whether to allow or block this navigation. The callback from
  // the task runner will call a method to give our final answer.
  return content::NavigationThrottle::DEFER;
}

content::NavigationThrottle::ThrottleCheckResult
DomainBlockNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

content::NavigationThrottle::ThrottleCheckResult
DomainBlockNavigationThrottle::WillProcessResponse() {
  // If there is an DomainBlockTabStorage associated to |web_contents_|, clear
  // the IsProceeding flag.
  DomainBlockTabStorage* tab_storage = DomainBlockTabStorage::FromWebContents(
      navigation_handle()->GetWebContents());
  if (tab_storage)
    tab_storage->SetIsProceeding(false);
  return content::NavigationThrottle::PROCEED;
}

void DomainBlockNavigationThrottle::OnShouldBlockDomain(
    bool should_block_domain) {
  if (should_block_domain) {
    ShowInterstitial();
  } else {
    // Navigation was deferred while we called the ad block service on a task
    // runner, but now we know that we want to allow navigation to continue.
    Resume();
  }
}

void DomainBlockNavigationThrottle::ShowInterstitial() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();
  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();
  PrefService* pref_service = user_prefs::UserPrefs::Get(context);

  // The controller client implements the actual logic to "go back" or "proceed"
  // from the interstitial.
  auto controller_client = std::make_unique<DomainBlockControllerClient>(
      web_contents, request_url, ad_block_custom_filters_service_, pref_service,
      locale_);

  // This handles populating the HTML template of the interstitial page with
  // localized strings and other information we only know at runtime,
  // including the URL of the page we're blocking. Once the user interacts with
  // the interstitial, this translates those actions into method calls on the
  // controller client.
  auto blocked_page = std::make_unique<DomainBlockPage>(
      web_contents, request_url, std::move(controller_client));

  // Get the page content before giving up ownership of |blocked_page|.
  std::string blocked_page_content = blocked_page->GetHTMLContents();

  // Replace the tab contents with our interstitial page.
  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      web_contents, handle->GetNavigationId(), std::move(blocked_page));

  // Navigation was deferred rather than canceled outright because the
  // call to the ad blocking service happens on a task runner, but now we
  // know that we definitely want to cancel the navigation.
  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      blocked_page_content));
}

const char* DomainBlockNavigationThrottle::GetNameForLogging() {
  return "DomainBlockNavigationThrottle";
}

}  // namespace brave_shields
