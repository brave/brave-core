/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/content/ens_offchain_lookup_interstitial_controller_client.h"

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
EnsOffchainLookupInterstitialControllerClient::GetMetricsHelper(
    const GURL& url) {
  security_interstitials::MetricsHelper::ReportDetails report_details;
  report_details.metric_prefix = "DecentralizedDns";

  return std::make_unique<security_interstitials::MetricsHelper>(
      url, report_details, nullptr);
}

EnsOffchainLookupInterstitialControllerClient::
    EnsOffchainLookupInterstitialControllerClient(
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

void EnsOffchainLookupInterstitialControllerClient::Proceed() {
  SetResolveMethodAndReload(EnsOffchainResolveMethod::kEnabled);
}

void EnsOffchainLookupInterstitialControllerClient::DontProceed() {
  SetResolveMethodAndReload(EnsOffchainResolveMethod::kDisabled);
}

void EnsOffchainLookupInterstitialControllerClient::SetResolveMethodAndReload(
    EnsOffchainResolveMethod type) {
  DCHECK(local_state_);
  SetEnsOffchainResolveMethod(local_state_, type);
  web_contents_->GetController().Reload(content::ReloadType::BYPASSING_CACHE,
                                        true);
}

}  // namespace decentralized_dns
