/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_wallet_navigation_throttle.h"

#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/extensions/extension_service.h"
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
    resume_pending(false) {
  extension_registry_observer_.Add(
      ExtensionRegistry::Get(
          navigation_handle->GetWebContents()->GetBrowserContext()));
}

BraveWalletNavigationThrottle::~BraveWalletNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
BraveWalletNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::WebContents* web_contents = navigation_handle()->GetWebContents();

  // Is this navigation targeting an extension resource?
  const GURL& url = navigation_handle()->GetURL();
  if (url.SchemeIs(content::kChromeUIScheme) &&
      url.host() == ethereum_remote_client_host) {
      extensions::ExtensionService* service =
         extensions::ExtensionSystem::Get(
             web_contents->GetBrowserContext())->extension_service();
       extensions::ComponentLoader* loader = service->component_loader();
      if (!loader->Exists(ethereum_remote_client_extension_id)) {
        static_cast<extensions::BraveComponentLoader*>(loader)->
            AddEthereumRemoteClientExtension();
        resume_pending = true;
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
  if (resume_pending &&
      extension->id() == ethereum_remote_client_extension_id) {
    Resume();
  }
}

}  // namespace extensions
