/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_controller_client.h"

#include "brave/components/request_otr/browser/request_otr_service.h"
#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/settings_page_helper.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

using content::BrowserContext;

namespace request_otr {

// static
std::unique_ptr<security_interstitials::MetricsHelper>
RequestOTRControllerClient::GetMetricsHelper(const GURL& url) {
  security_interstitials::MetricsHelper::ReportDetails report_details;
  report_details.metric_prefix = "request_otr";

  return std::make_unique<security_interstitials::MetricsHelper>(
      url, report_details, nullptr);
}

RequestOTRControllerClient::RequestOTRControllerClient(
    content::WebContents* web_contents,
    const GURL& request_url,
    ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
    PrefService* prefs,
    const std::string& locale)
    : security_interstitials::SecurityInterstitialControllerClient(
          web_contents,
          GetMetricsHelper(request_url),
          prefs,
          locale,
          GURL("about:blank") /* default_safe_page */,
          nullptr /* settings_page_helper */),
      request_url_(request_url),
      dont_warn_again_(false),
      ephemeral_storage_service_(ephemeral_storage_service) {}

RequestOTRControllerClient::~RequestOTRControllerClient() = default;

void RequestOTRControllerClient::GoBack() {
  SecurityInterstitialControllerClient::GoBackAfterNavigationCommitted();
}

void RequestOTRControllerClient::Proceed() {
  RequestOTRStorageTabHelper* tab_storage =
      RequestOTRStorageTabHelper::GetOrCreate(web_contents_);
  tab_storage->set_is_proceeding(true);
  if (dont_warn_again_) {
    if (PrefService* prefs = GetPrefService()) {
      prefs->SetInteger(
          kRequestOTRActionOption,
          static_cast<int>(RequestOTRService::RequestOTRActionOption::kNever));
    }
  }

  ReloadPage();
}

void RequestOTRControllerClient::ProceedOTR() {
  RequestOTRStorageTabHelper* tab_storage =
      RequestOTRStorageTabHelper::GetOrCreate(web_contents_);
  tab_storage->set_is_proceeding(true);
  tab_storage->set_requested_otr(true);
  if (dont_warn_again_) {
    if (PrefService* prefs = GetPrefService()) {
      prefs->SetInteger(
          kRequestOTRActionOption,
          static_cast<int>(RequestOTRService::RequestOTRActionOption::kNever));
    }
  }
  tab_storage->MaybeEnable1PESForUrl(
      ephemeral_storage_service_, request_url_,
      base::BindOnce(&RequestOTRControllerClient::ReloadPage,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RequestOTRControllerClient::ReloadPage() {
  web_contents_->GetController().Reload(content::ReloadType::NORMAL, false);
}

void RequestOTRControllerClient::SetDontWarnAgain(bool value) {
  dont_warn_again_ = value;
}

}  // namespace request_otr
