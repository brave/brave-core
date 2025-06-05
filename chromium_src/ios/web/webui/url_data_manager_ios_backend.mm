/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/webui/url_data_manager_ios_backend.h"

#include "ios/web/public/browser_state.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/url_request/url_request.h"

namespace web {
template <typename URLRequestChromeJob>
class BraveURLRequestJob : public URLRequestChromeJob {
 public:
  BraveURLRequestJob(net::URLRequest* request,
                     web::BrowserState* browser_state,
                     bool is_incognito);

  void GetResponseInfo(net::HttpResponseInfo* info) override;
};

template <typename ProtocolHandler>
class BraveProtocolHandler : public ProtocolHandler {
 public:
  BraveProtocolHandler(web::BrowserState* browser_state, bool is_incognito);

  std::unique_ptr<net::URLRequestJob> CreateJob(
      net::URLRequest* request) const override;

  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  raw_ptr<web::BrowserState> browser_state_;
  const bool is_incognito_;
};
}  // namespace web

#define ShouldDenyXFrameOptions ShouldDenyXFrameOptions());  \
  job->set_content_security_policy_frame_source(             \
      source->source()->GetContentSecurityPolicyFrameSrc()); \
        void(void

#define CreateProtocolHandler                                              \
  CreateProtocolHandler(BrowserState* browser_state) {                     \
    DCHECK(browser_state);                                                 \
    return std::make_unique<                                               \
        BraveProtocolHandler<net::URLRequestJobFactory::ProtocolHandler>>( \
        browser_state, browser_state->IsOffTheRecord());                   \
  }                                                                        \
                                                                           \
  std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>              \
      URLDataManagerIOSBackend::CreateProtocolHandler_ChromiumImpl

#define is_incognito()             \
  dummy_;                          \
  template <typename T>            \
  friend class BraveURLRequestJob; \
  bool is_incognito()

#include "src/ios/web/webui/url_data_manager_ios_backend.mm"

#undef is_incognito
#undef CreateProtocolHandler
#undef ShouldDenyXFrameOptions

namespace web {
template <typename URLRequestChromeJob>
BraveURLRequestJob<URLRequestChromeJob>::BraveURLRequestJob(
    net::URLRequest* request,
    web::BrowserState* browser_state,
    bool is_incognito)
    : URLRequestChromeJob(request, browser_state, is_incognito) {}

template <typename URLRequestChromeJob>
void BraveURLRequestJob<URLRequestChromeJob>::GetResponseInfo(
    net::HttpResponseInfo* info) {
  bool should_set_csp = this->add_content_security_policy_;
  set_add_content_security_policy(false);
  URLRequestChromeJob::(info);
  set_add_content_security_policy(should_set_csp);

  DCHECK(!info->headers.get());

  if (should_set_csp) {
    std::string base;
    base.append(this->content_security_policy_object_source_);
    base.append(this->content_security_policy_frame_source_);
    info->headers->AddHeader(web::kContentSecurityPolicy, base);
  }
}

template <typename ProtocolHandler>
BraveProtocolHandler<ProtocolHandler>::BraveProtocolHandler(
    web::BrowserState* browser_state,
    bool is_incognito)
    : browser_state_(browser_state), is_incognito_(is_incognito) {}

template <typename ProtocolHandler>
std::unique_ptr<net::URLRequestJob>
BraveProtocolHandler<ProtocolHandler>::CreateJob(
    net::URLRequest* request) const {
  DCHECK(request);

  return std::make_unique<BraveURLRequestJob<web::URLRequestChromeJob>>(
      request, browser_state_, is_incognito_);
}

template <typename ProtocolHandler>
bool BraveProtocolHandler<ProtocolHandler>::IsSafeRedirectTarget(
    const GURL& location) const {
  return false;
}

}  // namespace web
