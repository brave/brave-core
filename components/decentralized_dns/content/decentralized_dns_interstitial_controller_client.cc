/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/content/decentralized_dns_interstitial_controller_client.h"

#include "base/notreached.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/pref_names.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/settings_page_helper.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "content/public/browser/web_contents.h"

namespace decentralized_dns {

// static
std::unique_ptr<security_interstitials::MetricsHelper>
DecentralizedDnsInterstitialControllerClient::GetMetricsHelper(
    const GURL& url) {
  security_interstitials::MetricsHelper::ReportDetails report_details;
  report_details.metric_prefix = "DecentralizedDns";

  return std::make_unique<security_interstitials::MetricsHelper>(
      url, report_details, nullptr);
}

DecentralizedDnsInterstitialControllerClient::
    DecentralizedDnsInterstitialControllerClient(
        content::WebContents* web_contents,
        const GURL& request_url,
        PrefService* user_prefs,
        PrefService* local_state,
        const std::string& locale)
    : security_interstitials::SecurityInterstitialControllerClient(
          web_contents,
          GetMetricsHelper(request_url),
          user_prefs,
          locale,
          GURL("about:blank") /* default_safe_page */,
          nullptr /* settings_page_helper */),
      request_url_(request_url),
      local_state_(local_state) {}

void DecentralizedDnsInterstitialControllerClient::Proceed() {
  SetResolveMethodAndReload(ResolveMethodTypes::ENABLED);
}

void DecentralizedDnsInterstitialControllerClient::DontProceed() {
  SetResolveMethodAndReload(ResolveMethodTypes::DISABLED);
}

void DecentralizedDnsInterstitialControllerClient::SetResolveMethodAndReload(
    ResolveMethodTypes type) {
  DCHECK(local_state_);
  const char* pref_name = nullptr;
  if (IsUnstoppableDomainsTLD(request_url_.host_piece())) {
    pref_name = kUnstoppableDomainsResolveMethod;
  } else if (IsENSTLD(request_url_.host_piece())) {
    pref_name = kENSResolveMethod;
  } else if (IsSnsTLD(request_url_.host_piece())) {
    pref_name = kSnsResolveMethod;
  } else {
    NOTREACHED();
  }

  local_state_->SetInteger(pref_name, static_cast<int>(type));
  web_contents_->GetController().Reload(content::ReloadType::BYPASSING_CACHE,
                                        true);
}

}  // namespace decentralized_dns
