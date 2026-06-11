/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/memory/ptr_util.h"
#include "base/notreached.h"
// Note: `java_script_feature_manager.h` resolves to Brave's chromium_src
// override via `-iquote`, which exposes `BraveJavaScriptFeatureManager`.
#include "ios/web/js_messaging/java_script_feature_manager.h"
#include "ios/web/js_messaging/java_script_feature_util_impl.h"
#include "ios/web/js_messaging/web_frames_manager_java_script_feature.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/web_state.h"

// Replace the `WKWebViewConfigurationProvider` constructor call inside the
// upstream `FromBrowserState_ChromiumImpl` with the Brave subclass. The
// fallback path goes through this constructor with a null `WebState*` — it is
// only used by callers that don't have a tab (cookie utilities, JS content
// world init, tests, etc.).
#define SetUserData(key, ...)                                                \
  SetUserData(key, base::WrapUnique(new BraveWKWebViewConfigurationProvider( \
                       browser_state, /*web_state=*/nullptr)));
#include <ios/web/web_state/ui/wk_web_view_configuration_provider.mm>
#undef SetUserData

namespace web {

namespace {

constexpr char kBraveWKWebViewConfigProviderMapKey[] =
    "brave_wk_web_view_config_provider_map";

}  // namespace

BraveWKWebViewConfigurationProvider::BraveWKWebViewConfigurationProvider(
    BrowserState* browser_state,
    WebState* web_state)
    : WKWebViewConfigurationProvider(browser_state), web_state_(web_state) {}

BraveWKWebViewConfigurationProvider::~BraveWKWebViewConfigurationProvider() =
    default;

BraveWKWebViewConfigurationProviderMap::
    BraveWKWebViewConfigurationProviderMap() = default;

BraveWKWebViewConfigurationProviderMap::
    ~BraveWKWebViewConfigurationProviderMap() = default;

WKWebViewConfigurationProvider&
BraveWKWebViewConfigurationProviderMap::GetOrCreate(BrowserState* browser_state,
                                                    WebState* web_state) {
  DCHECK(web_state);
  WebStateID id = web_state->GetUniqueIdentifier();
  auto it = map_.find(id);
  if (it == map_.end()) {
    it = map_.emplace(id, std::make_unique<BraveWKWebViewConfigurationProvider>(
                              browser_state, web_state))
             .first;
  }
  return *it->second;
}

void BraveWKWebViewConfigurationProviderMap::Erase(WebStateID id) {
  map_.erase(id);
}

// static
// Per-tab entry point. Returns a `WKWebViewConfigurationProvider` keyed by the
// WebState's unique identifier so each tab has its own configuration.
WKWebViewConfigurationProvider& WKWebViewConfigurationProvider::FromWebState(
    WebState* web_state) {
  DCHECK(web_state);
  BrowserState* browser_state = web_state->GetBrowserState();
  auto* map = static_cast<BraveWKWebViewConfigurationProviderMap*>(
      browser_state->GetUserData(kBraveWKWebViewConfigProviderMapKey));
  if (!map) {
    auto owned = std::make_unique<BraveWKWebViewConfigurationProviderMap>();
    map = owned.get();
    browser_state->SetUserData(kBraveWKWebViewConfigProviderMapKey,
                               std::move(owned));
  }
  return map->GetOrCreate(browser_state, web_state);
}

// static
// Fallback for callers without a WebState (JS content world init, cookie
// utilities, browsing-data removal, tests, etc.). Returns the shared
// per-BrowserState provider that upstream creates.
WKWebViewConfigurationProvider&
WKWebViewConfigurationProvider::FromBrowserState(BrowserState* browser_state) {
  return FromBrowserState_ChromiumImpl(browser_state);
}

void BraveWKWebViewConfigurationProvider::ResetWithWebViewConfiguration(
    WKWebViewConfiguration* configuration) {
  if (configuration != nil) {
    // We need to ensure that each tab has isolated WKUserContentController &
    // WKPreferences, because as of now we specifically adjust these values per
    // web view created rather than when the configuration is created.
    //
    // This must happen prior to WKWebView's creation.
    configuration.userContentController =
        [[WKUserContentController alloc] init];
    configuration.preferences = [configuration.preferences copy];
  }

  WKWebViewConfigurationProvider::ResetWithWebViewConfiguration(configuration);

  // Adjusts the underlying WKWebViewConfiguration for settings we don't want
  // to inherit from Chromium.

  // Restore WKWebView long press.
  @try {
    [configuration_ setValue:@YES forKey:@"longPressActionsEnabled"];
  } @catch (NSException* exception) {
    NOTREACHED() << "Error setting value for longPressActionsEnabled";
  }

  // Restore Apple's safe browsing implementation.
  [[configuration_ preferences] setFraudulentWebsiteWarningEnabled:YES];

  // Reset fullscreen to default as it wasn't set in Brave.
  [[configuration_ preferences] setElementFullscreenEnabled:NO];

  // Add Brave-specific adjustments to the WKWebViewConfiguration here.

  configuration_.dataDetectorTypes = WKDataDetectorTypePhoneNumber;

  // Explicitly pass in the private configuration so that it can be mutated
  // correctly prior to being used in WebState. This is a temporary measure
  // and can be removed once:
  // - The `internal` scheme is removed and interstitials are converted to WebUI
  //   https://github.com/brave/brave-browser/issues/53028
  // - We replace WebKit's built in safe browsing implementation with Chromiums
  //   https://github.com/brave/brave-browser/issues/53029
  // - We replace our custom HTTPS-only upgrade and can remove the assignment of
  //   `WKWebViewConfiguration.upgradeKnownHostsToHTTPS`
  //   https://github.com/brave/brave-browser/issues/53030
  GetWebClient()->DidResetConfiguration(browser_state_, configuration_);
}

// Mirrors upstream `WKWebViewConfigurationProvider::UpdateScripts()` but routes
// through the per-tab `JavaScriptFeatureManager` when this provider is owned
// by a tab. The per-tab `WKUserContentController` is pushed into the feature
// manager up front so its `ConfigureFeatures` doesn't have to re-enter the
// provider to look it up (which would recurse back into this method).
void BraveWKWebViewConfigurationProvider::UpdateScripts() {
  [configuration_.userContentController removeAllUserScripts];

  JavaScriptFeatureManager* java_script_feature_manager =
      web_state_ ? JavaScriptFeatureManager::FromWebState(web_state_)
                 : JavaScriptFeatureManager::FromBrowserState(browser_state_);

  if (web_state_) {
    // `FromWebState` always returns a `BraveJavaScriptFeatureManager`.
    static_cast<BraveJavaScriptFeatureManager*>(java_script_feature_manager)
        ->set_user_content_controller(configuration_.userContentController);
  }

  std::vector<JavaScriptFeature*> features;
  for (JavaScriptFeature* feature :
       java_script_features::GetBuiltInJavaScriptFeatures(browser_state_)) {
    features.push_back(feature);
  }
  for (JavaScriptFeature* feature :
       GetWebClient()->GetJavaScriptFeatures(browser_state_)) {
    features.push_back(feature);
  }
  java_script_feature_manager->ConfigureFeatures(features);

  WKUserContentController* user_content_controller =
      GetWebViewConfiguration().userContentController;
  auto web_frames_manager_features = WebFramesManagerJavaScriptFeature::
      AllContentWorldFeaturesFromBrowserState(browser_state_);
  for (WebFramesManagerJavaScriptFeature* feature :
       web_frames_manager_features) {
    feature->ConfigureHandlers(user_content_controller);
  }
}

}  // namespace web
