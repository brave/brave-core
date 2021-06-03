/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_interstitial_controller_client.h"

#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/settings_page_helper.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace ipfs {

// static
std::unique_ptr<security_interstitials::MetricsHelper>
IPFSInterstitialControllerClient::GetMetricsHelper(const GURL& url) {
  security_interstitials::MetricsHelper::ReportDetails report_details;
  report_details.metric_prefix = "ipfs";

  return std::make_unique<security_interstitials::MetricsHelper>(
      url, report_details, nullptr);
}

IPFSInterstitialControllerClient::IPFSInterstitialControllerClient(
    content::WebContents* web_contents,
    const GURL& request_url,
    PrefService* prefs,
    const std::string& locale)
    : security_interstitials::SecurityInterstitialControllerClient(
          web_contents,
          GetMetricsHelper(request_url),
          prefs,
          locale,
          GURL("about:blank") /* default_safe_page */,
          nullptr /* settings_page_helper */),
      request_url_(request_url) {}

void IPFSInterstitialControllerClient::Proceed() {
  PrefService* prefs = GetPrefService();
  DCHECK(prefs);
  prefs->SetBoolean(kIPFSAutoFallbackToGateway, true);

  GURL url = ToPublicGatewayURL(request_url_, prefs);
  DCHECK(!url.is_empty());

  content::OpenURLParams params(url, content::Referrer(),
                                WindowOpenDisposition::CURRENT_TAB,
                                ui::PAGE_TRANSITION_LINK, false);
  web_contents_->OpenURL(params);
}

}  // namespace ipfs
