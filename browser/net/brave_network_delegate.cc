/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_network_delegate.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request.h"


struct OnBeforeURLRequestContext {
  OnBeforeURLRequestContext() {}
  ~OnBeforeURLRequestContext() {}
  GURL request_url;
  std::string new_url_spec;
  uint64_t request_identifier = 0;

  DISALLOW_COPY_AND_ASSIGN(OnBeforeURLRequestContext);
};

class PendingRequests {
public:
  void Insert(const uint64_t &request_identifier) {
    pending_requests_.insert(request_identifier);
  }
  void Destroy(const uint64_t &request_identifier) {
    pending_requests_.erase(request_identifier);
  }
  bool IsPendingAndAlive(const uint64_t &request_identifier) {
    bool isPending = pending_requests_.find(request_identifier) !=
      pending_requests_.end();
    return isPending;
  }
private:
  std::set<uint64_t> pending_requests_;
  // No need synchronization, should be executed in the same
  // thread content::BrowserThread::IO
};

BraveNetworkDelegate::BraveNetworkDelegate(
    extensions::EventRouterForwarder* event_router,
    BooleanPrefMember* enable_referrers) :
    ChromeNetworkDelegate(event_router, enable_referrers) {
  pending_requests_.reset(new PendingRequests());
}

BraveNetworkDelegate::~BraveNetworkDelegate() {
}

int BraveNetworkDelegate::OnBeforeURLRequest(net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {

  GURL tab_origin;
  if (!brave_shields::GetTabOrigin(request, &tab_origin)) {
    return ChromeNetworkDelegate::OnBeforeURLRequest(request,
        callback, new_url);
  }

  Profile* profile = ProfileManager::GetActiveUserProfile();
  auto* resource_context = profile->GetResourceContext();
  bool allow_https_everywhere = brave_shields::IsAllowContentSetting(
      resource_context, tab_origin,
      CONTENT_SETTINGS_TYPE_BRAVEHTTPSEVERYWHERE);

  if (!allow_https_everywhere) {
    return ChromeNetworkDelegate::OnBeforeURLRequest(request,
        callback, new_url);
  }

  std::shared_ptr<OnBeforeURLRequestContext> ctx(
      new OnBeforeURLRequestContext());
  if (request) {
    ctx->request_identifier = request->identifier();
  }
  return OnBeforeURLRequest_HttpsePreFileWork(request, callback, new_url, ctx);
}

int BraveNetworkDelegate::OnBeforeURLRequest_HttpsePreFileWork(
  net::URLRequest* request,
  const net::CompletionCallback& callback,
  GURL* new_url,
  std::shared_ptr<OnBeforeURLRequestContext> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  bool isValidURL = true;
  if (request) {
    isValidURL = request->url().is_valid();
    std::string scheme = request->url().scheme();
    if (scheme.length()) {
      std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
      if ("http" != scheme && "https" != scheme) {
        isValidURL = false;
      }
    }
  }

  if (isValidURL) {
    ctx->new_url_spec = g_browser_process->https_everywhere_service()
      ->GetHTTPSURLFromCacheOnly(&request->url(), request->identifier());
    if (ctx->new_url_spec == request->url().spec()) {
      ctx->request_url = request->url();
      content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::FILE, FROM_HERE,
        base::Bind(&BraveNetworkDelegate::OnBeforeURLRequest_HttpseFileWork,
            base::Unretained(this), base::Unretained(request), ctx),
        base::Bind(base::IgnoreResult(
            &BraveNetworkDelegate::OnBeforeURLRequest_HttpsePostFileWork),
            base::Unretained(this), base::Unretained(request),
            callback, new_url, ctx)
          );
      pending_requests_->Insert(request->identifier());
      return net::ERR_IO_PENDING;
    } else {
      *new_url = GURL(ctx->new_url_spec);
    }
  }

  return ChromeNetworkDelegate::OnBeforeURLRequest(request, callback, new_url);
}

void BraveNetworkDelegate::OnBeforeURLRequest_HttpseFileWork(
    net::URLRequest* request, std::shared_ptr<OnBeforeURLRequestContext> ctx) {
  base::ThreadRestrictions::AssertIOAllowed();
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);
  DCHECK(ctx->request_identifier != 0);
  ctx->new_url_spec = g_browser_process->https_everywhere_service()->
    GetHTTPSURL(&ctx->request_url, ctx->request_identifier);
}

int BraveNetworkDelegate::OnBeforeURLRequest_HttpsePostFileWork(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url,
    std::shared_ptr<OnBeforeURLRequestContext> ctx) {

  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (PendedRequestIsDestroyedOrCancelled(ctx.get(), request)) {
    return net::OK;
  }

  if (!ctx->new_url_spec.empty() &&
    ctx->new_url_spec != request->url().spec()) {
    *new_url = GURL(ctx->new_url_spec);
    brave_shields::DispatchBlockedEvent("httpsEverywhere", request);
  }

  int rv =
    ChromeNetworkDelegate::OnBeforeURLRequest(request, callback, new_url);
  if (rv != net::ERR_IO_PENDING) {
    callback.Run(rv);
  }
  return rv;
}

bool BraveNetworkDelegate::PendedRequestIsDestroyedOrCancelled(
    OnBeforeURLRequestContext* ctx, net::URLRequest* request) {

  if (!pending_requests_->IsPendingAndAlive(ctx->request_identifier)
    || request->status().status() == net::URLRequestStatus::CANCELED) {
    return true;
  }

  return false;
}
