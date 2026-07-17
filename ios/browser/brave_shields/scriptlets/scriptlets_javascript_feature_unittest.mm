// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/scriptlets/scriptlets_javascript_feature.h"

#import <Foundation/Foundation.h>

#include <optional>
#include <string>

#include "base/apple/foundation_util.h"
#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/ios/wait_util.h"
#include "brave/components/cosmetic_filters/common/scriptlet_constants.h"
#include "brave/ios/browser/api/brave_shields/adblock_engine.h"
#include "brave/ios/browser/brave_shields/scriptlets/scriptlets_tab_helper.h"
#include "brave/ios/browser/brave_shields/scriptlets/scriptlets_tab_helper_bridge.h"
#include "ios/chrome/test/ios_chrome_test_with_web_state.h"
#include "ios/web/public/test/fakes/fake_web_client.h"
#include "ios/web/public/test/web_state_test_util.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "url/gurl.h"

namespace {

// Builds an ad-block resources JSON containing a single scriptlet with the
// given `name` and `scriptlet` body, matching the approach used by
// `AdBlockServiceTest.CosmeticFilteringAboutBlankScriptlet`.
std::string ResourcesJSON(const std::string& name,
                          const std::string& scriptlet) {
  return base::StrCat({"[{"
                       "\"name\": \"",
                       name,
                       "\","
                       "\"aliases\": [\"",
                       name,
                       "\"],"
                       "\"kind\": {\"mime\": \"application/javascript\"},"
                       "\"content\": \"",
                       base::Base64Encode(scriptlet), "\"}]"});
}

// Builds an ad-block resources JSON containing the `set.js` scriptlet, whose
// body assigns `window.sval = true`.
std::string SetResourcesJSON() {
  return ResourcesJSON("set.js", "(function() {"
                                 "  window.sval = true;"
                                 "})();");
}

}  // namespace

@interface FakeScriptletsTabHelperBridge : NSObject <ScriptletsTabHelperBridge>
// Scriptlets returned to the feature for any requested frame.
@property(nonatomic, copy) NSArray<NSString*>* scriptlets;
@end

@implementation FakeScriptletsTabHelperBridge

- (instancetype)init {
  if ((self = [super init])) {
    _scriptlets = @[];
  }
  return self;
}

- (void)requestScriptletsForFrameURL:(NSURL*)frameURL
                          completion:(void (^)(NSArray<NSString*>* scriptlets))
                                         completion {
  completion(self.scriptlets);
}

@end

class ScriptletsJavaScriptFeatureTest : public IOSChromeTestWithWebState {
 protected:
  void SetUp() override {
    IOSChromeTestWithWebState::SetUp();

    static_cast<web::FakeWebClient*>(web::GetWebClient())
        ->SetJavaScriptFeatures({ScriptletsJavaScriptFeature::GetInstance()});

    bridge_ = [[FakeScriptletsTabHelperBridge alloc] init];
    ScriptletsTabHelper::CreateForWebState(web_state());
    ScriptletsTabHelper::FromWebState(web_state())->SetBridge(bridge_);
  }

  // Builds the scriptlet(s) an AdblockEngine produces for `rules` at
  // `frame_url` the same way the Swift `ScriptletsTabHelper` does.
  // `is_de_amp_enabled` toggles the injected `deAmpEnabled` global.
  NSArray<NSString*>* ScriptletsForRules(
      NSString* rules,
      NSString* frame_url,
      const std::string& resources_json = SetResourcesJSON(),
      bool is_de_amp_enabled = false) {
    [AdblockEngine setDomainResolver];

    NSError* error = nil;
    AdblockEngine* engine = [[AdblockEngine alloc] initWithRules:rules
                                                           error:&error];
    if (!engine) {
      return nil;
    }

    if (![engine useResources:base::SysUTF8ToNSString(resources_json)]) {
      return nil;
    }

    NSString* cosmetic = [engine cosmeticResourcesForURL:frame_url];
    std::optional<base::Value> parsed = base::JSONReader::Read(
        base::SysNSStringToUTF8(cosmetic), base::JSON_PARSE_RFC);
    if (!parsed || !parsed->is_dict()) {
      return nil;
    }
    const std::string* injected_script =
        parsed->GetDict().FindString("injected_script");
    if (!injected_script || injected_script->empty()) {
      return nil;
    }

    const std::string scriptlet_globals =
        cosmetic_filters::GetScriptletGlobalsScript(is_de_amp_enabled,
                                                    /*can_debug=*/false);
    return @[ base::SysUTF8ToNSString(
        base::StrCat({scriptlet_globals, *injected_script})) ];
  }

  FakeScriptletsTabHelperBridge* bridge_;
};

// Test the rule `brave.com##+js(set)` will inject the `set` scriptlet, which
// assigns `window.sval = true`.
TEST_F(ScriptletsJavaScriptFeatureTest, TestScriptletSet) {
  NSArray<NSString*>* scriptlets =
      ScriptletsForRules(@"brave.com##+js(set)", @"https://brave.com");
  ASSERT_TRUE(scriptlets);
  ASSERT_EQ(1u, scriptlets.count);
  bridge_.scriptlets = scriptlets;

  web::test::LoadHtml(@"<html><body></body></html>", GURL("https://brave.com"),
                      web_state());

  __block id result = nil;
  ASSERT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        result = web::test::ExecuteJavaScript(@"window.sval", web_state());
        return result != nil;
      }));
  EXPECT_NSEQ(@YES, result);
}

// Test that the `set` scriptlet is injected into `about:blank` child frames.
// The page hosts an `about:blank` iframe, and the scriptlet assigns
// `window.sval = true` within that frame.
TEST_F(ScriptletsJavaScriptFeatureTest, TestScriptletSetAboutBlank) {
  NSArray<NSString*>* scriptlets =
      ScriptletsForRules(@"brave.com##+js(set)", @"https://brave.com");
  ASSERT_TRUE(scriptlets);
  ASSERT_EQ(1u, scriptlets.count);
  bridge_.scriptlets = scriptlets;

  web::test::LoadHtml(
      @"<html><body><iframe src=\"about:blank\"></iframe></body></html>",
      GURL("https://brave.com"), web_state());

  __block id result = nil;
  ASSERT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        result = web::test::ExecuteJavaScript(
            @"document.querySelector('iframe').contentWindow.sval",
            web_state());
        return result != nil;
      }));
  EXPECT_NSEQ(@YES, result);
}

// Mirrors `AdBlockServiceTest.CheckForDeAmpPref`. The `deamp` scriptlet reads
// the injected `deAmpEnabled` global and overrides `window.getComputedStyle` to
// report a different color depending on its value. Toggling the global (as the
// De-AMP pref does) is reflected in the injected scriptlet's behavior.
TEST_F(ScriptletsJavaScriptFeatureTest, CheckForDeAmpPref) {
  const std::string resources_json = ResourcesJSON(
      "deamp.js", "(function() {"
                  " if (deAmpEnabled) {"
                  "   window.getComputedStyle = function(selector) {"
                  "     return { 'color': 'green' };"
                  "   }"
                  " } else {"
                  "   window.getComputedStyle = function(selector) {"
                  "     return { 'color': 'red' };"
                  "   }"
                  " }"
                  "})();");

  // De-AMP enabled: the scriptlet reports 'green'.
  NSArray<NSString*>* enabled_scriptlets = ScriptletsForRules(
      @"brave.com##+js(deamp)", @"https://brave.com", resources_json,
      /*is_de_amp_enabled=*/true);
  ASSERT_TRUE(enabled_scriptlets);
  ASSERT_EQ(1u, enabled_scriptlets.count);
  bridge_.scriptlets = enabled_scriptlets;

  web::test::LoadHtml(@"<html><body></body></html>", GURL("https://brave.com"),
                      web_state());

  EXPECT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        id result = web::test::ExecuteJavaScript(
            @"window.getComputedStyle(document.body).color", web_state());
        return [result isEqual:@"green"];
      }));

  // De-AMP disabled: the scriptlet reports 'red'.
  NSArray<NSString*>* disabled_scriptlets = ScriptletsForRules(
      @"brave.com##+js(deamp)", @"https://brave.com", resources_json,
      /*is_de_amp_enabled=*/false);
  ASSERT_TRUE(disabled_scriptlets);
  ASSERT_EQ(1u, disabled_scriptlets.count);
  bridge_.scriptlets = disabled_scriptlets;

  web::test::LoadHtml(@"<html><body></body></html>", GURL("https://brave.com"),
                      web_state());

  EXPECT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        id result = web::test::ExecuteJavaScript(
            @"window.getComputedStyle(document.body).color", web_state());
        return [result isEqual:@"red"];
      }));
}
