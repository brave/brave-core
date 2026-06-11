/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_FEATURE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_FEATURE_MANAGER_H_

#include <map>
#include <memory>

#include "base/memory/raw_ptr.h"
#include "ios/web/public/web_state_id.h"

@class WKUserContentController;

namespace web {
class WebState;
}  // namespace web

// Support for subclassing `JavaScriptFeatureManager`:
// - Inject a friend declaration so `BraveJavaScriptFeatureManager` can call
//   the private base constructor and access the private `page_content_world_`
//   and `isolated_world_` fields.
// - Make `ConfigureFeatures` virtual so Brave can override it.
// - Rename upstream `FromBrowserState` to `FromBrowserState_ChromiumImpl` and
//   declare a new `FromBrowserState` + `FromWebState` entry point.
#define browser_state_ \
  browser_state_;      \
  friend class BraveJavaScriptFeatureManager
#define ConfigureFeatures virtual ConfigureFeatures
#define FromBrowserState(...)                                     \
  FromBrowserState_ChromiumImpl(__VA_ARGS__);                     \
  static JavaScriptFeatureManager* FromBrowserState(__VA_ARGS__); \
  static JavaScriptFeatureManager* FromWebState(web::WebState* web_state)
#include <ios/web/js_messaging/java_script_feature_manager.h>  // IWYU pragma: export
#undef FromBrowserState
#undef ConfigureFeatures
#undef browser_state_

namespace web {

// Per-tab subclass. Holds a pointer to its owning WebState and the
// `WKUserContentController` for that tab, so the overridden
// `ConfigureFeatures` can install scripts and script-message handlers on the
// per-tab controller instead of the shared profile-level one.
//
// The per-tab controller is set explicitly by
// `BraveWKWebViewConfigurationProvider::UpdateScripts` just before it calls
// `ConfigureFeatures` on us. We don't look it up ourselves via
// `WKWebViewConfigurationProvider::FromWebState` because that call path would
// re-enter `UpdateScripts` -> `ConfigureFeatures` recursively on first use.
class BraveJavaScriptFeatureManager : public JavaScriptFeatureManager {
 public:
  BraveJavaScriptFeatureManager(BrowserState* browser_state,
                                WebState* web_state);
  ~BraveJavaScriptFeatureManager() override;

  WebState* GetWebState() const { return web_state_; }

  void set_user_content_controller(WKUserContentController* controller) {
    user_content_controller_ = controller;
  }

  void ConfigureFeatures(std::vector<JavaScriptFeature*> features) override;

 private:
  raw_ptr<WebState> web_state_;
  WKUserContentController* user_content_controller_ = nil;
};

// Per-BrowserState map of `WebStateID` -> `JavaScriptFeatureManager`, owned by
// the BrowserState through `SupportsUserData`. Parallels
// `BraveWKWebViewConfigurationProviderMap`.
class BraveJavaScriptFeatureManagerMap : public base::SupportsUserData::Data {
 public:
  BraveJavaScriptFeatureManagerMap();
  ~BraveJavaScriptFeatureManagerMap() override;

  BraveJavaScriptFeatureManagerMap(const BraveJavaScriptFeatureManagerMap&) =
      delete;
  BraveJavaScriptFeatureManagerMap& operator=(
      const BraveJavaScriptFeatureManagerMap&) = delete;

  JavaScriptFeatureManager* GetOrCreate(BrowserState* browser_state,
                                        WebState* web_state);
  void Erase(WebStateID id);

 private:
  std::map<WebStateID, std::unique_ptr<JavaScriptFeatureManager>> map_;
};

}  // namespace web

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_FEATURE_MANAGER_H_
