// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/public/brave_tab_features.h"

#include <memory>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/brave_tab_helpers.h"
#include "brave/browser/image_metadata_stripper/file_upload_controller.h"
#include "brave/browser/misc_metrics/captcha_metrics.h"
#include "brave/browser/misc_metrics/page_metrics_tab_helper.h"
#include "brave/browser/onboarding/onboarding_tab_helper.h"
#include "brave/browser/serp_metrics/serp_metrics_tab_helper.h"
#include "brave/browser/ui/side_panel/brave_side_panel_utils.h"
#include "brave/browser/ui/sidebar/sidebar_tab_helper.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/misc_metrics/features.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/page_action/action_ids.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/thumbnails/thumbnail_tab_helper.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/tabs/public/tab_interface.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/browser/brave_ads/creatives/search_result_ad/creative_search_result_ad_tab_helper.h"
#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "brave/browser/ui/geolocation/brave_geolocation_permission_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/brave_drm_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
#include "brave/browser/web_discovery/web_discovery_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#include "brave/browser/request_otr/request_otr_tab_helper.h"
#include "brave/components/request_otr/common/features.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "brave/browser/new_tab/background_color_tab_helper.h"
#include "brave/browser/ui/brave_ui_features.h"
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/container_tab_tracker.h"
#include "brave/browser/ui/views/page_action/partitioned_storage_page_action_controller.h"
#include "brave/components/containers/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/browser/ai_chat/tab_data_web_contents_observer.h"
#include "brave/browser/ai_chat/web_mcp_injection/web_mcp_injector.h"
#endif

#if BUILDFLAG(ENABLE_PSST)
#include "brave/browser/psst/psst_reporter_service_factory.h"
#include "brave/browser/psst/psst_settings_service_factory.h"
#include "brave/browser/psst/psst_tab_web_contents_observer.h"
#include "brave/browser/psst/psst_ui_delegate_impl.h"
#include "brave/browser/psst/psst_ui_desktop_presenter.h"
#include "brave/components/psst/core/common/features.h"
#include "chrome/browser/browser_process.h"
#include "components/variations/service/variations_service.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/speedreader/speedreader_tab_helper.h"
#include "brave/components/speedreader/common/features.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/views/page_action/onion_location_page_action_controller.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/ui/views/page_action/brave_news_page_action_controller.h"
#endif

namespace tabs {

// static
BraveTabFeatures* BraveTabFeatures::FromTabFeatures(TabFeatures* tab_features) {
  return static_cast<BraveTabFeatures*>(tab_features);
}

BraveTabFeatures::BraveTabFeatures() = default;
BraveTabFeatures::~BraveTabFeatures() = default;

void BraveTabFeatures::OnWillDiscardContents(
    TabInterface* tab,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  brave::AttachTabHelpers(new_contents);
}

void BraveTabFeatures::Init(TabInterface& tab, Profile* profile) {
  TabFeatures::Init(tab, profile);

  brave::AttachTabHelpers(tab.GetContents());
  // Upstream's discard path no longer calls `AttachTabHelpers` on the
  // replacement contents.
  will_discard_contents_subscription_ =
      tab.RegisterWillDiscardContents(base::BindRepeating(
          &BraveTabFeatures::OnWillDiscardContents, base::Unretained(this)));

  // Expect upstream's Init to create the registry.
  auto* side_panel_registry = SidePanelRegistry::From(&tab);
  CHECK(side_panel_registry);
  brave::RegisterContextualSidePanel(side_panel_registry, tab.GetContents());

  // `thumbnail_tab_helper_` is not instantiated because we disable
  // `kTabHoverCardImages`, however we do want `thumbnail_tab_helper_` to be
  // a valid instance, so we can switch hover modes via settings.
  if (!thumbnail_tab_helper_) {
    thumbnail_tab_helper_ =
        GetUserDataFactory().CreateInstance<ThumbnailTabHelper>(
            tab, tab, tab.GetContents());
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::IsAllowedForContext(profile)) {
    tab_data_observer_ = std::make_unique<ai_chat::TabDataWebContentsObserver>(
        tab.GetHandle().raw_value(), tab);
    // Injects Brave-provided WebMCP tools into matching pages; see
    // WebMcpInjector. Null when WebMCP is disabled or has no rules.
    web_mcp_injector_ = ai_chat::WebMcpInjector::MaybeCreate(tab);
  }
#endif

#if BUILDFLAG(ENABLE_PSST)
  // The PSST page action may be absent even when it exists (e.g. in unit
  // tests that don't register the browser's action items), so guard on
  // ActionExists() to avoid operating on an unregistered page action model.
  if (!profile->IsOffTheRecord() &&
      base::FeatureList::IsEnabled(psst::features::kEnablePsst) &&
      page_action_controller()->ActionExists(kActionShowPsstIcon)) {
    psst_action_controller_ =
        std::make_unique<page_actions::PsstActionController>(
            tab, *page_action_controller());
    auto* psst_settings_service =
        PsstSettingsServiceFactory::GetForProfile(profile);
    auto* variations_service = g_browser_process->variations_service();
    psst_web_contents_observer_ = psst::PsstTabWebContentsObserver::MaybeCreate(
        tab, profile,
        std::make_unique<psst::PsstUiDelegateImpl>(
            psst_settings_service,
            PsstReporterServiceFactory::GetForProfile(profile),
            profile->GetPrefs(),
            std::make_unique<psst::PsstUiDesktopPresenter>(
                tab, psst_action_controller_->AsWeakPtr())),
        psst_settings_service, variations_service,
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    container_tab_tracker_ = containers::ContainerTabTracker::MaybeCreate(tab);
    // The page action may be absent even when the controller exists (e.g. in
    // unit tests that don't register the browser's action items), so guard on
    // ActionExists() to avoid creating a controller that would operate on an
    // unregistered page action model.
    if (page_action_controller()->ActionExists(kActionShowPartitionedStorage)) {
      partitioned_storage_page_action_controller_ = std::make_unique<
          page_actions::PartitionedStoragePageActionController>(
          tab, *page_action_controller());
      partitioned_storage_page_action_controller_->Init();
    }
  }
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  // Created before the page action controller, which observes it.
  speedreader_tab_helper_ = speedreader::SpeedreaderTabHelper::MaybeCreate(tab);
  if (base::FeatureList::IsEnabled(
          speedreader::features::kSpeedreaderFeature) &&
      page_action_controller()->ActionExists(kActionShowSpeedreader)) {
    speedreader_page_action_controller_ =
        std::make_unique<page_actions::SpeedreaderPageActionController>(
            tab, *page_action_controller());
    speedreader_page_action_controller_->Init();
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  if (page_action_controller()->ActionExists(kActionShowWaybackMachine)) {
    wayback_machine_page_action_controller_ =
        std::make_unique<page_actions::WaybackMachinePageActionController>(
            tab, *page_action_controller());
    wayback_machine_page_action_controller_->Init();
  }
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (page_action_controller()->ActionExists(kActionShowPlaylistPageAction)) {
    playlist_page_action_controller_ =
        std::make_unique<page_actions::PlaylistPageActionController>(
            tab, *page_action_controller());
    playlist_page_action_controller_->Init();
  }
#endif

#if BUILDFLAG(ENABLE_TOR)
  if (page_action_controller()->ActionExists(kActionShowOnionLocation)) {
    onion_location_page_action_controller_ =
        std::make_unique<page_actions::OnionLocationPageActionController>(
            tab, *page_action_controller());
    onion_location_page_action_controller_->Init();
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  // Created before the page action controller, which observes it.
  brave_news_tab_helper_ = BraveNewsTabHelper::MaybeCreate(tab);
  if (!profile->IsOffTheRecord() &&
      page_action_controller()->ActionExists(kActionShowBraveNews)) {
    brave_news_page_action_controller_ =
        std::make_unique<page_actions::BraveNewsPageActionController>(
            tab, *page_action_controller());
    brave_news_page_action_controller_->Init();
  }
#endif

  if (base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripImageMetadataV1)) {
    stripper_upload_controller_ =
        image_metadata_stripper::FileUploadController::MaybeCreate(tab);
  }

  if (base::FeatureList::IsEnabled(
          misc_metrics::features::kCaptchaMetricsCollection)) {
    cloudflare_js_detection_tab_helper_ = misc_metrics::CaptchaMetrics::
        CloudflareJsDetectionTabHelper::MaybeCreate(tab);
  }

  page_metrics_tab_helper_ =
      std::make_unique<misc_metrics::PageMetricsTabHelper>(tab);

  sidebar_tab_helper_ = sidebar::SidebarTabHelper::MaybeCreate(tab);
  onboarding_tab_helper_ = OnboardingTabHelper::MaybeCreate(tab);
  serp_metrics_tab_helper_ =
      serp_metrics::SerpMetricsTabHelper::MaybeCreate(tab);

#if BUILDFLAG(ENABLE_BRAVE_ADS)
  ads_tab_helper_ = std::make_unique<brave_ads::AdsTabHelper>(tab);
  creative_search_result_ad_tab_helper_ =
      brave_ads::CreativeSearchResultAdTabHelper::MaybeCreate(tab);
#endif

#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  web_discovery_tab_helper_ =
      web_discovery::WebDiscoveryTabHelper::MaybeCreate(tab, *profile);
#endif

  brave_shields_tab_helper_ =
      std::make_unique<brave_shields::BraveShieldsTabHelper>(tab);

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  rewards_tab_helper_ = std::make_unique<brave_rewards::RewardsTabHelper>(tab);
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  drm_tab_helper_ = std::make_unique<BraveDrmTabHelper>(tab);
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  brave_wallet_tab_helper_ =
      std::make_unique<brave_wallet::BraveWalletTabHelper>(tab);
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  geolocation_permission_tab_helper_ =
      std::make_unique<BraveGeolocationPermissionTabHelper>(tab);
#endif

#if BUILDFLAG(ENABLE_REQUEST_OTR)
  if (!profile->IsOffTheRecord() &&
      base::FeatureList::IsEnabled(
          request_otr::features::kBraveRequestOTRTab)) {
    request_otr_tab_helper_ = std::make_unique<RequestOTRTabHelper>(tab);
  }
#endif

#if BUILDFLAG(IS_WIN)
  if (base::FeatureList::IsEnabled(features::kBraveWorkaroundNewWindowFlash)) {
    background_color_tab_helper_ =
        std::make_unique<BackgroundColorTabHelper>(tab);
  }
#endif
}

}  // namespace tabs
