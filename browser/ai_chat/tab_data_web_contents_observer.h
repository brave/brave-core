// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TAB_DATA_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_BROWSER_AI_CHAT_TAB_DATA_WEB_CONTENTS_OBSERVER_H_

#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

namespace content {
class NavigationEntry;
class Page;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace ai_chat {

class TabTrackerService;

// This class informs the TabTrackerService about changes to tabs (i.e.
// creation, deletion, title/url updates). Each instance of this class is
// associated with a single tab, and follows the tab's contents when they are
// replaced.
class TabDataWebContentsObserver : public tabs::ContentsObservingTabFeature {
 public:
  TabDataWebContentsObserver(int32_t tab_handle, tabs::TabInterface& tab);
  ~TabDataWebContentsObserver() override;

  TabDataWebContentsObserver(const TabDataWebContentsObserver&) = delete;
  TabDataWebContentsObserver& operator=(const TabDataWebContentsObserver&) =
      delete;

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void TitleWasSet(content::NavigationEntry* entry) override;

 private:
  void UpdateTab();

  int32_t tab_handle_ = 0;

  raw_ref<TabTrackerService> service_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TAB_DATA_WEB_CONTENTS_OBSERVER_H_
