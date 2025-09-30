/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_

#include "base/scoped_observation.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"

namespace content {
class WebContents;
}

// Controller that listens for Brave Shields events and opens the Page Info
// bubble when appropriate (e.g., after repeated reloads are detected).
class BraveShieldsPageInfoController
    : public brave_shields::BraveShieldsTabHelper::Observer {
 public:
  BraveShieldsPageInfoController();

  BraveShieldsPageInfoController(const BraveShieldsPageInfoController&) =
      delete;
  BraveShieldsPageInfoController& operator=(
      const BraveShieldsPageInfoController&) = delete;

  ~BraveShieldsPageInfoController() override;

  // Updates the controller to observe the specified WebContents.
  void UpdateWebContents(content::WebContents* web_contents);

  // brave_shields::BraveShieldsTabHelper::Observer:
  void OnResourcesChanged() override;
  void OnRepeatedReloadsDetected() override;

 private:
  base::ScopedObservation<brave_shields::BraveShieldsTabHelper,
                          brave_shields::BraveShieldsTabHelper::Observer>
      shields_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_
