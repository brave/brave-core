// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <WebKit/WebKit.h>

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ios/web/common/web_view_creation_util.h"
#include "ios/web/js_messaging/java_script_feature_manager.h"
#include "ios/web/public/js_messaging/content_world.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/test/fakes/fake_web_client.h"
#include "ios/web/public/test/web_test.h"
#include "ios/web/public/web_state.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "ios/web/web_state/web_state_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web {

namespace {

// A FakeWebClient recording the WebStates its per-WebState features are
// requested for.
class RecordingWebClient : public FakeWebClient {
 public:
  std::vector<JavaScriptFeature*> GetWebStateJavaScriptFeatures(
      WebState* web_state) const override {
    requested_web_states_.push_back(web_state);
    return FakeWebClient::GetWebStateJavaScriptFeatures(web_state);
  }

  const std::vector<raw_ptr<WebState>>& requested_web_states() const {
    return requested_web_states_;
  }

 private:
  mutable std::vector<raw_ptr<WebState>> requested_web_states_;
};

// A feature with a script, so it is added to a content world.
class TestFeature : public JavaScriptFeature {
 public:
  TestFeature()
      : JavaScriptFeature(ContentWorld::kIsolatedWorld,
                          {FeatureScript::CreateWithString(
                              "/* test */",
                              FeatureScript::InjectionTime::kDocumentStart,
                              FeatureScript::TargetFrames::kMainFrame)}) {}
};

}  // namespace

class WebStateJavaScriptFeaturesTest : public WebTest {
 protected:
  WebStateJavaScriptFeaturesTest()
      : WebTest(std::make_unique<RecordingWebClient>()) {}

  RecordingWebClient* web_client() {
    return static_cast<RecordingWebClient*>(GetWebClient());
  }

  std::unique_ptr<WebState> CreateWebState() {
    return WebState::Create(WebState::CreateParams(GetBrowserState()));
  }

  // Creates the configuration of `web_state`, which configures its features.
  void CreateConfiguration(WebState* web_state) {
    WebStateImpl::FromWebState(web_state)
        ->GetWKWebViewConfigurationProvider()
        .GetWebViewConfiguration();
  }

  TestFeature feature_;
};

// Tests that a feature provided for a single WebState is only configured in
// that WebState.
TEST_F(WebStateJavaScriptFeaturesTest, FeatureOnlyConfiguredForItsWebState) {
  std::unique_ptr<WebState> web_state1 = CreateWebState();
  std::unique_ptr<WebState> web_state2 = CreateWebState();
  web_client()->SetWebStateJavaScriptFeatures(web_state1.get(), {&feature_});

  CreateConfiguration(web_state1.get());
  CreateConfiguration(web_state2.get());

  EXPECT_TRUE(JavaScriptFeatureManager::FromWebState(web_state1.get())
                  ->GetContentWorldForFeature(&feature_));
  EXPECT_FALSE(JavaScriptFeatureManager::FromWebState(web_state2.get())
                   ->GetContentWorldForFeature(&feature_));
  EXPECT_EQ(
      (std::vector<raw_ptr<WebState>>{web_state1.get(), web_state2.get()}),
      web_client()->requested_web_states());
}

// Tests that a web view built without a WebState doesn't request per-WebState
// features.
TEST_F(WebStateJavaScriptFeaturesTest, NotRequestedWithoutWebState) {
  WKWebView* web_view = BuildWKWebView(CGRectZero, GetBrowserState());
  EXPECT_TRUE(web_view);
  EXPECT_TRUE(web_client()->requested_web_states().empty());
}

}  // namespace web
