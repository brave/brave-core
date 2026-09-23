/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "content/public/browser/web_contents_observer.h"

class LocationIconView;

// Controller that listens for Brave Shields events for the current active tab
// and opens the Page Info bubble when appropriate.
class BraveShieldsPageInfoController
    : public brave_shields::BraveShieldsTabHelper::Observer,
      public content::WebContentsObserver {
 public:
  explicit BraveShieldsPageInfoController(LocationIconView* location_icon_view);

  BraveShieldsPageInfoController(const BraveShieldsPageInfoController&) =
      delete;
  BraveShieldsPageInfoController& operator=(
      const BraveShieldsPageInfoController&) = delete;

  ~BraveShieldsPageInfoController() override;

  void UpdateWebContents(content::WebContents* web_contents);

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;

  // brave_shields::BraveShieldsTabHelper::Observer:
  void OnResourcesChanged() override;
  void OnRepeatedReloadsDetected() override;

 private:
  void MaybeShowShieldsIPH();
  void ShowBubbleForRepeatedReloads();

  raw_ref<LocationIconView> location_icon_view_;
  base::WeakPtrFactory<BraveShieldsPageInfoController> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_SHIELDS_PAGE_INFO_CONTROLLER_H_
