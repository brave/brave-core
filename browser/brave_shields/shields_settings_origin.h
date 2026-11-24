/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_SHIELDS_SETTINGS_ORIGIN_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_SHIELDS_SETTINGS_ORIGIN_H_

#include "url/gurl.h"

namespace brave {
class BraveRequestInfo;
}

namespace url {
class Origin;
}

namespace content {
class NavigationHandle;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace net {
class IsolationInfo;
}

namespace brave_shields {

// A snapshot of the site whose Shields settings apply. An empty settings URL
// selects the profile defaults. Callers cannot supply a candidate origin.
class ShieldsSettingsOrigin {
 public:
  ShieldsSettingsOrigin() = default;
  static ShieldsSettingsOrigin ForFrame(content::RenderFrameHost& frame);

  const GURL& GetURL() const { return url_; }

 private:
  friend class brave::BraveRequestInfo;
  friend class BraveShieldsWebContentsObserver;

  explicit ShieldsSettingsOrigin(const url::Origin& origin);
  static ShieldsSettingsOrigin ForIsolationInfo(const net::IsolationInfo& info);
  static ShieldsSettingsOrigin ForCommit(content::NavigationHandle& handle);
  static void SetForNewWindow(content::WebContents* contents,
                              content::RenderFrameHost* opener);
  static void OnNavigationStarted(content::NavigationHandle* handle);
  static void OnReadyToCommitNavigation(content::NavigationHandle* handle);
  static void OnNavigationFinished(content::NavigationHandle* handle);

  GURL url_;
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_SHIELDS_SETTINGS_ORIGIN_H_
