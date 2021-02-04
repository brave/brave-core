/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/unstoppable_domains_navigation_throttle.h"

#include <utility>

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/unstoppable_domains/unstoppable_domains_interstitial_controller_client.h"
#include "brave/components/unstoppable_domains/unstoppable_domains_opt_in_page.h"
#include "brave/components/unstoppable_domains/utils.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"

namespace unstoppable_domains {

// static
std::unique_ptr<UnstoppableDomainsNavigationThrottle>
UnstoppableDomainsNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    PrefService* local_state,
    const std::string& locale) {
  if (!IsUnstoppableDomainsEnabled())
    return nullptr;

  return std::make_unique<UnstoppableDomainsNavigationThrottle>(
      navigation_handle, local_state, locale);
}

UnstoppableDomainsNavigationThrottle::UnstoppableDomainsNavigationThrottle(
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

UnstoppableDomainsNavigationThrottle::~UnstoppableDomainsNavigationThrottle() =
    default;

content::NavigationThrottle::ThrottleCheckResult
UnstoppableDomainsNavigationThrottle::WillStartRequest() {
  GURL url = navigation_handle()->GetURL();
  if (!IsUnstoppableDomainsTLD(url) || !IsResolveMethodAsk(local_state_)) {
    return content::NavigationThrottle::PROCEED;
  }

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&UnstoppableDomainsNavigationThrottle::ShowInterstitial,
                     weak_ptr_factory_.GetWeakPtr()));
  return content::NavigationThrottle::DEFER;
}

void UnstoppableDomainsNavigationThrottle::ShowInterstitial() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();

  auto controller_client =
      std::make_unique<UnstoppableDomainsInterstitialControllerClient>(
          web_contents, request_url, user_prefs_, local_state_, locale_);
  auto page = std::make_unique<UnstoppableDomainsOptInPage>(
      web_contents, handle->GetURL(), std::move(controller_client));

  // Get the page content before giving up ownership of |page|.
  std::string page_content = page->GetHTMLContents();

  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      web_contents, handle->GetNavigationId(), std::move(page));

  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      page_content));
}

const char* UnstoppableDomainsNavigationThrottle::GetNameForLogging() {
  return "UnstoppableDomainsNavigationThrottle";
}

}  // namespace unstoppable_domains
