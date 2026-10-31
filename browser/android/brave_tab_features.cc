// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/android/brave_tab_features.h"

#include "base/feature_list.h"
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/browser/ai_chat/tab_data_web_contents_observer.h"
#include "brave/browser/ai_chat/web_mcp_injection/web_mcp_injector.h"
#include "brave/browser/brave_tab_helpers.h"
#include "brave/browser/image_metadata_stripper/file_upload_controller.h"
#include "brave/browser/misc_metrics/captcha_metrics.h"
#include "brave/browser/misc_metrics/page_metrics_tab_helper.h"
#include "brave/browser/serp_metrics/serp_metrics_tab_helper.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/misc_metrics/features.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/profiles/profile.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/browser/brave_ads/creatives/search_result_ad/creative_search_result_ad_tab_helper.h"
#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
#include "brave/browser/web_discovery/web_discovery_tab_helper.h"
#endif

namespace tabs {

BraveTabFeatures::BraveTabFeatures(content::WebContents* web_contents,
                                   Profile* profile)
    : TabFeatures_Chromium(web_contents, profile) {
  brave::AttachTabHelpers(web_contents);

  // Clients should rely on this for observing web contents based changes.
  tabs::TabInterface& tab_interface =
      *TabInterface::GetFromContents(web_contents);

  if (ai_chat::IsAllowedForContext(profile)) {
    tab_data_observer_ = std::make_unique<ai_chat::TabDataWebContentsObserver>(
        TabAndroid::FromWebContents(web_contents)->GetAndroidId(),
        tab_interface);
    // Injects Brave-provided WebMCP tools into matching pages; see
    // WebMcpInjector. Null when WebMCP is disabled or has no rules.
    web_mcp_injector_ = ai_chat::WebMcpInjector::MaybeCreate(tab_interface);
  }

  if (base::FeatureList::IsEnabled(
          misc_metrics::features::kCaptchaMetricsCollection)) {
    cloudflare_js_detection_tab_helper_ = misc_metrics::CaptchaMetrics::
        CloudflareJsDetectionTabHelper::MaybeCreate(tab_interface);
  }

  if (base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripImageMetadataV1)) {
    stripper_upload_controller_ =
        image_metadata_stripper::FileUploadController::MaybeCreate(
            tab_interface);
  }

  page_metrics_tab_helper_ =
      std::make_unique<misc_metrics::PageMetricsTabHelper>(tab_interface);
  serp_metrics_tab_helper_ =
      serp_metrics::SerpMetricsTabHelper::MaybeCreate(tab_interface);

#if BUILDFLAG(ENABLE_BRAVE_ADS)
  ads_tab_helper_ = std::make_unique<brave_ads::AdsTabHelper>(tab_interface);
  creative_search_result_ad_tab_helper_ =
      brave_ads::CreativeSearchResultAdTabHelper::MaybeCreate(tab_interface);
#endif

#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  web_discovery_tab_helper_ = web_discovery::WebDiscoveryTabHelper::MaybeCreate(
      tab_interface, *profile);
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  rewards_tab_helper_ =
      std::make_unique<brave_rewards::RewardsTabHelper>(tab_interface);
#endif
}

BraveTabFeatures::~BraveTabFeatures() = default;

TabFeatures::TabFeatures(content::WebContents* web_contents, Profile* profile)
    : brave_tab_features_(
          std::make_unique<BraveTabFeatures>(web_contents, profile)) {}

TabFeatures::~TabFeatures() = default;

NewTabPagePreloadPipelineManager*
TabFeatures::new_tab_page_preload_pipeline_manager() {
  return brave_tab_features_->new_tab_page_preload_pipeline_manager();
}

enterprise_data_protection::DataProtectionNavigationController*
TabFeatures::data_protection_controller() {
  return brave_tab_features_->data_protection_controller();
}

tasks::TaskTabHelper* TabFeatures::task_tab_helper() {
  return brave_tab_features_->task_tab_helper();
}

SupervisedUserNavigationObserver*
TabFeatures::supervised_user_navigation_observer() {
  return brave_tab_features_->supervised_user_navigation_observer();
}

// static
BraveTabFeatures* BraveTabFeatures::FromTabFeatures(TabFeatures* tab_features) {
  return static_cast<BraveTabFeatures*>(
      tab_features->brave_tab_features_.get());
}

}  // namespace tabs
