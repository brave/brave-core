/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_tab_helper.h"

#include "base/check.h"
#include "base/check_deref.h"
#include "base/feature_list.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/serp_metrics_service.h"
#include "brave/components/serp_metrics/navigation_tracker/navigation_tracker.h"
#include "brave/ios/browser/serp_metrics/serp_metrics_service_factory_ios.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/web_state.h"
#include "ui/base/page_transition_types.h"

namespace serp_metrics {

namespace {

bool IsAllowedToSendUsagePings() {
  return GetApplicationContext()->GetLocalState()->GetBoolean(
      kStatsReportingEnabled);
}

}  // namespace

SerpMetricsTabHelper::~SerpMetricsTabHelper() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
  }
}

// static
void SerpMetricsTabHelper::MaybeCreateForWebState(web::WebState* web_state) {
  CHECK(web_state);
  CHECK(base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature));

  SerpMetricsService* serp_metrics_service =
      SerpMetricsServiceFactoryIOS::GetForProfile(
          ProfileIOS::FromBrowserState(web_state->GetBrowserState()));
  if (!serp_metrics_service) {
    // `SerpMetricsService` is null for off-the-record profiles.
    return;
  }

  SerpMetrics* serp_metrics = serp_metrics_service->Get();
  CreateForWebState(web_state, CHECK_DEREF(serp_metrics));
}

///////////////////////////////////////////////////////////////////////////////

SerpMetricsTabHelper::SerpMetricsTabHelper(web::WebState* web_state,
                                           SerpMetrics& serp_metrics)
    : web_state_(web_state),
      serp_metrics_(serp_metrics),
      navigation_tracker_(
          std::make_unique<SerpMetricsNavigationTracker>(serp_metrics)) {
  CHECK(web_state_);
  web_state_->AddObserver(this);
}

void SerpMetricsTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!IsAllowedToSendUsagePings()) {
    // The user has opted out of usage pings, so SERP metrics are not collected.
    return;
  }

  if (!navigation_context->HasCommitted()) {
    return;
  }

  const bool is_new_navigation = ui::PageTransitionIsNewNavigation(
      navigation_context->GetPageTransition());

  navigation_tracker_->OnNavigationFinished(navigation_context->GetUrl(),
                                            is_new_navigation);
}

void SerpMetricsTabHelper::WebStateDestroyed(web::WebState* web_state) {
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

}  // namespace serp_metrics
