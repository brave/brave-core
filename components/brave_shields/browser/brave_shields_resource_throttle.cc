/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_resource_throttle.h"
#include "net/url_request/url_request.h"
#include "chrome/browser/browser_process.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"

content::ResourceThrottle* MaybeCreateBraveShieldsResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type) {
  return new BraveShieldsResourceThrottle(request, resource_type);
}

BraveShieldsResourceThrottle::BraveShieldsResourceThrottle(
    const net::URLRequest* request,
    content::ResourceType resource_type) {
  resource_type_ = resource_type;
  request_ = request;
}

BraveShieldsResourceThrottle::~BraveShieldsResourceThrottle() = default;

const char* BraveShieldsResourceThrottle::GetNameForLogging() const {
  return "BraveShieldsResourceThrottle";
}


void BraveShieldsResourceThrottle::WillStartRequest(bool* defer) {
  if (g_browser_process->ad_block_service()->Check(request_->url().spec(),
      resource_type_,
      request_->initiator()->host())) {
    Cancel();
  }
}

void BraveShieldsResourceThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
}
