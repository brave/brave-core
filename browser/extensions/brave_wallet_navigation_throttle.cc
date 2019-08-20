/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_wallet_navigation_throttle.h"

#include "base/bind.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"

namespace extensions {

BraveWalletNavigationThrottle::BraveWalletNavigationThrottle(
        content::NavigationHandle* navigation_handle) :
    content::NavigationThrottle(navigation_handle),
    extension_registry_observer_(this),
    resume_pending_(false) {
  extension_registry_observer_.Add(
      ExtensionRegistry::Get(
          navigation_handle->GetWebContents()->GetBrowserContext()));
}

BraveWalletNavigationThrottle::~BraveWalletNavigationThrottle() {
  timer_.Stop();
}

content::NavigationThrottle::ThrottleCheckResult
BraveWalletNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::WebContents* web_contents = navigation_handle()->GetWebContents();

  // Is this navigation targeting an extension resource?
  const GURL& url = navigation_handle()->GetURL();
  if (url.SchemeIs(content::kChromeUIScheme) &&
      url.host() == ethereum_remote_client_host) {
    // If a user has explicitly disabled the Brave Wallet,
    // then don't defer and try to install it.
    content::BrowserContext* browser_context =
        web_contents->GetBrowserContext();
    Profile* profile = Profile::FromBrowserContext(browser_context);
    if (!profile->GetPrefs()->GetBoolean(kBraveWalletEnabled) ||
        brave::IsTorProfile(profile)) {
      return content::NavigationThrottle::BLOCK_REQUEST;
    }
    auto* registry = ExtensionRegistry::Get(browser_context);
    if (!registry->ready_extensions().GetByID(
          ethereum_remote_client_extension_id)) {
      resume_pending_ = true;
      extensions::ExtensionService* service =
         extensions::ExtensionSystem::Get(browser_context)->extension_service();
      if (service) {
        extensions::ComponentLoader* loader = service->component_loader();
        static_cast<extensions::BraveComponentLoader*>(loader)->
            AddEthereumRemoteClientExtension();
      }
      return content::NavigationThrottle::DEFER;
    }
  }
  return content::NavigationThrottle::PROCEED;
}

const char* BraveWalletNavigationThrottle::GetNameForLogging() {
  return "BraveWalletNavigationThrottle";
}

void BraveWalletNavigationThrottle::OnExtensionReady(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  if (resume_pending_ &&
      extension->id() == ethereum_remote_client_extension_id) {
    // TODO(bbondy): For some reason the page won't load directly after install
    // and on startups even though the Ready event has fired.
    // There are no ExtensionRegistryObserver functions that get
    // called after this. I even tried checking if the background
    // process was available, and it was.
    // Delaying 1 second for when the extension is not already
    // loaded and ready makes this work reliabley for now.
    // The bug without this only seems to surface itself in Release builds.
    ScheduleBackgroundScriptTimer();
  }
}

void BraveWalletNavigationThrottle::ScheduleBackgroundScriptTimer() {
  constexpr base::TimeDelta kBackgroundScriptTimeout =
      base::TimeDelta::FromSeconds(1);
  timer_.Stop();
  timer_.Start(FROM_HERE, kBackgroundScriptTimeout,
  base::BindOnce(
      &BraveWalletNavigationThrottle::WalletBackgroundScriptTimer,
      base::Unretained(this)));
}

void BraveWalletNavigationThrottle::WalletBackgroundScriptTimer() {
  ResumeThrottle();
}

void BraveWalletNavigationThrottle::ResumeThrottle() {
  timer_.Stop();
  resume_pending_ = false;
  Resume();
}

}  // namespace extensions
