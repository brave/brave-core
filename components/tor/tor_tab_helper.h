/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_TAB_HELPER_H_
#define BRAVE_COMPONENTS_TOR_TOR_TAB_HELPER_H_

#include <memory>

#include "base/callback_list.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace tor {

// Owned by the tab's features, and follows the tab's contents across discards.
class TorTabHelper final : public content::WebContentsObserver {
 public:
  // Returns nullptr unless `tab` is in a Tor profile.
  static std::unique_ptr<TorTabHelper> MaybeCreate(tabs::TabInterface& tab);

  explicit TorTabHelper(tabs::TabInterface& tab);
  TorTabHelper(const TorTabHelper&) = delete;
  TorTabHelper& operator=(const TorTabHelper&) = delete;
  ~TorTabHelper() override;

 private:
  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void ReloadTab(content::WebContents* web_contents);

  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents);

  base::CallbackListSubscription will_discard_contents_subscription_;
  base::WeakPtrFactory<TorTabHelper> weak_ptr_factory_{this};
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_TAB_HELPER_H_
