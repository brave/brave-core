// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <WebKit/WebKit.h>

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/web/common/web_view_creation_util.h"
#include "ios/web/js_messaging/java_script_feature_manager.h"
#include "ios/web/public/js_messaging/content_world.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/test/fakes/fake_web_client.h"
#include "ios/web/public/test/web_test.h"
#include "ios/web/public/test/web_test_with_web_state.h"
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

// A feature whose script and signature can change, like a feature whose
// script depends on a preference.
class MutableScriptFeature : public JavaScriptFeature {
 public:
  MutableScriptFeature() : JavaScriptFeature(ContentWorld::kIsolatedWorld) {}

  void SetScript(const std::string& script) { script_ = script; }
  void SetSignature(const std::string& signature) { signature_ = signature; }

  // JavaScriptFeature:
  std::vector<FeatureScript> GetScripts() const override {
    return {FeatureScript::CreateWithString(
        script_, FeatureScript::InjectionTime::kDocumentStart,
        FeatureScript::TargetFrames::kMainFrame)};
  }
  std::string GetScriptsSignature() const override { return signature_; }

 private:
  std::string script_;
  std::string signature_;
};

// Returns whether a user script of `web_state`'s configuration contains
// `text`.
bool HasUserScriptContaining(WebState* web_state, const std::string& text) {
  WKWebViewConfiguration* configuration =
      WebStateImpl::FromWebState(web_state)
          ->GetWKWebViewConfigurationProvider()
          .GetWebViewConfiguration();
  NSString* ns_text = base::SysUTF8ToNSString(text);
  for (WKUserScript* script in configuration.userContentController
           .userScripts) {
    if ([script.source containsString:ns_text]) {
      return true;
    }
  }
  return false;
}

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

  void UpdateScriptsIfNeeded(WebState* web_state) {
    WebStateImpl::FromWebState(web_state)
        ->GetWKWebViewConfigurationProvider()
        .UpdateScriptsIfNeeded();
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

// Tests that the scripts are rebuilt when the features provided for a WebState
// change.
TEST_F(WebStateJavaScriptFeaturesTest, UpdateScriptsIfNeededFeaturesChanged) {
  std::unique_ptr<WebState> web_state = CreateWebState();
  CreateConfiguration(web_state.get());
  JavaScriptFeatureManager* manager =
      JavaScriptFeatureManager::FromWebState(web_state.get());
  ASSERT_FALSE(manager->GetContentWorldForFeature(&feature_));

  web_client()->SetWebStateJavaScriptFeatures(web_state.get(), {&feature_});
  UpdateScriptsIfNeeded(web_state.get());
  EXPECT_TRUE(manager->GetContentWorldForFeature(&feature_));

  web_client()->SetWebStateJavaScriptFeatures(web_state.get(), {});
  UpdateScriptsIfNeeded(web_state.get());
  EXPECT_FALSE(manager->GetContentWorldForFeature(&feature_));
}

// Tests that the scripts are only rebuilt when the signature of a feature's
// scripts changes.
TEST_F(WebStateJavaScriptFeaturesTest, UpdateScriptsIfNeededSignatureChanged) {
  MutableScriptFeature feature;
  feature.SetScript("/* script-a */");
  feature.SetSignature("a");
  std::unique_ptr<WebState> web_state = CreateWebState();
  web_client()->SetWebStateJavaScriptFeatures(web_state.get(), {&feature});
  CreateConfiguration(web_state.get());
  ASSERT_TRUE(HasUserScriptContaining(web_state.get(), "script-a"));

  // The signature didn't change, so the scripts are not rebuilt.
  feature.SetScript("/* script-b */");
  UpdateScriptsIfNeeded(web_state.get());
  EXPECT_TRUE(HasUserScriptContaining(web_state.get(), "script-a"));
  EXPECT_FALSE(HasUserScriptContaining(web_state.get(), "script-b"));

  feature.SetSignature("b");
  UpdateScriptsIfNeeded(web_state.get());
  EXPECT_FALSE(HasUserScriptContaining(web_state.get(), "script-a"));
  EXPECT_TRUE(HasUserScriptContaining(web_state.get(), "script-b"));
}

// Tests that the scripts are not built before the configuration is created.
TEST_F(WebStateJavaScriptFeaturesTest, UpdateScriptsIfNeededNoConfiguration) {
  std::unique_ptr<WebState> web_state = CreateWebState();
  UpdateScriptsIfNeeded(web_state.get());
  EXPECT_TRUE(web_client()->requested_web_states().empty());
}

class WebStateJavaScriptFeaturesNavigationTest : public WebTestWithWebState {
 protected:
  WebStateJavaScriptFeaturesNavigationTest()
      : WebTestWithWebState(std::make_unique<FakeWebClient>()) {}

  FakeWebClient* web_client() {
    return static_cast<FakeWebClient*>(GetWebClient());
  }

  MutableScriptFeature feature_;
};

// Tests that a main frame navigation rebuilds the scripts after a feature's
// signature changed.
TEST_F(WebStateJavaScriptFeaturesNavigationTest, NavigationUpdatesScripts) {
  feature_.SetScript("/* script-a */");
  feature_.SetSignature("a");
  web_client()->SetWebStateJavaScriptFeatures(web_state(), {&feature_});
  ASSERT_TRUE(LoadHtml("<html><body>1</body></html>"));
  ASSERT_TRUE(HasUserScriptContaining(web_state(), "script-a"));

  feature_.SetScript("/* script-b */");
  feature_.SetSignature("b");
  ASSERT_TRUE(LoadHtml("<html><body>2</body></html>"));
  EXPECT_FALSE(HasUserScriptContaining(web_state(), "script-a"));
  EXPECT_TRUE(HasUserScriptContaining(web_state(), "script-b"));
}

}  // namespace web
