// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_

#include <memory>

#include "base/callback_list.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"

class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace image_metadata_stripper {
class FileUploadController;
}  // namespace image_metadata_stripper

#if BUILDFLAG(ENABLE_AI_CHAT)
namespace ai_chat {
class TabDataWebContentsObserver;
class WebMcpInjector;
}  // namespace ai_chat
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
namespace containers {
class ContainerTabTracker;
}
namespace page_actions {
class PartitionedStoragePageActionController;
}
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/views/page_action/speedreader_page_action_controller.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/browser/ui/views/page_action/wayback_machine_page_action_controller.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/ui/views/page_action/playlist_page_action_controller.h"
#endif

#if BUILDFLAG(ENABLE_PSST)
#include "brave/browser/ui/views/page_action/psst_action_controller.h"
namespace psst {
class PsstTabWebContentsObserver;
}
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/views/page_action/onion_location_page_action_controller.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/browser/ui/views/page_action/brave_news_page_action_controller.h"
#endif

namespace tabs {

class ContentsObservingTabFeature;
class TabInterface;

class BraveTabFeatures : public TabFeatures {
 public:
  static BraveTabFeatures* FromTabFeatures(TabFeatures* tab_features);
  BraveTabFeatures();
  ~BraveTabFeatures() override;

  void Init(TabInterface& tab, Profile* profile) override;

#if BUILDFLAG(ENABLE_PSST)
  psst::PsstTabWebContentsObserver* psst_web_contents_observer() {
    return psst_web_contents_observer_.get();
  }
  page_actions::PsstActionController* psst_page_action_controller() {
    return psst_action_controller_.get();
  }
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
  page_actions::PartitionedStoragePageActionController*
  partitioned_storage_page_action_controller() {
    return partitioned_storage_page_action_controller_.get();
  }
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  page_actions::SpeedreaderPageActionController*
  speedreader_page_action_controller() {
    return speedreader_page_action_controller_.get();
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  page_actions::WaybackMachinePageActionController*
  wayback_machine_page_action_controller() {
    return wayback_machine_page_action_controller_.get();
  }
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  page_actions::PlaylistPageActionController*
  playlist_page_action_controller() {
    return playlist_page_action_controller_.get();
  }
#endif

#if BUILDFLAG(ENABLE_TOR)
  page_actions::OnionLocationPageActionController*
  onion_location_page_action_controller() {
    return onion_location_page_action_controller_.get();
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  page_actions::BraveNewsPageActionController*
  brave_news_page_action_controller() {
    return brave_news_page_action_controller_.get();
  }
#endif

 private:
  void OnWillDiscardContents(TabInterface* tab,
                             content::WebContents* old_contents,
                             content::WebContents* new_contents);

  base::CallbackListSubscription will_discard_contents_subscription_;

  // The main controller which takes care of stripping metadata from images
  // for the upload flow.
  std::unique_ptr<image_metadata_stripper::FileUploadController>
      stripper_upload_controller_;

  // Records Cloudflare javascript-detection script loads.
  std::unique_ptr<ContentsObservingTabFeature>
      cloudflare_js_detection_tab_helper_;

  // Avoids a white flash on new tabs. Windows only, null elsewhere.
  std::unique_ptr<ContentsObservingTabFeature> background_color_tab_helper_;

  // Records page and media session metrics.
  std::unique_ptr<ContentsObservingTabFeature> page_metrics_tab_helper_;

  // Shows an infobar while browsing a site off the record at its request.
  std::unique_ptr<ContentsObservingTabFeature> request_otr_tab_helper_;

  // Opens the Leo panel once for users in the sidebar test.
  std::unique_ptr<ContentsObservingTabFeature> sidebar_tab_helper_;
  // Highlights the Brave Shields icon once for new users.
  std::unique_ptr<ContentsObservingTabFeature> onboarding_tab_helper_;

  // Records searches. Null when SERP metrics are disabled or off the record.
  std::unique_ptr<ContentsObservingTabFeature> serp_metrics_tab_helper_;
  // Notifies the ads service of tab events. Null without Brave Ads.
  std::unique_ptr<ContentsObservingTabFeature> ads_tab_helper_;
  // Handles search result ads. Null without Brave Ads or off the record.
  std::unique_ptr<ContentsObservingTabFeature>
      creative_search_result_ad_tab_helper_;
  // Offers and runs Web Discovery. Null without it or outside regular profiles.
  std::unique_ptr<ContentsObservingTabFeature> web_discovery_tab_helper_;
  // Detects the Rewards creator of the page. Null without Brave Rewards.
  std::unique_ptr<ContentsObservingTabFeature> rewards_tab_helper_;
  // Finds the page's Brave News feeds. Null without Brave News or off the
  // record. Declared before the page action controllers that observe it.
  std::unique_ptr<ContentsObservingTabFeature> brave_news_tab_helper_;
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  // Serves the wallet providers of the tab's frames.
  std::unique_ptr<ContentsObservingTabFeature> brave_wallet_tab_helper_;
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
  // Distills the page. Null when the Speedreader feature is off or the profile
  // has no Speedreader service. Declared before the page action controller
  // that observes it.
  std::unique_ptr<ContentsObservingTabFeature> speedreader_tab_helper_;
#endif
  // Offers to enable Widevine when the page asks for it. Null without it.
  std::unique_ptr<ContentsObservingTabFeature> drm_tab_helper_;
  // Holds the Shields state for the page shown in the tab.
  std::unique_ptr<ContentsObservingTabFeature> brave_shields_tab_helper_;
  // Records whether the page asked for high accuracy geolocation.
  std::unique_ptr<ContentsObservingTabFeature>
      geolocation_permission_tab_helper_;

#if BUILDFLAG(ENABLE_AI_CHAT)
  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
  std::unique_ptr<ai_chat::WebMcpInjector> web_mcp_injector_;
#endif
#if BUILDFLAG(ENABLE_PSST)
  std::unique_ptr<psst::PsstTabWebContentsObserver> psst_web_contents_observer_;
  std::unique_ptr<page_actions::PsstActionController> psst_action_controller_;
#endif
#if BUILDFLAG(ENABLE_CONTAINERS)
  std::unique_ptr<containers::ContainerTabTracker> container_tab_tracker_;
  std::unique_ptr<page_actions::PartitionedStoragePageActionController>
      partitioned_storage_page_action_controller_;
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
  std::unique_ptr<page_actions::SpeedreaderPageActionController>
      speedreader_page_action_controller_;
#endif
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  std::unique_ptr<page_actions::WaybackMachinePageActionController>
      wayback_machine_page_action_controller_;
#endif
#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  std::unique_ptr<page_actions::PlaylistPageActionController>
      playlist_page_action_controller_;
#endif
#if BUILDFLAG(ENABLE_TOR)
  std::unique_ptr<page_actions::OnionLocationPageActionController>
      onion_location_page_action_controller_;
#endif
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  std::unique_ptr<page_actions::BraveNewsPageActionController>
      brave_news_page_action_controller_;
#endif
};

}  // namespace tabs
#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_
