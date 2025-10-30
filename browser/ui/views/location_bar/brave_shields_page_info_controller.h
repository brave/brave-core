/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class LocationIconView;
class TabStripModel;

namespace content {
class WebContents;
}

// Controller that listens for Brave Shields events and opens the Page Info
// bubble when appropriate (e.g., after repeated reloads are detected).
class BraveShieldsPageInfoController
    : public brave_shields::BraveShieldsTabHelper::Observer,
      public TabStripModelObserver {
 public:
  BraveShieldsPageInfoController(TabStripModel* tab_strip_model,
                                 LocationIconView* location_icon_view);

  BraveShieldsPageInfoController(const BraveShieldsPageInfoController&) =
      delete;
  BraveShieldsPageInfoController& operator=(
      const BraveShieldsPageInfoController&) = delete;

  ~BraveShieldsPageInfoController() override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // brave_shields::BraveShieldsTabHelper::Observer:
  void OnResourcesChanged() override;
  void OnRepeatedReloadsDetected() override;

 private:
  void UpdateShieldsObservation(content::WebContents* web_contents);
  void ShowBubbleForRepeatedReloads();

  raw_ref<TabStripModel> tab_strip_model_;
  raw_ref<LocationIconView> location_icon_view_;
  base::ScopedObservation<brave_shields::BraveShieldsTabHelper,
                          brave_shields::BraveShieldsTabHelper::Observer>
      shields_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_
