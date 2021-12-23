/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_navigation_throttle.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/rand_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_interstitial_controller_client.h"
#include "brave/components/ipfs/ipfs_not_connected_page.h"
#include "brave/components/ipfs/ipfs_onboarding_page.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "net/base/net_errors.h"

namespace {

// Used to retry request if we got zero peers from ipfs service
// Actual value will be generated randomly in range
// (kMinimalPeersRetryIntervalMs, kPeersRetryRate*kMinimalPeersRetryIntervalMs)
const int kMinimalPeersRetryIntervalMs = 50;
const int kPeersRetryRate = 3;

base::TimeDelta CalculatePeersRetryTime() {
  return base::Milliseconds(
      base::RandInt(kMinimalPeersRetryIntervalMs,
                    kPeersRetryRate * kMinimalPeersRetryIntervalMs));
}

// Used to scope the posted navigation task to the lifetime of |web_contents|.
class IPFSWebContentsLifetimeHelper
    : public content::WebContentsUserData<IPFSWebContentsLifetimeHelper> {
 public:
  explicit IPFSWebContentsLifetimeHelper(content::WebContents* web_contents)
      : content::WebContentsUserData<IPFSWebContentsLifetimeHelper>(
            *web_contents) {}

  base::WeakPtr<IPFSWebContentsLifetimeHelper> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void NavigateTo(const content::OpenURLParams& url_params) {
    GetWebContents().OpenURL(url_params);
  }

 private:
  friend class content::WebContentsUserData<IPFSWebContentsLifetimeHelper>;

  base::WeakPtrFactory<IPFSWebContentsLifetimeHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSWebContentsLifetimeHelper);

}  // namespace

namespace ipfs {

// static
std::unique_ptr<IpfsNavigationThrottle>
IpfsNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    IpfsService* ipfs_service,
    PrefService* pref_service,
    const std::string& locale) {
  if (!ipfs_service)
    return nullptr;
  return std::make_unique<IpfsNavigationThrottle>(
      navigation_handle, ipfs_service, pref_service, locale);
}

IpfsNavigationThrottle::IpfsNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    IpfsService* ipfs_service,
    PrefService* pref_service,
    const std::string& locale)
    : content::NavigationThrottle(navigation_handle),
      ipfs_service_(ipfs_service),
      pref_service_(pref_service),
      locale_(locale) {}

IpfsNavigationThrottle::~IpfsNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
IpfsNavigationThrottle::WillStartRequest() {
  GURL url = navigation_handle()->GetURL();

  bool should_ask =
      pref_service_->FindPreference(kIPFSResolveMethod) &&
      pref_service_->GetInteger(kIPFSResolveMethod) ==
          static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK);

  if (IsIPFSScheme(url) && should_ask) {
    return ShowIPFSOnboardingInterstitial();
  }

  if (!IsLocalGatewayURL(url)) {
    return content::NavigationThrottle::PROCEED;
  }

  bool is_local_mode =
      pref_service_->FindPreference(kIPFSResolveMethod) &&
      pref_service_->GetInteger(kIPFSResolveMethod) ==
          static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);

  if (is_local_mode && !ipfs_service_->IsDaemonLaunched()) {
    resume_pending_ = true;
    ipfs_service_->LaunchDaemon(
        base::BindOnce(&IpfsNavigationThrottle::OnIpfsLaunched,
                       weak_ptr_factory_.GetWeakPtr()));
    return content::NavigationThrottle::DEFER;
  }

  // Check # of connected peers before using local node.
  if (is_local_mode && ipfs_service_->IsDaemonLaunched()) {
    resume_pending_ = true;
    GetConnectedPeers();
    return content::NavigationThrottle::DEFER;
  }

  return content::NavigationThrottle::PROCEED;
}

void IpfsNavigationThrottle::GetConnectedPeers() {
  ipfs_service_->GetConnectedPeers(
      base::BindOnce(&IpfsNavigationThrottle::OnGetConnectedPeers,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IpfsNavigationThrottle::OnGetConnectedPeers(
    bool success,
    const std::vector<std::string>& peers) {
  if (!resume_pending_)
    return;

  resume_pending_ = false;

  // Resume the navigation if there are connected peers.
  if (success && !peers.empty()) {
    Resume();
    return;
  }

  if (success && peers.empty()) {
    resume_pending_ = true;
    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&IpfsNavigationThrottle::GetConnectedPeers,
                       weak_ptr_factory_.GetWeakPtr()),
        CalculatePeersRetryTime());
    return;
  }
  // Show interstitial page if kIPFSAutoFallbackToGateway is not set to true,
  // which will cancel the deferred navigation.
  if (!pref_service_->FindPreference(kIPFSAutoFallbackToGateway) ||
      !pref_service_->GetBoolean(kIPFSAutoFallbackToGateway)) {
    ShowInterstitial();
    return;
  }

  // Fallback to the public gateway.
  LoadPublicGatewayURL();
  CancelDeferredNavigation(content::NavigationThrottle::CANCEL_AND_IGNORE);
}

content::NavigationThrottle::ThrottleCheckResult
IpfsNavigationThrottle::ShowIPFSOnboardingInterstitial() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();

  auto controller_client = std::make_unique<IPFSInterstitialControllerClient>(
      web_contents, request_url, pref_service_, locale_);
  auto page = std::make_unique<IPFSOnboardingPage>(
      ipfs_service_, web_contents, handle->GetURL(),
      std::move(controller_client));

  // Get the page content before giving up ownership of |page|.
  std::string page_content = page->GetHTMLContents();

  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      handle, std::move(page));
  return content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      page_content);
}

void IpfsNavigationThrottle::ShowInterstitial() {
  content::NavigationHandle* handle = navigation_handle();
  content::WebContents* web_contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();

  auto controller_client = std::make_unique<IPFSInterstitialControllerClient>(
      web_contents, request_url, pref_service_, locale_);
  auto page = std::make_unique<IPFSNotConnectedPage>(
      web_contents, handle->GetURL(), std::move(controller_client));

  // Get the page content before giving up ownership of |page|.
  std::string page_content = page->GetHTMLContents();

  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      handle, std::move(page));

  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
      page_content));
}

void IpfsNavigationThrottle::LoadPublicGatewayURL() {
  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents)
    return;

  const GURL url =
      ToPublicGatewayURL(navigation_handle()->GetURL(), pref_service_);
  if (url.is_empty())
    return;

  content::OpenURLParams params =
      content::OpenURLParams::FromNavigationHandle(navigation_handle());
  params.url = url;
  params.transition = ui::PAGE_TRANSITION_CLIENT_REDIRECT;

  // Post a task to navigate to the public gateway URL, as starting a
  // navigation within a navigation is an antipattern. Use a helper object
  // scoped to the WebContents lifetime to scope the navigation task to the
  // WebContents lifetime.
  IPFSWebContentsLifetimeHelper::CreateForWebContents(web_contents);
  IPFSWebContentsLifetimeHelper* helper =
      IPFSWebContentsLifetimeHelper::FromWebContents(web_contents);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&IPFSWebContentsLifetimeHelper::NavigateTo,
                                helper->GetWeakPtr(), std::move(params)));
}

const char* IpfsNavigationThrottle::GetNameForLogging() {
  return "IpfsNavigationThrottle";
}

void IpfsNavigationThrottle::OnIpfsLaunched(bool result) {
  if (!resume_pending_)
    return;

  if (!result) {
    resume_pending_ = false;
    ShowInterstitial();
  } else {
    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&IpfsNavigationThrottle::GetConnectedPeers,
                       weak_ptr_factory_.GetWeakPtr()),
        CalculatePeersRetryTime());
  }
}

}  // namespace ipfs
