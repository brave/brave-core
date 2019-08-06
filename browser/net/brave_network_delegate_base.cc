/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_network_delegate_base.h"

#include <algorithm>
#include <utility>

#include "base/task/post_task.h"
#include "base/stl_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/net/brave_stp_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;
using content::ResourceRequestInfo;
using net::HttpResponseHeaders;
using net::URLRequest;

namespace {

bool OnAllowAccessCookies(
    const URLRequest& request,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(&request);
  if (info) {
    ProfileIOData* io_data =
        ProfileIOData::FromResourceContext(info->GetContext());

    GURL url = request.url();
    GURL tab_origin = request.site_for_cookies();
    if (tab_origin.is_empty())
      tab_origin = GURL(request.network_isolation_key().ToString());
    if (tab_origin.is_empty() && request.top_frame_origin().has_value())
      tab_origin = request.top_frame_origin()->GetURL();

    return
        io_data->GetCookieSettings()->IsCookieAccessAllowed(url, tab_origin) &&
            g_brave_browser_process->tracking_protection_service()
                ->ShouldStoreState(io_data->GetHostContentSettingsMap(),
                                   ctx->render_process_id,
                                   ctx->render_frame_id,
                                   url,
                                   tab_origin);
  }

  return true;
}

}  // namespace


BraveNetworkDelegateBase::BraveNetworkDelegateBase(
    extensions::EventRouterForwarder* event_router)
    : ChromeNetworkDelegate(event_router),
      referral_headers_list_(nullptr) {
  // Initialize the preference change registrar.
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::UI},
      base::Bind(&BraveNetworkDelegateBase::InitPrefChangeRegistrarOnUI,
                 base::Unretained(this)));
}

BraveNetworkDelegateBase::~BraveNetworkDelegateBase() {}

void BraveNetworkDelegateBase::InitPrefChangeRegistrarOnUI() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  PrefService* prefs = g_browser_process->local_state();
  pref_change_registrar_.reset(new PrefChangeRegistrar());
  pref_change_registrar_->Init(prefs);
  pref_change_registrar_->Add(
      kReferralHeaders,
      base::Bind(&BraveNetworkDelegateBase::OnReferralHeadersChanged,
                 base::Unretained(this)));
  // Retrieve current referral headers, if any.
  OnReferralHeadersChanged();
#endif
}

void BraveNetworkDelegateBase::OnReferralHeadersChanged() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (const base::ListValue* referral_headers =
          g_browser_process->local_state()->GetList(kReferralHeaders)) {
    base::PostTaskWithTraits(
        FROM_HERE, {BrowserThread::IO},
        base::Bind(&BraveNetworkDelegateBase::SetReferralHeaders,
                   base::Unretained(this), referral_headers->DeepCopy()));
  }
}

void BraveNetworkDelegateBase::SetReferralHeaders(
    base::ListValue* referral_headers) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  referral_headers_list_.reset(referral_headers);
}

int BraveNetworkDelegateBase::OnBeforeURLRequest(
    URLRequest* request,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  if (before_url_request_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnBeforeURLRequest(
        request, std::move(callback), new_url);
  }
  std::shared_ptr<brave::BraveRequestInfo> ctx(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request, ctx);
  ctx->new_url = new_url;
  ctx->event_type = brave::kOnBeforeRequest;
  callbacks_[request->identifier()] = std::move(callback);
  RunNextCallback(request, ctx);
  return net::ERR_IO_PENDING;
}

int BraveNetworkDelegateBase::OnBeforeStartTransaction(
    URLRequest* request,
    net::CompletionOnceCallback callback,
    net::HttpRequestHeaders* headers) {
  if (before_start_transaction_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnBeforeStartTransaction(
        request, std::move(callback), headers);
  }
  std::shared_ptr<brave::BraveRequestInfo> ctx(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request, ctx);
  ctx->event_type = brave::kOnBeforeStartTransaction;
  ctx->headers = headers;
  ctx->referral_headers_list = referral_headers_list_.get();
  callbacks_[request->identifier()] = std::move(callback);
  RunNextCallback(request, ctx);
  return net::ERR_IO_PENDING;
}

int BraveNetworkDelegateBase::OnHeadersReceived(
    URLRequest* request,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  if (request->top_frame_origin().has_value()) {
    brave::RemoveTrackableSecurityHeadersForThirdParty(
        request->url(), request->top_frame_origin().value(),
        original_response_headers, override_response_headers);
  }

  if (headers_received_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnHeadersReceived(
        request, std::move(callback), original_response_headers,
        override_response_headers, allowed_unsafe_redirect_url);
  }

  std::shared_ptr<brave::BraveRequestInfo> ctx(new brave::BraveRequestInfo());
  callbacks_[request->identifier()] = std::move(callback);
  brave::BraveRequestInfo::FillCTXFromRequest(request, ctx);
  ctx->event_type = brave::kOnHeadersReceived;
  ctx->original_response_headers = original_response_headers;
  ctx->override_response_headers = override_response_headers;
  ctx->allowed_unsafe_redirect_url = allowed_unsafe_redirect_url;

  // Return ERR_IO_PENDING and run callbacks later by posting a task.
  // URLRequestHttpJob::awaiting_callback_ will be set to true after we
  // return net::ERR_IO_PENDING here, callbacks need to be run later than this
  // to set awaiting_callback_ back to false.
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
                 base::Unretained(this), request, ctx));
  return net::ERR_IO_PENDING;
}

bool BraveNetworkDelegateBase::OnCanGetCookies(
    const URLRequest& request,
    const net::CookieList& cookie_list,
    bool allowed_from_caller) {
  std::shared_ptr<brave::BraveRequestInfo> ctx(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(&request, ctx);
  ctx->event_type = brave::kOnCanGetCookies;

  return OnAllowAccessCookies(request, ctx);
}

bool BraveNetworkDelegateBase::OnCanSetCookie(
    const URLRequest& request,
    const net::CanonicalCookie& cookie,
    net::CookieOptions* options,
    bool allowed_from_caller) {
  std::shared_ptr<brave::BraveRequestInfo> ctx(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(&request, ctx);
  ctx->event_type = brave::kOnCanSetCookies;

  return OnAllowAccessCookies(request, ctx);
}

void BraveNetworkDelegateBase::RunCallbackForRequestIdentifier(
    uint64_t request_identifier,
    int rv) {
  std::map<uint64_t, net::CompletionOnceCallback>::iterator it =
      callbacks_.find(request_identifier);
  std::move(it->second).Run(rv);
}

void BraveNetworkDelegateBase::RunNextCallback(
    URLRequest* request,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!base::Contains(callbacks_, ctx->request_identifier)) {
    return;
  }

  if (request->status().status() == net::URLRequestStatus::CANCELED) {
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
          base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
                     base::Unretained(this), request, ctx);
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
          base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
                     base::Unretained(this), request, ctx);
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
          base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
                     base::Unretained(this), request, ctx);
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

  net::CompletionOnceCallback wrapped_callback =
      base::BindOnce(&BraveNetworkDelegateBase::RunCallbackForRequestIdentifier,
                     base::Unretained(this), ctx->request_identifier);

  if (ctx->event_type == brave::kOnBeforeRequest) {
    if (!ctx->new_url_spec.empty() &&
        (ctx->new_url_spec != ctx->request_url.spec()) &&
        IsRequestIdentifierValid(ctx->request_identifier)) {
      *ctx->new_url = GURL(ctx->new_url_spec);
    }
    if (ctx->blocked_by == brave::kAdBlocked) {
      // We are going to intercept this request and block it later in the
      // network stack.
      if (ctx->cancel_request_explicitly) {
        RunCallbackForRequestIdentifier(ctx->request_identifier,
            net::ERR_ABORTED);
        return;
      }
      request->SetExtraRequestHeaderByName("X-Brave-Block", "", true);
    }
    if (!ctx->new_referrer.is_empty()) {
      request->SetReferrer(ctx->new_referrer.spec());
    }
    rv = ChromeNetworkDelegate::OnBeforeURLRequest(
        request, std::move(wrapped_callback), ctx->new_url);
  } else if (ctx->event_type == brave::kOnBeforeStartTransaction) {
    rv = ChromeNetworkDelegate::OnBeforeStartTransaction(
        request, std::move(wrapped_callback), ctx->headers);
  } else if (ctx->event_type == brave::kOnHeadersReceived) {
    rv = ChromeNetworkDelegate::OnHeadersReceived(
        request, std::move(wrapped_callback), ctx->original_response_headers,
        ctx->override_response_headers, ctx->allowed_unsafe_redirect_url);
  }

  // ChromeNetworkDelegate returns net::ERR_IO_PENDING if an extension is
  // intercepting the request and OK if the request should proceed normally.
  if (rv != net::ERR_IO_PENDING) {
    RunCallbackForRequestIdentifier(ctx->request_identifier, rv);
  }
}

void BraveNetworkDelegateBase::OnURLRequestDestroyed(URLRequest* request) {
  if (base::Contains(callbacks_, request->identifier())) {
    callbacks_.erase(request->identifier());
  }
  ChromeNetworkDelegate::OnURLRequestDestroyed(request);
}

bool BraveNetworkDelegateBase::IsRequestIdentifierValid(
    uint64_t request_identifier) {
  return base::Contains(callbacks_, request_identifier);
}
