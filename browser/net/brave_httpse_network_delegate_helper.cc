/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_httpse_network_delegate_helper.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;

namespace brave {

void OnBeforeURLRequest_HttpseFileWork(
    net::URLRequest* request,
    GURL* new_url,
    std::shared_ptr<BraveRequestInfo> ctx) {
  base::AssertBlockingAllowed();
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  DCHECK(ctx->request_identifier != 0);
  g_brave_browser_process->https_everywhere_service()->
    GetHTTPSURL(&ctx->request_url, ctx->request_identifier, ctx->new_url_spec);
}

void OnBeforeURLRequest_HttpsePostFileWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {

  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!ctx->new_url_spec.empty() &&
    ctx->new_url_spec != request->url().spec()) {
    *new_url = GURL(ctx->new_url_spec);
    brave_shields::DispatchBlockedEventFromIO(request,
        brave_shields::kHTTPUpgradableResources);
  }

  next_callback.Run();
}

int OnBeforeURLRequest_HttpsePreFileWork(
  net::URLRequest* request,
  GURL* new_url,
  const ResponseCallback& next_callback,
  std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  GURL tab_origin = request->site_for_cookies().GetOrigin();
  bool allow_brave_shields = brave_shields::IsAllowContentSettingFromIO(
      request, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields);
  bool allow_http_upgradable_resource = brave_shields::IsAllowContentSettingFromIO(
      request, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kHTTPUpgradableResources);
  if (tab_origin.is_empty() || allow_http_upgradable_resource ||
      !allow_brave_shields) {
    return net::OK;
  }

  bool is_valid_url = true;
  if (request) {
    is_valid_url = request->url().is_valid();
    std::string scheme = request->url().scheme();
    if (scheme.length()) {
      std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
      if ("http" != scheme && "https" != scheme) {
        is_valid_url = false;
      }
    }
  }

  if (is_valid_url) {
    if (!g_brave_browser_process->https_everywhere_service()->
        GetHTTPSURLFromCacheOnly(&request->url(), request->identifier(),
          ctx->new_url_spec)) {
      ctx->request_url = request->url();
      BrowserThread::PostTaskAndReply(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(OnBeforeURLRequest_HttpseFileWork,
            base::Unretained(request), new_url, ctx),
        base::Bind(base::IgnoreResult(
            &OnBeforeURLRequest_HttpsePostFileWork),
            base::Unretained(request),
            new_url, next_callback, ctx)
          );
      return net::ERR_IO_PENDING;
    } else {
      if (!ctx->new_url_spec.empty()) {
        *new_url = GURL(ctx->new_url_spec);
        brave_shields::DispatchBlockedEventFromIO(request, "httpsEverywhere");
      }
    }
  }

  return net::OK;
}

}  // namespace brave
