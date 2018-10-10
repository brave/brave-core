/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_tor_network_delegate_helper.h"

#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#include "net/url_request/url_request_context.h"

using content::BrowserThread;
using content::ResourceRequestInfo;

namespace brave {

int OnBeforeURLRequest_TorWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  const ResourceRequestInfo* resource_info =
    ResourceRequestInfo::ForRequest(request);
  if (!resource_info) {
    return net::OK;
  }

  const BraveNavigationUIData* ui_data =
    static_cast<const BraveNavigationUIData*>(
        resource_info->GetNavigationUIData());
  if (!ui_data) {
    return net::OK;
  }

  auto* tor_profile_service = ui_data->GetTorProfileService();
  if (!tor_profile_service) {
    return net::OK;
  }

  if (!(request->url().SchemeIsHTTPOrHTTPS() ||
        request->url().SchemeIs(content::kChromeUIScheme) ||
        request->url().SchemeIs(extensions::kExtensionScheme) ||
        request->url().SchemeIs(content::kChromeDevToolsScheme))) {
    return net::ERR_DISALLOWED_URL_SCHEME;
  }

  auto* proxy_service = request->context()->proxy_resolution_service();
  tor_profile_service->SetProxy(proxy_service, request->url(), false);

  return net::OK;
}

}  // namespace brave
