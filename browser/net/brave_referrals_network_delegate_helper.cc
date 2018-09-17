/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_referrals_network_delegate_helper.h"

#include "base/values.h"
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
  for (const auto& headers_value : *ctx->referral_headers_list) {
    const base::Value* domains_list =
        headers_value.FindKeyOfType("domains", base::Value::Type::LIST);
    if (!domains_list) {
      LOG(WARNING) << "Failed to retrieve 'domains' key from referral headers";
      continue;
    }
    const base::Value* headers_dict =
        headers_value.FindKeyOfType("headers", base::Value::Type::DICTIONARY);
    if (!headers_dict) {
      LOG(WARNING) << "Failed to retrieve 'headers' key from referral headers";
      continue;
    }
    for (const auto& domain_value : domains_list->GetList()) {
      URLPattern url_pattern(URLPattern::SCHEME_HTTPS |
                             URLPattern::SCHEME_HTTP);
      url_pattern.SetScheme("*");
      url_pattern.SetHost(domain_value.GetString());
      url_pattern.SetPath("/*");
      url_pattern.SetMatchSubdomains(true);
      if (!url_pattern.MatchesURL(request->url()))
        continue;
      for (const auto& it : headers_dict->DictItems()) {
        headers->SetHeader(it.first, it.second.GetString());
      }
      return net::OK;
    }
  }

  return net::OK;
}

}  // namespace brave
