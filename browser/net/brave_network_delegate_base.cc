/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_network_delegate_base.h"

#include <algorithm>

#include "base/task/post_task.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;
using net::URLRequest;


namespace {

  content::WebContents* GetWebContentsFromProcessAndFrameId(
      int render_process_id, int render_frame_id) {
  if (render_process_id) {
    content::RenderFrameHost* rfh =
        content::RenderFrameHost::FromID(render_process_id, render_frame_id);
    return content::WebContents::FromRenderFrameHost(rfh);
  }
  return content::WebContents::FromFrameTreeNodeId(render_frame_id);
}

}  // namespace

BraveNetworkDelegateBase::BraveNetworkDelegateBase(
    extensions::EventRouterForwarder* event_router)
    : ChromeNetworkDelegate(event_router), referral_headers_list_(nullptr) {
  // Retrieve the current referral headers, if any.
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::UI},
      base::Bind(&BraveNetworkDelegateBase::GetReferralHeaders,
                 base::Unretained(this)));
}

BraveNetworkDelegateBase::~BraveNetworkDelegateBase() {
}

void BraveNetworkDelegateBase::GetReferralHeaders() {
  const base::ListValue* referral_headers =
      g_browser_process->local_state()->GetList(kReferralHeaders);
  if (referral_headers)
    referral_headers_list_ = referral_headers->CreateDeepCopy();
}

int BraveNetworkDelegateBase::OnBeforeURLRequest(URLRequest* request,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  if (before_url_request_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnBeforeURLRequest(request, std::move(callback), new_url);
  }
  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request, ctx);
  ctx->new_url = new_url;
  ctx->event_type = brave::kOnBeforeRequest;
  callbacks_[request->identifier()] = std::move(callback);
  RunNextCallback(request, ctx);
  return net::ERR_IO_PENDING;
}

int BraveNetworkDelegateBase::OnBeforeStartTransaction(URLRequest* request,
    net::CompletionOnceCallback callback,
    net::HttpRequestHeaders* headers) {
  if (before_start_transaction_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnBeforeStartTransaction(request, std::move(callback),
                                                           headers);
  }
  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request, ctx);
  ctx->event_type = brave::kOnBeforeStartTransaction;
  ctx->headers = headers;
  ctx->referral_headers_list = referral_headers_list_.get();
  callbacks_[request->identifier()] = std::move(callback);
  RunNextCallback(request, ctx);
  return net::ERR_IO_PENDING;
}

int BraveNetworkDelegateBase::OnHeadersReceived(URLRequest* request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) {
  if (headers_received_callbacks_.empty() || !request) {
    return ChromeNetworkDelegate::OnHeadersReceived(request,
        std::move(callback), original_response_headers,
        override_response_headers, allowed_unsafe_redirect_url);
  }

  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
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

bool BraveNetworkDelegateBase::OnCanGetCookies(const URLRequest& request,
    const net::CookieList& cookie_list,
    bool allowed_from_caller) {
  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(&request, ctx);
  ctx->event_type = brave::kOnCanGetCookies;
  bool allow = std::all_of(can_get_cookies_callbacks_.begin(), can_get_cookies_callbacks_.end(),
      [&ctx](brave::OnCanGetCookiesCallback callback){
        return callback.Run(ctx);
      });

  int frame_id;
  int process_id;
  int frame_tree_node_id;
  brave_shields::GetRenderFrameInfo(&request, &frame_id, &process_id,
      &frame_tree_node_id);
  base::RepeatingCallback<content::WebContents*(void)> wc_getter =
      base::BindRepeating(&GetWebContentsFromProcessAndFrameId, process_id,
                          frame_id);
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::UI},
      base::BindOnce(&TabSpecificContentSettings::CookiesRead, wc_getter,
          request.url(), request.site_for_cookies(), cookie_list,
          !allow));

  return allow;
}

bool BraveNetworkDelegateBase::OnCanSetCookie(const URLRequest& request,
    const net::CanonicalCookie& cookie,
    net::CookieOptions* options,
    bool allowed_from_caller) {
  std::shared_ptr<brave::BraveRequestInfo> ctx(
      new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(&request, ctx);
  ctx->event_type = brave::kOnCanSetCookies;
  bool allow = std::all_of(can_set_cookies_callbacks_.begin(), can_set_cookies_callbacks_.end(),
      [&ctx](brave::OnCanSetCookiesCallback callback){
        return callback.Run(ctx);
      });

  int frame_id;
  int process_id;
  int frame_tree_node_id;
  brave_shields::GetRenderFrameInfo(&request, &frame_id, &process_id,
      &frame_tree_node_id);
  base::RepeatingCallback<content::WebContents*(void)> wc_getter =
      base::BindRepeating(&GetWebContentsFromProcessAndFrameId, process_id,
                          frame_id);
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::UI},
      base::BindOnce(&TabSpecificContentSettings::CookieChanged, wc_getter,
          request.url(), request.site_for_cookies(), cookie,
          !allow));

  return allow;
}

void BraveNetworkDelegateBase::RunCallbackForRequestIdentifier(uint64_t request_identifier, int rv) {
  std::map<uint64_t, net::CompletionOnceCallback>::iterator it =
      callbacks_.find(request_identifier);
  std::move(it->second).Run(rv);
}

void BraveNetworkDelegateBase::RunNextCallback(
    URLRequest* request,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!ContainsKey(callbacks_, ctx->request_identifier)) {
    return;
  }

  if (request->status().status() == net::URLRequestStatus::CANCELED) {
    return;
  }

  // Continue processing callbacks until we hit one that returns PENDING
  int rv = net::OK;

  if (ctx->event_type == brave::kOnBeforeRequest) {
    while(before_url_request_callbacks_.size() != ctx->next_url_request_index) {
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
    while(before_start_transaction_callbacks_.size() != ctx->next_url_request_index) {
      brave::OnBeforeStartTransactionCallback callback =
          before_start_transaction_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
              base::Unretained(this), request, ctx);
      rv = callback.Run(request, ctx->headers, next_callback, ctx);
      if (rv == net::ERR_IO_PENDING) {
        return;
      }
      if (rv != net::OK) {
        break;
      }
    }
  } else if (ctx->event_type == brave::kOnHeadersReceived) {
    while(headers_received_callbacks_.size() != ctx->next_url_request_index) {
      brave::OnHeadersReceivedCallback callback =
          headers_received_callbacks_[ctx->next_url_request_index++];
      brave::ResponseCallback next_callback =
          base::Bind(&BraveNetworkDelegateBase::RunNextCallback,
              base::Unretained(this), request, ctx);
      rv = callback.Run(request, ctx->original_response_headers,
          ctx->override_response_headers, ctx->allowed_unsafe_redirect_url,
          next_callback, ctx);
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

  net::CompletionOnceCallback wrapped_callback = base::BindOnce(
      &BraveNetworkDelegateBase::RunCallbackForRequestIdentifier, base::Unretained(this), ctx->request_identifier);

  if (ctx->event_type == brave::kOnBeforeRequest) {
    if (!ctx->new_url_spec.empty() &&
        (ctx->new_url_spec != ctx->request_url.spec() ||
          ctx->referrer_changed) &&
        IsRequestIdentifierValid(ctx->request_identifier)) {
      *ctx->new_url = GURL(ctx->new_url_spec);
    }
    rv = ChromeNetworkDelegate::OnBeforeURLRequest(request,
        std::move(wrapped_callback), ctx->new_url);
  } else if (ctx->event_type == brave::kOnBeforeStartTransaction) {
    rv = ChromeNetworkDelegate::OnBeforeStartTransaction(request,
        std::move(wrapped_callback), ctx->headers);
  } else if (ctx->event_type == brave::kOnHeadersReceived) {
    rv = ChromeNetworkDelegate::OnHeadersReceived(request,
        std::move(wrapped_callback), ctx->original_response_headers,
        ctx->override_response_headers, ctx->allowed_unsafe_redirect_url);
  }

  // ChromeNetworkDelegate returns net::ERR_IO_PENDING if an extension is
  // intercepting the request and OK if the request should proceed normally.
  if (rv != net::ERR_IO_PENDING) {
    RunCallbackForRequestIdentifier(ctx->request_identifier, rv);
  }
}

void BraveNetworkDelegateBase::OnURLRequestDestroyed(URLRequest* request) {
  if (ContainsKey(callbacks_, request->identifier())) {
    callbacks_.erase(request->identifier());
  }
  ChromeNetworkDelegate::OnURLRequestDestroyed(request);
}

bool BraveNetworkDelegateBase::IsRequestIdentifierValid(uint64_t request_identifier) {
  return ContainsKey(callbacks_, request_identifier);
}
