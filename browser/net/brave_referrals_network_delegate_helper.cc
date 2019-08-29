/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_referrals_network_delegate_helper.h"

#include "base/values.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/common/network_constants.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"

namespace brave {

int OnBeforeStartTransaction_ReferralsWork(
    net::URLRequest* request,
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  if (!ctx->referral_headers_list)
    return net::OK;
  // If the domain for this request matches one of our target domains,
  // set the associated custom headers.
  const base::DictionaryValue* request_headers_dict = nullptr;
  if (!BraveReferralsService::GetMatchingReferralHeaders(
          *ctx->referral_headers_list, &request_headers_dict, request->url()))
    return net::OK;
  for (const auto& it : request_headers_dict->DictItems()) {
    if (it.first == kBravePartnerHeader) {
      headers->SetHeader(it.first, it.second.GetString());
      ctx->set_headers.insert(it.first);
    }
  }
  return net::OK;
}

}  // namespace brave
