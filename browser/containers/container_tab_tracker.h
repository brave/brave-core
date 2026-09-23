// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_CONTAINERS_CONTAINER_TAB_TRACKER_H_
#define BRAVE_BROWSER_CONTAINERS_CONTAINER_TAB_TRACKER_H_

#include <memory>

#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace containers {

// Observes the tab's active WebContents for the first primary main-frame
// navigation and, if the tab uses a Containers storage partition, marks the
// container as used.
class ContainerTabTracker : public tabs::ContentsObservingTabFeature {
 public:
  ~ContainerTabTracker() override;

  ContainerTabTracker(const ContainerTabTracker&) = delete;
  ContainerTabTracker& operator=(const ContainerTabTracker&) = delete;

  // Returns nullptr if |ContainersService| is unavailable for this tab's
  // profile.
  static std::unique_ptr<ContainerTabTracker> MaybeCreate(
      tabs::TabInterface& tab);

 private:
  explicit ContainerTabTracker(tabs::TabInterface& tab);

  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

  bool tracked_ = false;
};

}  // namespace containers

#endif  // BRAVE_BROWSER_CONTAINERS_CONTAINER_TAB_TRACKER_H_
