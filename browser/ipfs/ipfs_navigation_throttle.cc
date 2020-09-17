/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_navigation_throttle.h"

#include "brave/browser/ipfs/ipfs_service.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/common/pref_names.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "brave/components/ipfs/common/ipfs_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace ipfs {

// static
std::unique_ptr<IpfsNavigationThrottle>
IpfsNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  if (!IpfsService::IsIpfsEnabled(
      navigation_handle->GetWebContents()->GetBrowserContext()))
    return nullptr;

  return std::make_unique<IpfsNavigationThrottle>(navigation_handle);
}

IpfsNavigationThrottle::IpfsNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {
  content::BrowserContext* context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  ipfs_service_ = IpfsServiceFactory::GetForContext(context);
  pref_service_ = user_prefs::UserPrefs::Get(context);

  DCHECK(ipfs_service_);
  ipfs_service_->AddObserver(this);
}

IpfsNavigationThrottle::~IpfsNavigationThrottle() {
  ipfs_service_->RemoveObserver(this);
}

content::NavigationThrottle::ThrottleCheckResult
IpfsNavigationThrottle::WillStartRequest() {
  GURL url = navigation_handle()->GetURL();
  if (!IpfsUtils::IsIPFSURL(url)) {
    return content::NavigationThrottle::PROCEED;
  }

  bool is_local_mode = pref_service_->FindPreference(kIPFSResolveMethod) &&
      pref_service_->GetInteger(kIPFSResolveMethod) ==
          static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);

  if (is_local_mode && !ipfs_service_->IsDaemonLaunched()) {
    resume_pending_ = true;
    ipfs_service_->RegisterIpfsClientUpdater();
    return content::NavigationThrottle::DEFER;
  }

  return content::NavigationThrottle::PROCEED;
}

const char* IpfsNavigationThrottle::GetNameForLogging() {
  return "IpfsNavigationThrottle";
}

void IpfsNavigationThrottle::OnIpfsLaunched(bool result, int64_t pid) {
  if (result && resume_pending_) {
    resume_pending_ = false;
    Resume();
  }
}

}  // namespace ipfs
