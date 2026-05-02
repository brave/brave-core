/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_

#include <map>
#include <memory>

#include "base/memory/raw_ptr.h"
#include "ios/web/public/web_state_id.h"

namespace web {
class WebState;
}  // namespace web

// Support for subclassing WKWebViewConfigurationProvider:
// - `browser_state_` gets a friend declaration so the Brave subclass can
//   access the protected field (and the private base constructor).
// - `ResetWithWebViewConfiguration` and `UpdateScripts` become virtual so the
//   Brave subclass can override them.
// - `FromBrowserState` is renamed to `FromBrowserState_ChromiumImpl` in the
//   base and a new `FromBrowserState` + `FromWebState` are declared so Brave
//   can return per-tab providers.
#define browser_state_ \
  browser_state_;      \
  friend class BraveWKWebViewConfigurationProvider
#define ResetWithWebViewConfiguration virtual ResetWithWebViewConfiguration
#define UpdateScripts virtual UpdateScripts
#define FromBrowserState(...)                                                \
  FromBrowserState_ChromiumImpl(__VA_ARGS__);                                \
  static web::WKWebViewConfigurationProvider& FromBrowserState(__VA_ARGS__); \
  static web::WKWebViewConfigurationProvider& FromWebState(                  \
      web::WebState* web_state)
#include <ios/web/web_state/ui/wk_web_view_configuration_provider.h>  // IWYU pragma: export
#undef FromBrowserState
#undef UpdateScripts
#undef ResetWithWebViewConfiguration
#undef browser_state_

namespace web {

// Per-tab subclass. Stores a pointer back to its owning `WebState` so its
// overridden `UpdateScripts()` can route to the per-tab
// `JavaScriptFeatureManager`.
class BraveWKWebViewConfigurationProvider
    : public WKWebViewConfigurationProvider {
 public:
  BraveWKWebViewConfigurationProvider(BrowserState* browser_state,
                                      WebState* web_state);
  ~BraveWKWebViewConfigurationProvider() override;

  WebState* GetWebState() const { return web_state_; }

  void ResetWithWebViewConfiguration(
      WKWebViewConfiguration* configuration) override;
  void UpdateScripts() override;

 private:
  // Non-owning. The WebState owns this provider transitively through its
  // BrowserState, so the WebState outlives... actually the WebState can be
  // destroyed before the BrowserState. See `Erase` below — the map should
  // drop the entry on WebState destruction. Until that's wired up this is
  // a lifetime bet: consult the WebState only while it is known to be alive
  // (inside `UpdateScripts`, which is called while the tab is loading).
  raw_ptr<WebState> web_state_;
};

// Per-BrowserState map of `WebStateID` -> `WKWebViewConfigurationProvider`.
// Owned by the BrowserState via `SupportsUserData` so the providers are torn
// down with the profile.
class BraveWKWebViewConfigurationProviderMap
    : public base::SupportsUserData::Data {
 public:
  BraveWKWebViewConfigurationProviderMap();
  ~BraveWKWebViewConfigurationProviderMap() override;

  BraveWKWebViewConfigurationProviderMap(
      const BraveWKWebViewConfigurationProviderMap&) = delete;
  BraveWKWebViewConfigurationProviderMap& operator=(
      const BraveWKWebViewConfigurationProviderMap&) = delete;

  WKWebViewConfigurationProvider& GetOrCreate(BrowserState* browser_state,
                                              WebState* web_state);
  void Erase(WebStateID id);

 private:
  std::map<WebStateID, std::unique_ptr<WKWebViewConfigurationProvider>> map_;
};

}  // namespace web

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_
