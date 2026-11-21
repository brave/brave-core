/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_SHIELDS_SETTINGS_ORIGIN_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_SHIELDS_SETTINGS_ORIGIN_H_

#include <optional>

#include "content/public/browser/document_user_data.h"
#include "content/public/browser/navigation_handle_user_data.h"
#include "url/origin.h"

class GURL;

namespace content {
class NavigationHandle;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace brave_shields {

// The origin that Shields settings are looked up for in a page. This is the
// top frame origin, ignoring sandboxing. When the top frame has no origin of
// its own (e.g. `about:blank` or `data:`), it is the value of the page that
// opened it or navigated it, and an opaque origin (default settings) when
// there is no such page.
//
// The value is attached to the outermost main frame document. It only changes
// when that frame navigates.
class ShieldsSettingsOrigin
    : public content::DocumentUserData<ShieldsSettingsOrigin> {
 public:
  ShieldsSettingsOrigin(const ShieldsSettingsOrigin&) = delete;
  ShieldsSettingsOrigin& operator=(const ShieldsSettingsOrigin&) = delete;
  ~ShieldsSettingsOrigin() override;

  // Returns `origin` if it is not opaque, otherwise the origin of `url` if
  // that is not opaque, otherwise `inherited`.
  static url::Origin Resolve(const std::optional<url::Origin>& origin,
                             const GURL& url,
                             const url::Origin& inherited);

  // Returns the value for the page that `render_frame_host` belongs to.
  static url::Origin GetForFrame(content::RenderFrameHost* render_frame_host);

  // Gives the initial empty document of `new_contents` the value of the page
  // that `opener` belongs to.
  static void SetForNewWindow(content::WebContents* new_contents,
                              content::RenderFrameHost* opener);

  // Navigation hooks for outermost main frames.
  static void OnNavigationStarted(content::NavigationHandle* handle);
  static void OnReadyToCommitNavigation(content::NavigationHandle* handle);
  static void OnNavigationFinished(content::NavigationHandle* handle);

  const url::Origin& value() const { return value_; }

 private:
  friend DocumentUserData;
  DOCUMENT_USER_DATA_KEY_DECL();

  ShieldsSettingsOrigin(content::RenderFrameHost* render_frame_host,
                        const url::Origin& value);

  const url::Origin value_;
};

// The value computed for an outermost main frame navigation, moved to the new
// document's `ShieldsSettingsOrigin` when it commits.
class ShieldsSettingsOriginNavigationData
    : public content::NavigationHandleUserData<
          ShieldsSettingsOriginNavigationData> {
 public:
  ShieldsSettingsOriginNavigationData(
      const ShieldsSettingsOriginNavigationData&) = delete;
  ShieldsSettingsOriginNavigationData& operator=(
      const ShieldsSettingsOriginNavigationData&) = delete;
  ~ShieldsSettingsOriginNavigationData() override;

  const url::Origin& inherited() const { return inherited_; }
  const url::Origin& value() const { return value_; }
  void set_value(const url::Origin& value) { value_ = value; }

 private:
  friend NavigationHandleUserData;
  NAVIGATION_HANDLE_USER_DATA_KEY_DECL();

  ShieldsSettingsOriginNavigationData(content::NavigationHandle& handle,
                                      const url::Origin& inherited,
                                      const url::Origin& value);

  // The value of the page that initiated the navigation, used when the
  // navigation commits a document without an origin of its own.
  const url::Origin inherited_;
  url::Origin value_;
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_SHIELDS_SETTINGS_ORIGIN_H_
