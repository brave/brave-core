/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_csp_network_delegate_helper.h"

#include <optional>
#include <string>

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_service_helper.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace brave {

std::optional<std::string> GetCspDirectivesOnTaskRunner(
    std::shared_ptr<BraveRequestInfo> ctx,
    std::optional<std::string> original_csp) {
  std::string source_host;
  if (ctx->initiator_url.is_valid() && !ctx->initiator_url.host().empty()) {
    source_host = ctx->initiator_url.host();
  } else if (ctx->request_url.is_valid()) {
    // Top-level document requests do not have a valid initiator URL, and
    // requests from special schemes like file:// do not have host parts, so we
    // use the request URL as the initiator.
    source_host = ctx->request_url.host();
  } else {
    return std::nullopt;
  }

  std::optional<std::string> csp_directives =
      g_brave_browser_process->ad_block_service()->GetCspDirectives(
          ctx->request_url, ctx->resource_type, source_host);

  brave_shields::MergeCspDirectiveInto(original_csp, &csp_directives);
  return csp_directives;
}

void OnReceiveCspDirectives(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx,
    scoped_refptr<net::HttpResponseHeaders> override_response_headers,
    std::optional<std::string> csp_directives) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (csp_directives) {
    override_response_headers
        ->AddHeader("Content-Security-Policy", *csp_directives);
  }

  next_callback.Run();
}

int OnHeadersReceived_AdBlockCspWork(
    const net::HttpResponseHeaders* response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!response_headers || !ctx->allow_brave_shields || ctx->allow_ads) {
    return net::OK;
  }

  if (ctx->resource_type == blink::mojom::ResourceType::kMainFrame ||
      ctx->resource_type == blink::mojom::ResourceType::kSubFrame) {
    // If the override_response_headers have already been populated, we should
    // use those directly.  Otherwise, we populate them from the original
    // headers.
    if (!*override_response_headers) {
      *override_response_headers =
          new net::HttpResponseHeaders(response_headers->raw_headers());
    }

    std::optional<std::string> original_csp =
        (*override_response_headers)
            ->GetNormalizedHeader("Content-Security-Policy");

    (*override_response_headers)->RemoveHeader("Content-Security-Policy");

    g_brave_browser_process->ad_block_service()
        ->GetTaskRunner()
        ->PostTaskAndReplyWithResult(
            FROM_HERE,
            base::BindOnce(&GetCspDirectivesOnTaskRunner, ctx, original_csp),
            base::BindOnce(&OnReceiveCspDirectives, next_callback, ctx,
                           *override_response_headers));
    return net::ERR_IO_PENDING;
  }

  return net::OK;
}

}  // namespace brave
