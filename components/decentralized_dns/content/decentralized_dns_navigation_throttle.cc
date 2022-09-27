/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/content/decentralized_dns_navigation_throttle.h"

#include <utility>

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/decentralized_dns/content/decentralized_dns_interstitial_controller_client.h"
#include "brave/components/decentralized_dns/content/decentralized_dns_opt_in_page.h"
#include "brave/components/decentralized_dns/content/ens_offchain_lookup_interstitial_controller_client.h"
#include "brave/components/decentralized_dns/content/ens_offchain_lookup_opt_in_page.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"

namespace decentralized_dns {

// static
std::unique_ptr<DecentralizedDnsNavigationThrottle>
DecentralizedDnsNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    PrefService* local_state,
    const std::string& locale) {
  content::BrowserContext* context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  if (context->IsOffTheRecord())
    return nullptr;

  return std::make_unique<DecentralizedDnsNavigationThrottle>(
      navigation_handle, local_state, locale);
}

DecentralizedDnsNavigationThrottle::DecentralizedDnsNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    PrefService* local_state,
    const std::string& locale)
    : content::NavigationThrottle(navigation_handle),
      local_state_(local_state),
      locale_(locale) {
  content::BrowserContext* context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  user_prefs_ = user_prefs::UserPrefs::Get(context);
}

DecentralizedDnsNavigationThrottle::~DecentralizedDnsNavigationThrottle() =
    default;

content::NavigationThrottle::ThrottleCheckResult
DecentralizedDnsNavigationThrottle::WillStartRequest() {
  GURL url = navigation_handle()->GetURL();
  if ((IsUnstoppableDomainsTLD(url) &&
       IsUnstoppableDomainsResolveMethodAsk(local_state_)) ||
      (IsENSTLD(url) && IsENSResolveMethodAsk(local_state_))) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&DecentralizedDnsNavigationThrottle::ShowInterstitial,
                       weak_ptr_factory_.GetWeakPtr()));
    return content::NavigationThrottle::DEFER;
  }

  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
DecentralizedDnsNavigationThrottle::WillFailRequest() {
  auto* handle = navigation_handle();
  if (handle &&
      handle->GetNetErrorCode() == net::ERR_ENS_OFFCHAIN_LOOKUP_NOT_SELECTED) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&DecentralizedDnsNavigationThrottle::
                                      ShowEnsOffchainLookupInterstitial,
                                  weak_ptr_factory_.GetWeakPtr()));
    return content::NavigationThrottle::DEFER;
  }
  return content::NavigationThrottle::PROCEED;
}

void DecentralizedDnsNavigationThrottle::ShowInterstitial() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();

  auto controller_client =
      std::make_unique<DecentralizedDnsInterstitialControllerClient>(
          web_contents, request_url, user_prefs_, local_state_, locale_);
  auto page = std::make_unique<DecentralizedDnsOptInPage>(
      web_contents, handle->GetURL(), std::move(controller_client));

  // Get the page content before giving up ownership of |page|.
  std::string page_content = page->GetHTMLContents();

  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      handle, std::move(page));

  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      page_content));
}

void DecentralizedDnsNavigationThrottle::ShowEnsOffchainLookupInterstitial() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();

  auto controller_client =
      std::make_unique<EnsOffchainLookupInterstitialControllerClient>(
          web_contents, request_url, user_prefs_, local_state_, locale_);
  auto page = std::make_unique<EnsOffchainLookupOptInPage>(
      web_contents, handle->GetURL(), std::move(controller_client));

  // Get the page content before giving up ownership of |page|.
  std::string page_content = page->GetHTMLContents();

  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      handle, std::move(page));

  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      page_content));
}

const char* DecentralizedDnsNavigationThrottle::GetNameForLogging() {
  return "DecentralizedDnsNavigationThrottle";
}

}  // namespace decentralized_dns
