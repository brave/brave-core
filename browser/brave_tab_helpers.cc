/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_tab_helpers.h"
#include <string>
#include <string_view>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/browser/brave_ads/ad_units/search_result_ad/search_result_ad_tab_helper.h"
#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/browser/misc_metrics/page_metrics_tab_helper.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/ntp_background/ntp_tab_helper.h"
#include "brave/browser/skus/skus_service_factory.h"
#include "brave/browser/ui/bookmark/brave_bookmark_tab_helper.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/psst/browser/content/psst_tab_helper.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/version_info/channel.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/features.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_GREASELION)
#include "brave/browser/greaselion/greaselion_tab_helper.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/android/preferences/background_video_playback_tab_helper.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_shields_data_controller.h"
#include "chrome/browser/ui/thumbnails/thumbnail_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/brave_drm_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/browser/infobars/brave_wayback_machine_delegate_impl.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/components/tor/tor_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ipfs/ipfs_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/web_discovery/web_discovery_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#include "brave/browser/request_otr/request_otr_tab_helper.h"
#include "brave/components/request_otr/common/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/browser/playlist/playlist_tab_helper.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/onboarding/onboarding_tab_helper.h"
#endif

namespace brave {

#if defined(TOOLKIT_VIEWS)
// Register per-tab(contextual) side-panel registry.
// Defined at //brave/browser/ui/views/side_panel/brave_side_panel_utils.cc as
// the implementation is view-layer specific.
void RegisterContextualSidePanel(content::WebContents* web_contents);
#endif

void AttachTabHelpers(content::WebContents* web_contents) {
#if defined(TOOLKIT_VIEWS)
  RegisterContextualSidePanel(web_contents);
#endif
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion::GreaselionTabHelper::CreateForWebContents(web_contents);
#endif
  brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents(
      web_contents);
#if BUILDFLAG(IS_ANDROID)
  BackgroundVideoPlaybackTabHelper::CreateForWebContents(web_contents);
#else
  // Add tab helpers here unless they are intended for android too
  BraveBookmarkTabHelper::CreateForWebContents(web_contents);
  brave_shields::BraveShieldsDataController::CreateForWebContents(web_contents);
  ThumbnailTabHelper::CreateForWebContents(web_contents);
#endif

  brave_rewards::RewardsTabHelper::CreateForWebContents(web_contents);

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::features::IsAIChatEnabled()) {
    content::BrowserContext* context = web_contents->GetBrowserContext();
    auto skus_service_getter = base::BindRepeating(
        [](content::BrowserContext* context) {
          return skus::SkusServiceFactory::GetForContext(context);
        },
        context);
    ai_chat::AIChatTabHelper::CreateForWebContents(
        web_contents,
        g_brave_browser_process->process_misc_metrics()->ai_chat_metrics(),
        skus_service_getter, g_browser_process->local_state(),
        std::string(version_info::GetChannelString(chrome::GetChannel())));
  }
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  BraveDrmTabHelper::CreateForWebContents(web_contents);
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  BraveWaybackMachineDelegateImpl::AttachTabHelperIfNeeded(web_contents);
#endif

  brave_perf_predictor::PerfPredictorTabHelper::CreateForWebContents(
      web_contents);

  brave_ads::AdsTabHelper::CreateForWebContents(web_contents);
  brave_ads::SearchResultAdTabHelper::MaybeCreateForWebContents(web_contents);
  psst::PsstTabHelper::MaybeCreateForWebContents(
      web_contents, ISOLATED_WORLD_ID_BRAVE_INTERNAL);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  WebDiscoveryTabHelper::MaybeCreateForWebContents(web_contents);
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderTabHelper::MaybeCreateForWebContents(web_contents);
#endif

#if BUILDFLAG(ENABLE_TOR)
  tor::TorTabHelper::MaybeCreateForWebContents(
      web_contents, web_contents->GetBrowserContext()->IsTor());
  tor::OnionLocationTabHelper::CreateForWebContents(web_contents);
#endif

#if BUILDFLAG(ENABLE_IPFS)
  ipfs::IPFSTabHelper::MaybeCreateForWebContents(web_contents);
#endif

  BraveNewsTabHelper::MaybeCreateForWebContents(web_contents);

#if defined(TOOLKIT_VIEWS)
  OnboardingTabHelper::MaybeCreateForWebContents(web_contents);
#endif

  if (base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage)) {
    ephemeral_storage::EphemeralStorageTabHelper::CreateForWebContents(
        web_contents);
  }

  brave_wallet::BraveWalletTabHelper::CreateForWebContents(web_contents);

  if (!web_contents->GetBrowserContext()->IsOffTheRecord()) {
    ntp_background_images::NTPTabHelper::CreateForWebContents(web_contents);
    misc_metrics::PageMetricsTabHelper::CreateForWebContents(web_contents);
#if BUILDFLAG(ENABLE_REQUEST_OTR)
    if (base::FeatureList::IsEnabled(
            request_otr::features::kBraveRequestOTRTab)) {
      RequestOTRTabHelper::CreateForWebContents(web_contents);
    }
#endif
  }

#if BUILDFLAG(ENABLE_PLAYLIST)
  playlist::PlaylistTabHelper::MaybeCreateForWebContents(web_contents);
#endif
}

}  // namespace brave
