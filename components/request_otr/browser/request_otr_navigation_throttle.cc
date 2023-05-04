/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_navigation_throttle.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/request_otr/browser/request_otr_controller_client.h"
#include "brave/components/request_otr/browser/request_otr_page.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "brave/components/request_otr/browser/request_otr_tab_storage.h"
#include "brave/components/request_otr/common/features.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "components/prefs/pref_service.h"
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
  if (!request_otr_service || !ephemeral_storage_service || !pref_service) {
    return nullptr;
  }

  // If 'request off-the-record' feature is disabled or ephemeral storage
  // is disabled, don't bother creating throttle.
  if (!base::FeatureList::IsEnabled(request_otr::features::kBraveRequestOTR)) {
    return nullptr;
  }
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage)) {
    return nullptr;
  }

  // Don't block subframes.
  if (!navigation_handle->IsInMainFrame()) {
    return nullptr;
  }

  // If user preference is 'never go off the record', don't bother creating
  // throttle.
  if (pref_service->GetInteger(prefs::kRequestOTRActionOption) ==
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
  RequestOTRTabStorage* tab_storage =
      RequestOTRTabStorage::GetOrCreate(web_contents);
  if (tab_storage->IsProceeding()) {
    return content::NavigationThrottle::PROCEED;
  }
  // If user has already indicated they want to go off-the-record,
  // don't show another interstitial. (Everything is already set up.)
  if (tab_storage->RequestedOTR()) {
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

  // If there is an RequestOTRTabStorage associated to |web_contents_|, clear
  // the IsProceeding flag.
  RequestOTRTabStorage* tab_storage = RequestOTRTabStorage::FromWebContents(
      navigation_handle()->GetWebContents());
  if (tab_storage) {
    tab_storage->SetIsProceeding(false);

    // If we have already offered to go off-the-record (i.e. shown our
    // interstitial), don't offer again.
    if (tab_storage->OfferedOTR()) {
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
  if (!headers->HasHeaderValue("X-Request-OTR", "1")) {
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

  if (pref_service_->GetInteger(prefs::kRequestOTRActionOption) ==
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
  RequestOTRTabStorage* tab_storage =
      RequestOTRTabStorage::GetOrCreate(web_contents);
  tab_storage->SetOfferedOTR(true);

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
  RequestOTRTabStorage* tab_storage = RequestOTRTabStorage::FromWebContents(
      navigation_handle()->GetWebContents());
  if (tab_storage) {
    tab_storage->MaybeEnable1PESForUrl(
        ephemeral_storage_service_, navigation_handle()->GetURL(),
        base::BindOnce(&RequestOTRNavigationThrottle::Resume,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

const char* RequestOTRNavigationThrottle::GetNameForLogging() {
  return "RequestOTRNavigationThrottle";
}

}  // namespace request_otr
