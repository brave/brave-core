/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_tab_helpers.h"

#include "brave/browser/ui/bookmark/brave_bookmark_tab_helper.h"
#include "brave/components/brave_ads/browser/ads_tab_helper.h"
#include "brave/components/brave_perf_predictor/browser/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/browser/buildflags/buildflags.h"  // For STP
#include "brave/components/brave_wayback_machine/buildflags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "content/public/browser/web_contents.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_GREASELION)
#include "brave/browser/greaselion/greaselion_tab_helper.h"
#endif

#if defined(OS_ANDROID)
#include "brave/browser/android/preferences/background_video_playback_tab_helper.h"
#include "brave/browser/android/preferences/website/desktop_mode_tab_helper.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#endif

#if BUILDFLAG(BRAVE_STP_ENABLED)
#include "brave/components/brave_shields/browser/tracking_protection_helper.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/brave_drm_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/browser/infobars/brave_wayback_machine_delegate_impl.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#endif

namespace brave {

void AttachTabHelpers(content::WebContents* web_contents) {
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion::GreaselionTabHelper::CreateForWebContents(web_contents);
#endif
  brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents(
      web_contents);

#if defined(OS_ANDROID)
  DesktopModeTabHelper::CreateForWebContents(web_contents);
  BackgroundVideoPlaybackTabHelper::CreateForWebContents(web_contents);
#else
  // Add tab helpers here unless they are intended for android too
  BraveBookmarkTabHelper::CreateForWebContents(web_contents);
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  brave_rewards::RewardsTabHelper::CreateForWebContents(web_contents);
#endif

#if BUILDFLAG(BRAVE_STP_ENABLED)
  if (brave_shields::TrackingProtectionService::
          IsSmartTrackingProtectionEnabled()) {
    brave_shields::TrackingProtectionHelper::CreateForWebContents(web_contents);
  }
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  BraveDrmTabHelper::CreateForWebContents(web_contents);
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  BraveWaybackMachineDelegateImpl::AttachTabHelperIfNeeded(web_contents);
#endif

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
  brave_perf_predictor::PerfPredictorTabHelper::CreateForWebContents(
      web_contents);
#endif

  brave_ads::AdsTabHelper::CreateForWebContents(web_contents);

#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderTabHelper::CreateForWebContents(web_contents);
#endif
}

}  // namespace brave
