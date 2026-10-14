// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_NAVIGATION_COMPLETION_WAITER_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_NAVIGATION_COMPLETION_WAITER_H_

#include <cstdint>

#include "base/functional/callback.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace gestures {

// Waits for a navigation to actually finish loading, not just commit, so a
// trailing chained action can't tear down the renderer mid-load.
class NavigationCompletionWaiter final
    : public content::WebContentsUserData<NavigationCompletionWaiter>,
      public content::WebContentsObserver {
 public:
  NavigationCompletionWaiter(const NavigationCompletionWaiter&) = delete;
  NavigationCompletionWaiter& operator=(const NavigationCompletionWaiter&) =
      delete;

  ~NavigationCompletionWaiter() override;

  // Waits for the next navigation on `web_contents` to finish loading,
  // running `callback` once it has. Replaces any previous unfinished waiter.
  static void Watch(content::WebContents* web_contents,
                    base::OnceClosure callback);

 private:
  friend class content::WebContentsUserData<NavigationCompletionWaiter>;

  // Constructed before `LoadURL` runs, so `DidStartNavigation` can capture
  // the started navigation's ID even if it commits synchronously.
  NavigationCompletionWaiter(content::WebContents* web_contents,
                             base::OnceClosure callback);

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidStopLoading() override;

  void Finish();

  bool target_committed_ = false;
  // Set by `DidStartNavigation` to the first primary-main-frame navigation
  // observed after construction; unset (-1) until then.
  int64_t target_navigation_id_ = -1;
  base::OnceClosure completion_callback_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_NAVIGATION_COMPLETION_WAITER_H_
