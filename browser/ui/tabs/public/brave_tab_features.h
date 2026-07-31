// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_

#include <memory>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"

class Profile;

#if BUILDFLAG(ENABLE_AI_CHAT)
namespace ai_chat {
class TabDataWebContentsObserver;
class WebMcpInjector;
}
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
