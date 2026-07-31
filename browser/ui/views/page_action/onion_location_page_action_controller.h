// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_ONION_LOCATION_PAGE_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_ONION_LOCATION_PAGE_ACTION_CONTROLLER_H_

#include "base/callback_list.h"
#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents_observer.h"

namespace page_actions {

// Drives the Onion Location page action: shows an icon (and, when browsing
// via Tor, a pill with the "Onion available" label) when the current page has
// an associated .onion location, and switches to (or opens) the Tor profile
// on that .onion location when clicked.
class OnionLocationPageActionController : public content::WebContentsObserver {
 public:
  OnionLocationPageActionController(
      tabs::TabInterface& tab,
      page_actions::PageActionController& page_action_controller);
  OnionLocationPageActionController(const OnionLocationPageActionController&) =
      delete;
  OnionLocationPageActionController& operator=(
      const OnionLocationPageActionController&) = delete;
  ~OnionLocationPageActionController() override;

  void Init();

  void ExecuteAction();

 private:
  void AttachToWebContents();
  void UpdatePageAction();
  void ResetPageAction();

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  const raw_ref<tabs::TabInterface> tab_;
  const raw_ref<page_actions::PageActionControllerImpl> page_action_controller_;

  base::CallbackListSubscription did_activate_subscription_;
  base::CallbackListSubscription will_discard_contents_subscription_;
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_ONION_LOCATION_PAGE_ACTION_CONTROLLER_H_
