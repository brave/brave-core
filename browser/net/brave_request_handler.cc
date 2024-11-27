/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_request_handler.h"

#include <algorithm>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/net/brave_ad_block_csp_network_delegate_helper.h"
#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"
#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/brave_localhost_permission_network_delegate_helper.h"
#include "brave/browser/net/brave_reduce_language_network_delegate_helper.h"
#include "brave/browser/net/brave_service_key_network_delegate_helper.h"
#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"
#include "brave/browser/net/brave_stp_util.h"
#include "brave/browser/net/decentralized_dns_network_delegate_helper.h"
#include "brave/browser/net/global_privacy_control_network_delegate_helper.h"
#include "brave/browser/net/search_ads_header_network_delegate_helper.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_constants.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/constants.h"
#include "net/base/features.h"
#include "net/base/net_errors.h"
#include "third_party/blink/public/common/features.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/browser/net/brave_torrent_redirect_network_delegate_helper.h"
#endif

static bool IsInternalScheme(std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK(ctx);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (ctx->request_url.SchemeIs(extensions::kExtensionScheme))
    return true;
#endif
  return ctx->request_url.SchemeIs(content::kChromeUIScheme);
}

BraveRequestHandler::BraveRequestHandler() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  SetupCallbacks();
}

BraveRequestHandler::~BraveRequestHandler() = default;

void BraveRequestHandler::SetupCallbacks() {
  brave::OnBeforeURLRequestCallback callback =
      base::BindRepeating(brave::OnBeforeURLRequest_SiteHacksWork);
  before_url_request_callbacks_.push_back(callback);

  callback = base::BindRepeating(brave::OnBeforeURLRequest_AdBlockTPPreWork);
  before_url_request_callbacks_.push_back(callback);

  callback =
      base::BindRepeating(brave::OnBeforeURLRequest_CommonStaticRedirectWork);
  before_url_request_callbacks_.push_back(callback);

  callback = base::BindRepeating(
      decentralized_dns::OnBeforeURLRequest_DecentralizedDnsPreRedirectWork);
  before_url_request_callbacks_.push_back(callback);

  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveLocalhostAccessPermission)) {
    callback =
        base::BindRepeating(brave::OnBeforeURLRequest_LocalhostPermissionWork);
    before_url_request_callbacks_.push_back(callback);
  }

  brave::OnBeforeStartTransactionCallback start_transaction_callback =
      base::BindRepeating(brave::OnBeforeStartTransaction_SiteHacksWork);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);

  if (base::FeatureList::IsEnabled(
          blink::features::kBraveGlobalPrivacyControl)) {
    start_transaction_callback = base::BindRepeating(
        brave::OnBeforeStartTransaction_GlobalPrivacyControlWork);
    before_start_transaction_callbacks_.push_back(start_transaction_callback);
  }

  start_transaction_callback =
      base::BindRepeating(brave::OnBeforeStartTransaction_BraveServiceKey);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);

  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveReduceLanguage)) {
    start_transaction_callback =
        base::BindRepeating(brave::OnBeforeStartTransaction_ReduceLanguageWork);
    before_start_transaction_callbacks_.push_back(start_transaction_callback);
  }

  start_transaction_callback =
      base::BindRepeating(brave::OnBeforeStartTransaction_SearchAdsHeader);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  brave::OnHeadersReceivedCallback headers_received_callback =
      base::BindRepeating(webtorrent::OnHeadersReceived_TorrentRedirectWork);
  headers_received_callbacks_.push_back(headers_received_callback);
#endif

  if (base::FeatureList::IsEnabled(
          ::brave_shields::features::kBraveAdblockCspRules)) {
    brave::OnHeadersReceivedCallback headers_received_callback2 =
        base::BindRepeating(brave::OnHeadersReceived_AdBlockCspWork);
    headers_received_callbacks_.push_back(headers_received_callback2);
  }
}

bool BraveRequestHandler::IsRequestIdentifierValid(
    uint64_t request_identifier) {
  return callbacks_.contains(request_identifier);
}

int BraveRequestHandler::OnBeforeURLRequest(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  if (before_url_request_callbacks_.empty() || IsInternalScheme(ctx)) {
    return net::OK;
  }
  ctx->new_url = new_url;
  ctx->event_type = brave::kOnBeforeRequest;
  callbacks_[ctx->request_identifier] = std::move(callback);
  RunNextCallback(ctx);
  return net::ERR_IO_PENDING;
}

int BraveRequestHandler::OnBeforeStartTransaction(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    net::HttpRequestHeaders* headers) {
  if (before_start_transaction_callbacks_.empty() || IsInternalScheme(ctx)) {
    return net::OK;
  }
  ctx->event_type = brave::kOnBeforeStartTransaction;
  ctx->headers = headers;
  callbacks_[ctx->request_identifier] = std::move(callback);
  RunNextCallback(ctx);
  return net::ERR_IO_PENDING;
}

int BraveRequestHandler::OnHeadersReceived(
    std::shared_ptr<brave::BraveRequestInfo> ctx,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  if (!ctx->tab_origin.is_empty()) {
    brave::RemoveTrackableSecurityHeadersForThirdParty(
        ctx->request_url, url::Origin::Create(ctx->tab_origin),
        original_response_headers, override_response_headers);
  }

  if (headers_received_callbacks_.empty() &&
      !ctx->request_url.SchemeIs(content::kChromeUIScheme)) {
    // Extension scheme not excluded since brave_webtorrent needs it.
    return net::OK;
  }

  callbacks_[ctx->request_identifier] = std::move(callback);
  ctx->event_type = brave::kOnHeadersReceived;
  ctx->original_response_headers = original_response_headers;
  ctx->override_response_headers = override_response_headers;
  ctx->allowed_unsafe_redirect_url = allowed_unsafe_redirect_url;

  RunNextCallback(ctx);
  return net::ERR_IO_PENDING;
}

void BraveRequestHandler::OnURLRequestDestroyed(
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  auto it = callbacks_.find(ctx->request_identifier);
  if (it != callbacks_.end()) {
    callbacks_.erase(it);
  }
}

void BraveRequestHandler::RunCallbackForRequestIdentifier(
    uint64_t request_identifier,
    int rv) {
  std::map<uint64_t, net::CompletionOnceCallback>::iterator it =
      callbacks_.find(request_identifier);
  // We intentionally do the async call to maintain the proper flow
  // of URLLoader callbacks.
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(std::move(it->second), rv));
}

// TODO(iefremov): Merge all callback containers into one and run only one loop
// instead of many (issues/5574).
void BraveRequestHandler::RunNextCallback(
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!callbacks_.contains(ctx->request_identifier)) {
    return;
  }

  if (ctx->pending_error.has_value()) {
    RunCallbackForRequestIdentifier(ctx->request_identifier,
                                    ctx->pending_error.value());
    return;
  }

  // Continue processing callbacks until we hit one that returns PENDING
  int rv = net::OK;

  if (ctx->event_type == brave::kOnBeforeRequest) {
    while (before_url_request_callbacks_.size() !=
           ctx->next_url_request_index) {
      brave::OnBeforeURLRequestCallback callback =
          before_url_request_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::BindRepeating(&BraveRequestHandler::RunNextCallback,
                              weak_factory_.GetWeakPtr(), ctx);
      rv = callback.Run(next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv != net::OK) {
        break;
      }
    }
  } else if (ctx->event_type == brave::kOnBeforeStartTransaction) {
    while (before_start_transaction_callbacks_.size() !=
           ctx->next_url_request_index) {
      brave::OnBeforeStartTransactionCallback callback =
          before_start_transaction_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::BindRepeating(&BraveRequestHandler::RunNextCallback,
                              weak_factory_.GetWeakPtr(), ctx);
      rv = callback.Run(ctx->headers, next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv != net::OK) {
        break;
      }
    }
  } else if (ctx->event_type == brave::kOnHeadersReceived) {
    while (headers_received_callbacks_.size() != ctx->next_url_request_index) {
      brave::OnHeadersReceivedCallback callback =
          headers_received_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::BindRepeating(&BraveRequestHandler::RunNextCallback,
                              weak_factory_.GetWeakPtr(), ctx);
      rv = callback.Run(ctx->original_response_headers,
                        ctx->override_response_headers,
                        ctx->allowed_unsafe_redirect_url, next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv != net::OK) {
        break;
      }
    }
  }

  if (rv != net::OK) {
    RunCallbackForRequestIdentifier(ctx->request_identifier, rv);
    return;
  }

  if (ctx->event_type == brave::kOnBeforeRequest) {
    if (!ctx->new_url_spec.empty() &&
        (ctx->new_url_spec != ctx->request_url.spec()) &&
        IsRequestIdentifierValid(ctx->request_identifier)) {
      *ctx->new_url = GURL(ctx->new_url_spec);
    }
    if (ctx->blocked_by == brave::kAdBlocked ||
        ctx->blocked_by == brave::kOtherBlocked) {
      if (!ctx->ShouldMockRequest()) {
        RunCallbackForRequestIdentifier(ctx->request_identifier,
                                        net::ERR_BLOCKED_BY_CLIENT);
        return;
      }
    }
  }
  RunCallbackForRequestIdentifier(ctx->request_identifier, rv);
}
