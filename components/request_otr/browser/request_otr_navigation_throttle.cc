/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_navigation_throttle.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/request_otr/browser/request_otr_blocking_page.h"
#include "brave/components/request_otr/browser/request_otr_controller_client.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"
#include "brave/components/request_otr/common/features.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/profile_metrics/browser_profile_type.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "net/base/features.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"

namespace request_otr {

// static
std::unique_ptr<RequestOTRNavigationThrottle>
RequestOTRNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    RequestOTRService* request_otr_service,
    ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
    PrefService* pref_service,
    const std::string& locale) {
  DCHECK(pref_service);

  // If 'request off-the-record' feature is disabled or ephemeral storage
  // is disabled, don't bother creating throttle.
  if (!base::FeatureList::IsEnabled(
          request_otr::features::kBraveRequestOTRTab)) {
    return nullptr;
  }
  DCHECK(request_otr_service);

  if (!base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage)) {
    return nullptr;
  }

  // If this is the system profile, then we don't need the throttle.
  if (profile_metrics::GetBrowserProfileType(
          navigation_handle->GetWebContents()->GetBrowserContext()) ==
      profile_metrics::BrowserProfileType::kSystem) {
    return nullptr;
  }
  DCHECK(ephemeral_storage_service);

  // Don't block subframes.
  if (!navigation_handle->IsInMainFrame()) {
    return nullptr;
  }

  // If user preference is 'never go off the record', don't bother creating
  // throttle.
  if (pref_service->GetInteger(kRequestOTRActionOption) ==
      static_cast<int>(RequestOTRService::RequestOTRActionOption::kNever)) {
    return nullptr;
  }

  return std::make_unique<RequestOTRNavigationThrottle>(
      navigation_handle, request_otr_service, ephemeral_storage_service,
      pref_service, locale);
}

RequestOTRNavigationThrottle::RequestOTRNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    RequestOTRService* request_otr_service,
    ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
    PrefService* pref_service,
    const std::string& locale)
    : content::NavigationThrottle(navigation_handle),
      request_otr_service_(request_otr_service),
      ephemeral_storage_service_(ephemeral_storage_service),
      pref_service_(pref_service),
      locale_(locale) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

RequestOTRNavigationThrottle::~RequestOTRNavigationThrottle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

content::NavigationThrottle::ThrottleCheckResult
RequestOTRNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  if (!web_contents || !handle->IsInMainFrame()) {
    return NavigationThrottle::PROCEED;
  }

  // If user has just chosen to proceed on our interstitial, don't show
  // another one.
  RequestOTRStorageTabHelper* tab_storage =
      RequestOTRStorageTabHelper::GetOrCreate(web_contents);
  if (tab_storage->is_proceeding()) {
    return content::NavigationThrottle::PROCEED;
  }
  // If user has already indicated they want to go off-the-record,
  // don't show another interstitial. (Everything is already set up.)
  if (tab_storage->has_requested_otr()) {
    return content::NavigationThrottle::PROCEED;
  }

  // Call the request OTR service to determine whether this domain should
  // present an interstitial.
  if (!request_otr_service_->ShouldBlock(handle->GetURL())) {
    return NavigationThrottle::PROCEED;
  }

  return MaybeShowInterstitial();
}

content::NavigationThrottle::ThrottleCheckResult
RequestOTRNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

content::NavigationThrottle::ThrottleCheckResult
RequestOTRNavigationThrottle::WillProcessResponse() {
  content::NavigationHandle* handle = navigation_handle();

  // Ignore same-document navigations.
  if (handle->IsSameDocument()) {
    return content::NavigationThrottle::PROCEED;
  }

  // Ignore network errors.
  if (handle->GetNetErrorCode() != net::OK) {
    return content::NavigationThrottle::PROCEED;
  }

  // If there is an RequestOTRStorageTabHelper associated to |web_contents_|,
  // clear the IsProceeding flag.
  RequestOTRStorageTabHelper* tab_storage =
      RequestOTRStorageTabHelper::FromWebContents(
          navigation_handle()->GetWebContents());
  if (tab_storage) {
    tab_storage->set_is_proceeding(false);

    // If we have already offered to go off-the-record (i.e. shown our
    // interstitial), don't offer again.
    if (tab_storage->has_offered_otr()) {
      return content::NavigationThrottle::PROCEED;
    }
  }

  // Get HTTP headers from this request.
  const net::HttpResponseHeaders* headers =
      navigation_handle()->GetResponseHeaders();
  if (!headers) {
    return content::NavigationThrottle::PROCEED;
  }

  // Check if this site sent an HTTP header indicating it wants to offer to go
  // off-the-record.
  if (!headers->HasHeaderValue("Request-OTR", "1")) {
    return content::NavigationThrottle::PROCEED;
  }

  return MaybeShowInterstitial();
}

content::NavigationThrottle::ThrottleCheckResult
RequestOTRNavigationThrottle::MaybeShowInterstitial() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();

  // The controller client implements the actual logic to "go back" or "proceed"
  // from the interstitial.
  auto controller_client = std::make_unique<RequestOTRControllerClient>(
      web_contents, request_url, ephemeral_storage_service_, pref_service_,
      locale_);

  if (pref_service_->GetInteger(kRequestOTRActionOption) ==
      static_cast<int>(RequestOTRService::RequestOTRActionOption::kAlways)) {
    controller_client->ProceedOTR();
    return content::NavigationThrottle::PROCEED;
  }

  // This handles populating the HTML template of the interstitial page with
  // localized strings and other information we only know at runtime,
  // including the URL of the page we're blocking. Once the user interacts with
  // the interstitial, this translates those actions into method calls on the
  // controller client.
  auto blocked_page = std::make_unique<RequestOTRBlockingPage>(
      web_contents, request_url, std::move(controller_client));

  // Get the page content before giving up ownership of |blocked_page|.
  std::string blocked_page_content = blocked_page->GetHTMLContents();

  // Record (in memory) that we have shown this interstitial.
  RequestOTRStorageTabHelper* tab_storage =
      RequestOTRStorageTabHelper::GetOrCreate(web_contents);
  tab_storage->set_offered_otr(true);

  // Replace the tab contents with our interstitial page.
  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      handle, std::move(blocked_page));

  // Return cancel result
  return content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      blocked_page_content);
}

void RequestOTRNavigationThrottle::Enable1PESAndResume() {
  DCHECK(ephemeral_storage_service_);
  RequestOTRStorageTabHelper* tab_storage =
      RequestOTRStorageTabHelper::FromWebContents(
          navigation_handle()->GetWebContents());
  if (tab_storage) {
    tab_storage->MaybeEnable1PESForUrl(
        ephemeral_storage_service_, navigation_handle()->GetURL(),
        base::BindOnce(&RequestOTRNavigationThrottle::On1PESState,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void RequestOTRNavigationThrottle::On1PESState(bool is_1pes_enabled) {
  if (is_1pes_enabled) {
    RestartNavigation(navigation_handle()->GetURL());
  } else {
    Resume();
  }
}

void RequestOTRNavigationThrottle::RestartNavigation(const GURL& url) {
  content::NavigationHandle* handle = navigation_handle();

  content::OpenURLParams params =
      content::OpenURLParams::FromNavigationHandle(handle);

  content::WebContents* contents = handle->GetWebContents();

  params.url = url;
  params.transition = static_cast<ui::PageTransition>(
      params.transition | ui::PAGE_TRANSITION_CLIENT_REDIRECT);
  // We get a DCHECK here if we don't clear the redirect chain because
  // technically this is a new navigation
  params.redirect_chain.clear();

  // Cancel without an error status to surface any real errors during page
  // load.
  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL));

  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> web_contents,
                        const content::OpenURLParams& params) {
                       if (!web_contents) {
                         return;
                       }
                       web_contents->OpenURL(params,
                                             /*navigation_handle_callback=*/{});
                     },
                     contents->GetWeakPtr(), std::move(params)));
}

const char* RequestOTRNavigationThrottle::GetNameForLogging() {
  return "RequestOTRNavigationThrottle";
}

}  // namespace request_otr
