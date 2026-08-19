// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_javascript_feature.h"

#import <Foundation/Foundation.h>

#include "base/apple/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/ios/wait_util.h"
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper.h"
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper_bridge.h"
#include "ios/chrome/test/ios_chrome_test_with_web_state.h"
#include "ios/web/public/test/fakes/fake_web_client.h"
#include "ios/web/public/test/web_state_test_util.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

@interface FakeCosmeticFilteringTabHelperBridge
    : NSObject <CosmeticFilteringTabHelperBridge>

// Selectors returned in the `CosmeticFilteringArgs` for any URL.
@property(nonatomic, strong) NSSet<NSString*>* standardSelectors;
@property(nonatomic, strong) NSSet<NSString*>* aggressiveSelectors;

// Procedural filters (each a JSON-encoded `ProceduralOrActionFilter`) returned
// in the `CosmeticFilteringArgs` for any URL.
@property(nonatomic, strong) NSSet<NSString*>* proceduralFilters;

// Selectors returned from `selectorsToHideFor:ids:classes:completion:` in
// response to the ids/classes the script discovers on the page.
@property(nonatomic, strong) NSSet<NSString*>* hideStandardSelectors;
@property(nonatomic, strong) NSSet<NSString*>* hideAggressiveSelectors;

@end

@implementation FakeCosmeticFilteringTabHelperBridge

- (instancetype)init {
  if ((self = [super init])) {
    _standardSelectors = [NSSet set];
    _aggressiveSelectors = [NSSet set];
    _proceduralFilters = [NSSet set];
    _hideStandardSelectors = [NSSet set];
    _hideAggressiveSelectors = [NSSet set];
  }
  return self;
}

- (void)cosmeticFilteringArgsFor:(NSURL*)url
                      completion:(void (^)(CosmeticFilteringArgs*))completion {
  completion([[CosmeticFilteringArgs alloc]
          initWithHideFirstPartyContent:NO
                            genericHide:NO
           firstSelectorsPollingDelayMs:nil
      switchToSelectorsPollingThreshold:nil
       fetchNewClassIdRulesThrottlingMs:nil
                    aggressiveSelectors:self.aggressiveSelectors
                      standardSelectors:self.standardSelectors
                      proceduralFilters:self.proceduralFilters]);
}

- (void)selectorsToHideFor:(NSURL*)frameURL
                       ids:(NSSet<NSString*>*)ids
                   classes:(NSSet<NSString*>*)classes
                completion:(void (^)(NSSet<NSString*>* standardSelectors,
                                     NSSet<NSString*>* aggressiveSelectors))
                               completion {
  completion(self.hideStandardSelectors, self.hideAggressiveSelectors);
}

@end

class CosmeticFilteringJavaScriptFeatureTest
    : public IOSChromeTestWithWebState {
 protected:
  void SetUp() override {
    IOSChromeTestWithWebState::SetUp();

    static_cast<web::FakeWebClient*>(web::GetWebClient())
        ->SetJavaScriptFeatures(
            {CosmeticFilteringJavaScriptFeature::GetInstance()});

    bridge_ = [[FakeCosmeticFilteringTabHelperBridge alloc] init];
    CosmeticFilteringTabHelper::CreateForWebState(web_state());
    CosmeticFilteringTabHelper::FromWebState(web_state())->SetBridge(bridge_);
  }

  // The script applies its rules asynchronously (via an adopted stylesheet,
  // direct DOM mutation, or element removal), so poll the page until the given
  // boolean JavaScript `condition` evaluates to true.
  [[nodiscard]] bool WaitForCondition(NSString* condition) {
    return base::test::ios::WaitUntilConditionOrTimeout(
        base::test::ios::kWaitForJSCompletionTimeout, ^bool {
          id result = web::test::ExecuteJavaScript(condition, web_state());
          return [result isKindOfClass:[NSNumber class]] && [result boolValue];
        });
  }

  FakeCosmeticFilteringTabHelperBridge* bridge_;
};

#pragma mark Cosmetic Filters

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestHideElementWithArgs) {
  // Sent via initial CosmeticFilteringArgs
  bridge_.standardSelectors = [NSSet setWithObject:@"#test-ad"];

  NSString* html = @"<html><body>"
                   @"<div id='test-ad'></div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  // The script applies the hide rules asynchronously via an adopted stylesheet,
  // so poll until the element's computed style reports it as hidden.
  EXPECT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        id display = web::test::ExecuteJavaScript(
            @"window.getComputedStyle(document.getElementById('test-ad'))"
            @".display",
            web_state());
        return [display isEqual:@"none"];
      }));
}

TEST_F(CosmeticFilteringJavaScriptFeatureTest,
       TestHideElementWithSelectorMessage) {
  // Provide no CosmeticFilteringArgs, send selector via `SelectorsToHideFor`
  bridge_.hideStandardSelectors = [NSSet setWithObject:@"#test-ad"];

  NSString* html = @"<html><body>"
                   @"<div id='test-ad'></div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  // The script applies the hide rules asynchronously via an adopted stylesheet,
  // so poll until the element's computed style reports it as hidden.
  EXPECT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        id display = web::test::ExecuteJavaScript(
            @"window.getComputedStyle(document.getElementById('test-ad'))"
            @".display",
            web_state());
        return [display isEqual:@"none"];
      }));
}

#pragma mark Procedural Filters

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestProceduralRemoveElement) {
  // Rule: `brave.com###test-remove-element:remove()`
  bridge_.proceduralFilters =
      [NSSet setWithObject:
                 @"{\"selector\":[{\"type\":\"css-selector\",\"arg\":\"#"
                 @"test-remove-element\"}],\"action\":{\"type\":\"remove\"}}"];

  NSString* html = @"<html><body>"
                   @"<div id='test-remove-element'></div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  // The matching element is removed from the DOM entirely.
  EXPECT_TRUE(WaitForCondition(
      @"document.getElementById('test-remove-element') === null"));
}

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestProceduralRemoveClass) {
  // Rule: `brave.com###test-remove-class:remove-class(test)`
  bridge_.proceduralFilters = [NSSet
      setWithObject:@"{\"selector\":[{\"type\":\"css-selector\",\"arg\":\"#"
                    @"test-remove-class\"}],\"action\":{\"type\":\"remove-"
                    @"class\",\"arg\":\"test\"}}"];

  NSString* html = @"<html><body>"
                   @"<div id='test-remove-class' class='test'></div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  // The `test` class is removed from the matching element.
  EXPECT_TRUE(WaitForCondition(@"!document.getElementById('test-remove-class')"
                               @".classList.contains('test')"));
}

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestProceduralRemoveAttribute) {
  // Rule: `brave.com###test-remove-attribute:remove-attr(test)`
  bridge_.proceduralFilters = [NSSet
      setWithObject:@"{\"selector\":[{\"type\":\"css-selector\",\"arg\":\"#"
                    @"test-remove-attribute\"}],\"action\":{\"type\":\"remove-"
                    @"attr\",\"arg\":\"test\"}}"];

  NSString* html = @"<html><body>"
                   @"<div id='test-remove-attribute' test></div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  // The `test` attribute is removed from the matching element.
  EXPECT_TRUE(
      WaitForCondition(@"!document.getElementById('test-remove-attribute')"
                       @".hasAttribute('test')"));
}

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestProceduralStyleElement) {
  // Rule: `brave.com###test-style-element:style(background-color: red
  // !important)`
  bridge_.proceduralFilters = [NSSet
      setWithObject:@"{\"selector\":[{\"type\":\"css-selector\",\"arg\":\"#"
                    @"test-style-element\"}],\"action\":{\"type\":\"style\","
                    @"\"arg\":\"background-color: red !important\"}}"];

  NSString* html = @"<html><body>"
                   @"<div id='test-style-element'>Styled text</div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  // The custom style is applied to the matching element.
  EXPECT_TRUE(WaitForCondition(
      @"window.getComputedStyle(document.getElementById('test-style-element'))"
      @".backgroundColor === 'rgb(255, 0, 0)'"));
}

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestProceduralUpwardInt) {
  // Rule: `brave.com###test-upward-int-target:upward(2)`
  bridge_.proceduralFilters = [NSSet
      setWithObject:@"{\"selector\":[{\"type\":\"css-selector\",\"arg\":\"#"
                    @"test-upward-int-target\"},{\"type\":\"upward\",\"arg\":"
                    @"\"2\"}]}"];

  // `upward(2)` walks two parents up from the target, hiding
  // `#test-upward-int`.
  NSString* html = @"<html><body>"
                   @"<div id='test-upward-int'>"
                   @"<div><div id='test-upward-int-target'></div></div>"
                   @"</div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  EXPECT_TRUE(WaitForCondition(
      @"window.getComputedStyle(document.getElementById('test-upward-int'))"
      @".display === 'none'"));
}

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestProceduralUpwardSelector) {
  // Rule:
  // `brave.com###test-upward-selector-target:upward(#test-upward-selector)`
  bridge_.proceduralFilters = [NSSet
      setWithObject:@"{\"selector\":[{\"type\":\"css-selector\",\"arg\":\"#"
                    @"test-upward-selector-target\"},{\"type\":\"upward\","
                    @"\"arg\":\"#test-upward-selector\"}]}"];

  // `upward(#test-upward-selector)` walks up from the target until it matches
  // the given selector, hiding `#test-upward-selector`.
  NSString* html =
      @"<html><body>"
      @"<div id='test-upward-selector'>"
      @"<div><div><div id='test-upward-selector-target'></div></div></div>"
      @"</div>"
      @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  EXPECT_TRUE(WaitForCondition(@"window.getComputedStyle(document."
                               @"getElementById('test-upward-selector'))"
                               @".display === 'none'"));
}

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestProceduralHasText) {
  // Rule: `brave.com###test-has-text:has-text(hide me)`
  bridge_.proceduralFilters = [NSSet
      setWithObject:@"{\"selector\":[{\"type\":\"css-selector\",\"arg\":\"#"
                    @"test-has-text\"},{\"type\":\"has-text\",\"arg\":\"hide "
                    @"me\"}]}"];

  // The element's text contains "hide me", so it is hidden.
  NSString* html = @"<html><body>"
                   @"<div id='test-has-text'>Please hide me.</div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  EXPECT_TRUE(WaitForCondition(
      @"window.getComputedStyle(document.getElementById('test-has-text'))"
      @".display === 'none'"));
}

TEST_F(CosmeticFilteringJavaScriptFeatureTest, TestProceduralHas) {
  // Rule: `brave.com###test-has:has(a.banner-link)`
  // Note that `:has()` is native CSS, so the adblock engine treats this rule
  // as a plain hide selector rather than a procedural filter.
  bridge_.standardSelectors =
      [NSSet setWithObject:@"#test-has:has(a.banner-link)"];

  // The element matches because it has an `a.banner-link` descendant.
  NSString* html = @"<html><body>"
                   @"<div id='test-has'><a class='banner-link'>link</a></div>"
                   @"</body></html>";
  web::test::LoadHtml(html, GURL("https://brave.com/"), web_state());

  EXPECT_TRUE(WaitForCondition(
      @"window.getComputedStyle(document.getElementById('test-has'))"
      @".display === 'none'"));
}
