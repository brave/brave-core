// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/navigation_completion_waiter.h"

#include <utility>

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace gestures {

void NavigationCompletionWaiter::Watch(content::WebContents* web_contents,
                                       base::OnceClosure on_done) {
  web_contents->RemoveUserData(UserDataKey());
  CreateForWebContents(web_contents, std::move(on_done));
}

NavigationCompletionWaiter::NavigationCompletionWaiter(
    content::WebContents* web_contents,
    base::OnceClosure on_done)
    : content::WebContentsUserData<NavigationCompletionWaiter>(*web_contents),
      content::WebContentsObserver(web_contents),
      on_done_(std::move(on_done)) {}

NavigationCompletionWaiter::~NavigationCompletionWaiter() = default;

void NavigationCompletionWaiter::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // Only the first primary-main-frame navigation after construction is the
  // one `LoadURL` is about to start; a navigation already in flight on this
  // `WebContents` (e.g. a brand new tab's own default page) started before
  // this and is never captured here.
  if (target_navigation_id_ != -1 ||
      !navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }
  target_navigation_id_ = navigation_handle->GetNavigationId();
}

void NavigationCompletionWaiter::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->GetNavigationId() != target_navigation_id_) {
    return;
  }
  if (web_contents()->IsBeingDestroyed()) {
    return;
  }
  target_committed_ = true;
  // An already-cached, instant page can finish loading by the time its own
  // navigation commits, with no separate `DidStopLoading` call left to
  // wait for.
  if (!web_contents()->IsLoading()) {
    Finish();
  }
}

void NavigationCompletionWaiter::DidStopLoading() {
  // Loading can stop and restart for reasons unrelated to the requested
  // navigation (e.g. the tab's own default page finishing before the
  // requested navigation has even committed); only acts once the target
  // navigation itself has committed.
  if (!target_committed_ || web_contents()->IsBeingDestroyed()) {
    return;
  }
  Finish();
}

void NavigationCompletionWaiter::Finish() {
  // Grabs a local copy first: removing ourselves from the `WebContents`
  // destroys `this` before `on_done_` could otherwise run.
  base::OnceClosure on_done = std::move(on_done_);
  web_contents()->RemoveUserData(UserDataKey());
  std::move(on_done).Run();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(NavigationCompletionWaiter);

}  // namespace gestures
