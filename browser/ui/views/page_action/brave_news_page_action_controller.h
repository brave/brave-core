// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_BRAVE_NEWS_PAGE_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_BRAVE_NEWS_PAGE_ACTION_CONTROLLER_H_

#include <vector>

#include "base/callback_list.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "components/prefs/pref_member.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents_observer.h"

class ToolbarButtonProvider;

namespace actions {
class ActionItem;
}

namespace content {
class WebContents;
}

namespace page_actions {

// Drives the Brave News page action: shows the icon when the current page has
// an available news feed (colored differently when the user is already
// subscribed to it), and shows the Brave News bubble when clicked.
class BraveNewsPageActionController
    : public BraveNewsTabHelper::PageFeedsObserver,
      public content::WebContentsObserver {
 public:
  BraveNewsPageActionController(
      tabs::TabInterface& tab,
      page_actions::PageActionController& page_action_controller);
  BraveNewsPageActionController(const BraveNewsPageActionController&) = delete;
  BraveNewsPageActionController& operator=(
      const BraveNewsPageActionController&) = delete;
  ~BraveNewsPageActionController() override;

  void Init();

  void ExecuteAction(ToolbarButtonProvider* toolbar_button_provider,
                     actions::ActionItem* item);

  // BraveNewsTabHelper::PageFeedsObserver:
  void OnAvailableFeedsChanged(const std::vector<GURL>& feeds) override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

 private:
  void OnPrefChanged();

  // (Re-)observes |contents| and its BraveNewsTabHelper, since the tab's
  // contents can be swapped out (e.g. tab discarding). Stops observing the
  // previously attached helper, which may be about to be destroyed.
  void AttachToTabHelper(content::WebContents* contents);

  void UpdatePageAction(content::WebContents* contents);

  const raw_ref<tabs::TabInterface> tab_;
  const raw_ref<page_actions::PageActionControllerImpl> page_action_controller_;

  base::CallbackListSubscription did_activate_subscription_;
  base::CallbackListSubscription will_discard_contents_subscription_;

  base::ScopedObservation<BraveNewsTabHelper,
                          BraveNewsTabHelper::PageFeedsObserver>
      page_feeds_observer_{this};

  BooleanPrefMember should_show_;
  BooleanPrefMember opted_in_;
  BooleanPrefMember news_enabled_;
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_BRAVE_NEWS_PAGE_ACTION_CONTROLLER_H_
