/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_url_request_interceptor.h"

#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/site_instance.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job.h"

namespace tor {

TorURLRequestInterceptor::TorURLRequestInterceptor(
    content::BrowserContext* browser_context) :
      browser_context_(browser_context) {}
TorURLRequestInterceptor::~TorURLRequestInterceptor() {}

net::URLRequestJob* TorURLRequestInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  SetProxyInternal(request);
  return nullptr;
}

net::URLRequestJob* TorURLRequestInterceptor::MaybeInterceptRedirect(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& location) const {
  SetProxyInternal(request);
  return nullptr;
}

void TorURLRequestInterceptor::SetProxyInternal(net::URLRequest* request) const {
  net::ProxyResolutionService* proxy_service =
    request->context()->proxy_resolution_service();
  GURL url(content::SiteInstance::GetSiteForURL(browser_context_,
                                                request->url()));
  Profile* profile = Profile::FromBrowserContext(browser_context_);
  TorProfileService* tor_profile_service =
    TorProfileServiceFactory::GetForProfile(profile);
  tor_profile_service->SetProxy(proxy_service, url, false);
}

}  // namespace tor
