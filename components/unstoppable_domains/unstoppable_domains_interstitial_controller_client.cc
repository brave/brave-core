/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/unstoppable_domains/unstoppable_domains_interstitial_controller_client.h"

#include "brave/components/unstoppable_domains/constants.h"
#include "brave/components/unstoppable_domains/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/settings_page_helper.h"
#include "components/security_interstitials/core/metrics_helper.h"

namespace unstoppable_domains {

// static
std::unique_ptr<security_interstitials::MetricsHelper>
UnstoppableDomainsInterstitialControllerClient::GetMetricsHelper(
    const GURL& url) {
  security_interstitials::MetricsHelper::ReportDetails report_details;
  report_details.metric_prefix = "UnstoppableDomains";

  return std::make_unique<security_interstitials::MetricsHelper>(
      url, report_details, nullptr);
}

UnstoppableDomainsInterstitialControllerClient::
    UnstoppableDomainsInterstitialControllerClient(
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

void UnstoppableDomainsInterstitialControllerClient::Proceed() {
  SetResolveMethodAndReload(ResolveMethodTypes::DNS_OVER_HTTPS);
}

void UnstoppableDomainsInterstitialControllerClient::DontProceed() {
  SetResolveMethodAndReload(ResolveMethodTypes::DISABLED);
}

void UnstoppableDomainsInterstitialControllerClient::SetResolveMethodAndReload(
    ResolveMethodTypes type) {
  DCHECK(local_state_);
  local_state_->SetInteger(kResolveMethod, static_cast<int>(type));
  Reload();
}

}  // namespace unstoppable_domains
