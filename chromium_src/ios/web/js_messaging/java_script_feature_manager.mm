/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/js_messaging/java_script_feature_manager.h"

#import <WebKit/WebKit.h>

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "ios/web/js_messaging/java_script_content_world.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/content_world.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/web_state.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

#include <ios/web/js_messaging/java_script_feature_manager.mm>

namespace web {

namespace {

constexpr char kBraveJavaScriptFeatureManagerMapKey[] =
    "brave_java_script_feature_manager_map";

}  // namespace

BraveJavaScriptFeatureManager::BraveJavaScriptFeatureManager(
    BrowserState* browser_state,
    WebState* web_state)
    : JavaScriptFeatureManager(browser_state), web_state_(web_state) {}

BraveJavaScriptFeatureManager::~BraveJavaScriptFeatureManager() = default;

// Mirrors upstream `JavaScriptFeatureManager::ConfigureFeatures` but replaces
// the content worlds' `user_content_controller_` with the per-tab one before
// calling `AddFeature`, so that all user scripts and script-message handlers
// land on the tab's controller (which is what the tab's WKWebView actually
// uses) rather than the profile-level one.
//
// The caller (`BraveWKWebViewConfigurationProvider::UpdateScripts`) must have
// called `set_user_content_controller` before invoking this method. If it
// hasn't, we preserve upstream behavior.
void BraveJavaScriptFeatureManager::ConfigureFeatures(
    std::vector<JavaScriptFeature*> features) {
  if (!user_content_controller_) {
    JavaScriptFeatureManager::ConfigureFeatures(std::move(features));
    return;
  }

  page_content_world_ = std::make_unique<JavaScriptContentWorld>(
      browser_state_, WKContentWorld.pageWorld);
  isolated_world_ = std::make_unique<JavaScriptContentWorld>(
      browser_state_, WKContentWorld.defaultClientWorld);

  // Overwrite the controller the ctor picked up from the profile-level
  // provider. Access granted by `friend class BraveJavaScriptFeatureManager`
  // in the Brave `java_script_content_world.h` override.
  page_content_world_->user_content_controller_ = user_content_controller_;
  isolated_world_->user_content_controller_ = user_content_controller_;

  for (JavaScriptFeature* feature : features) {
    switch (feature->GetSupportedContentWorld()) {
      case ContentWorld::kAllContentWorlds:
        isolated_world_->AddFeature(feature);
        page_content_world_->AddFeature(feature);
        break;
      case ContentWorld::kIsolatedWorld:
        isolated_world_->AddFeature(feature);
        break;
      case ContentWorld::kPageContentWorld:
        page_content_world_->AddFeature(feature);
        break;
    }
  }
}

BraveJavaScriptFeatureManagerMap::BraveJavaScriptFeatureManagerMap() = default;

BraveJavaScriptFeatureManagerMap::~BraveJavaScriptFeatureManagerMap() = default;

JavaScriptFeatureManager* BraveJavaScriptFeatureManagerMap::GetOrCreate(
    BrowserState* browser_state,
    WebState* web_state) {
  DCHECK(web_state);
  WebStateID id = web_state->GetUniqueIdentifier();
  auto it = map_.find(id);
  if (it == map_.end()) {
    it = map_.emplace(id, std::make_unique<BraveJavaScriptFeatureManager>(
                              browser_state, web_state))
             .first;
  }
  return it->second.get();
}

void BraveJavaScriptFeatureManagerMap::Erase(WebStateID id) {
  map_.erase(id);
}

// static
// Per-tab entry point. Returns a `JavaScriptFeatureManager` keyed by the
// WebState's unique identifier.
JavaScriptFeatureManager* JavaScriptFeatureManager::FromWebState(
    WebState* web_state) {
  DCHECK(web_state);
  BrowserState* browser_state = web_state->GetBrowserState();
  auto* map = static_cast<BraveJavaScriptFeatureManagerMap*>(
      browser_state->GetUserData(kBraveJavaScriptFeatureManagerMapKey));
  if (!map) {
    auto owned = std::make_unique<BraveJavaScriptFeatureManagerMap>();
    map = owned.get();
    browser_state->SetUserData(kBraveJavaScriptFeatureManagerMapKey,
                               std::move(owned));
  }
  return map->GetOrCreate(browser_state, web_state);
}

// static
// Fallback for callers without a WebState.
JavaScriptFeatureManager* JavaScriptFeatureManager::FromBrowserState(
    BrowserState* browser_state) {
  return FromBrowserState_ChromiumImpl(browser_state);
}

}  // namespace web
