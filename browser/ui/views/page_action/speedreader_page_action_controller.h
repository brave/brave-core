// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_SPEEDREADER_PAGE_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_SPEEDREADER_PAGE_ACTION_CONTROLLER_H_

#include "base/callback_list.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/speedreader/speedreader_tab_helper.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "components/tabs/public/tab_interface.h"

namespace content {
class WebContents;
}

namespace page_actions {

// Drives the Speedreader page action: shows a reader-mode icon whenever the
// active page is distilled or distillable, and routes icon clicks to
// SpeedreaderTabHelper (left click toggles distillation, right click shows
// the reader-mode bubble).
class SpeedreaderPageActionController
    : public speedreader::SpeedreaderTabHelper::Observer {
 public:
  SpeedreaderPageActionController(
      tabs::TabInterface& tab,
      page_actions::PageActionController& page_action_controller);
  SpeedreaderPageActionController(const SpeedreaderPageActionController&) =
      delete;
  SpeedreaderPageActionController& operator=(
      const SpeedreaderPageActionController&) = delete;
  ~SpeedreaderPageActionController() override;

  void Init();

  void ExecuteAction(int event_flags);

 private:
  // speedreader::SpeedreaderTabHelper::Observer:
  void OnDistillStateUpdated() override;

  // (Re-)observes |contents|' SpeedreaderTabHelper, since the tab's contents
  // can be swapped out (e.g. tab discarding). Stops observing the previously
  // attached helper, which may be about to be destroyed.
  void AttachToTabHelper(content::WebContents* contents);

  void UpdatePageAction(content::WebContents* contents);

  const raw_ref<tabs::TabInterface> tab_;
  const raw_ref<page_actions::PageActionControllerImpl> page_action_controller_;

  base::CallbackListSubscription did_activate_subscription_;
  base::CallbackListSubscription will_discard_contents_subscription_;

  base::ScopedObservation<speedreader::SpeedreaderTabHelper,
                          speedreader::SpeedreaderTabHelper::Observer>
      tab_helper_observation_{this};
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_SPEEDREADER_PAGE_ACTION_CONTROLLER_H_
