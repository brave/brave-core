/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_SHIELDS_SETTINGS_ORIGIN_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_SHIELDS_SETTINGS_ORIGIN_H_

#include <optional>

#include "base/types/optional_ref.h"
#include "content/public/browser/content_browser_client.h"
#include "url/gurl.h"

namespace network {
struct ResourceRequest;
}  // namespace network

namespace brave {
class BraveRequestInfo;
}

namespace url {
class Origin;
}

namespace content {
struct GlobalRenderFrameHostToken;
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

  // An about:blank popup can retain its opening subframe's factory after
  // the subframe goes away. Copy the key while its outermost document exists.
  static std::optional<ShieldsSettingsOrigin> ForEmbeddedDocumentFactory(
      const content::GlobalRenderFrameHostToken& token,
      content::ContentBrowserClient::URLLoaderFactoryType type);

  static ShieldsSettingsOrigin ForRequest(
      const network::ResourceRequest& request,
      const content::GlobalRenderFrameHostToken& token,
      std::optional<content::ContentBrowserClient::URLLoaderFactoryType>
          factory_type = std::nullopt,
      base::optional_ref<const net::IsolationInfo> factory_info = std::nullopt,
      base::optional_ref<const ShieldsSettingsOrigin> document_key =
          std::nullopt);

  // Subresources still belong to the original document after a redirect.
  // A main-frame redirect carries the next destination's isolation info.
  ShieldsSettingsOrigin ForRedirect(
      const network::ResourceRequest& request) const;

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
