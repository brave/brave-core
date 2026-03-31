/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_csp_network_delegate_helper.h"

#include <optional>
#include <string>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_service_helper.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace brave {

std::optional<std::string> GetCspDirectivesOnTaskRunner(
    brave_shields::AdBlockEngineWrapper* engine_wrapper,
    const GURL& initiator_url,
    const GURL& request_url,
    blink::mojom::ResourceType resource_type,
    std::optional<std::string> original_csp) {
  std::string source_host;
  if (initiator_url.is_valid() && !initiator_url.host().empty()) {
    source_host = initiator_url.host();
  } else if (request_url.is_valid()) {
    // Top-level document requests do not have a valid initiator URL, and
    // requests from special schemes like file:// do not have host parts, so we
    // use the request URL as the initiator.
    source_host = request_url.host();
  } else {
    return std::nullopt;
  }

  std::optional<std::string> csp_directives =
      engine_wrapper->GetCspDirectives(request_url, resource_type, source_host);

  brave_shields::MergeCspDirectiveInto(original_csp, &csp_directives);
  return csp_directives;
}

void OnReceiveCspDirectives(
    const ResponseCallback& next_callback,
    scoped_refptr<net::HttpResponseHeaders> override_response_headers,
    std::optional<std::string> csp_directives) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (csp_directives) {
    override_response_headers
        ->AddHeader("Content-Security-Policy", *csp_directives);
  }

  next_callback.Run();
}

template <template <typename> class T>
int OnHeadersReceived_AdBlockCspWork(
    const net::HttpResponseHeaders* response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const brave::ResponseCallback& next_callback,
    T<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(ctx);

  if (!response_headers || !ctx->allow_brave_shields() || ctx->allow_ads()) {
    return net::OK;
  }

  if (ctx->resource_type() == blink::mojom::ResourceType::kMainFrame ||
      ctx->resource_type() == blink::mojom::ResourceType::kSubFrame) {
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
            base::BindOnce(
                &GetCspDirectivesOnTaskRunner,
                base::Unretained(g_brave_browser_process->ad_block_service()
                                     ->engine_wrapper()),
                ctx->initiator_url(), ctx->request_url(), ctx->resource_type(),
                original_csp),
            base::BindOnce(&OnReceiveCspDirectives, next_callback,
                           *override_response_headers));
    return net::ERR_IO_PENDING;
  }

  return net::OK;
}

template int OnHeadersReceived_AdBlockCspWork<std::shared_ptr>(
    const net::HttpResponseHeaders*,
    scoped_refptr<net::HttpResponseHeaders>*,
    GURL*,
    const brave::ResponseCallback&,
    std::shared_ptr<brave::BraveRequestInfo>);

template int OnHeadersReceived_AdBlockCspWork<base::WeakPtr>(
    const net::HttpResponseHeaders*,
    scoped_refptr<net::HttpResponseHeaders>*,
    GURL*,
    const brave::ResponseCallback&,
    base::WeakPtr<brave::BraveRequestInfo>);

}  // namespace brave
