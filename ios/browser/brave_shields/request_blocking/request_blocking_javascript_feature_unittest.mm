// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/request_blocking/request_blocking_javascript_feature.h"

#import <Foundation/Foundation.h>

#include "base/apple/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/ios/wait_util.h"
#include "brave/ios/browser/brave_shields/request_blocking/request_blocking_tab_helper.h"
#include "brave/ios/browser/brave_shields/request_blocking/request_blocking_tab_helper_bridge.h"
#include "ios/chrome/test/ios_chrome_test_with_web_state.h"
#include "ios/web/public/test/fakes/fake_web_client.h"
#include "ios/web/public/test/web_state_test_util.h"
#include "ios/web/public/web_client.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// Fake bridge that records the parameters of the most recently received request
// and replies with a configurable blocking decision so tests can assert against
// what the feature forwarded.
@interface FakeRequestBlockingTabHelperBridge
    : NSObject <RequestBlockingTabHelperBridge>

// The decision returned to the feature for incoming requests.
@property(nonatomic) BOOL shouldBlock;
// Whether a request has been received from the feature.
@property(nonatomic) BOOL requestReceived;
// Parameters from the most recently received request.
@property(nonatomic, copy, nullable) NSURL* lastRequestURL;
@property(nonatomic, copy, nullable) NSURL* lastSourceURL;
@property(nonatomic, copy, nullable) NSString* lastResourceType;

@end

@implementation FakeRequestBlockingTabHelperBridge

- (void)shouldBlockRequestURL:(NSURL*)requestURL
                    sourceURL:(NSURL*)sourceURL
                 resourceType:(NSString*)resourceType
                   completion:(void (^)(BOOL shouldBlock))completion {
  self.lastRequestURL = requestURL;
  self.lastSourceURL = sourceURL;
  self.lastResourceType = resourceType;
  self.requestReceived = YES;
  completion(self.shouldBlock);
}

@end

class RequestBlockingJavaScriptFeatureTest : public IOSChromeTestWithWebState {
 protected:
  void SetUp() override {
    IOSChromeTestWithWebState::SetUp();

    static_cast<web::FakeWebClient*>(web::GetWebClient())
        ->SetJavaScriptFeatures(
            {RequestBlockingJavaScriptFeature::GetInstance()});

    bridge_ = [[FakeRequestBlockingTabHelperBridge alloc] init];
    RequestBlockingTabHelper::CreateForWebState(web_state());
    RequestBlockingTabHelper::FromWebState(web_state())->SetBridge(bridge_);

    web::test::LoadHtml(@"<html></html>", GURL("https://brave.com/"),
                        web_state());
  }

  FakeRequestBlockingTabHelperBridge* bridge_;
};

// Tests that a `fetch()` issued by the page forwards the request to the bridge
// `ShouldBlock` and that a blocking decision rejects the page's `fetch()`.
TEST_F(RequestBlockingJavaScriptFeatureTest, TestFetchRequest) {
  bridge_.shouldBlock = YES;
  ASSERT_FALSE(bridge_.requestReceived);
  ASSERT_FALSE(bridge_.lastResourceType);
  ASSERT_FALSE(bridge_.lastSourceURL);

  // Perform a fetch request and store the value on window.sval
  web::test::ExecuteJavaScript(
      @"window.sval = undefined;"
      @"fetch('https://brave.com/resource.json')"
      @"    .then(() => { window.sval = 'fetched'; })"
      @"    .catch(() => { window.sval = 'blocked'; });"
      @"true;",  // keep response serializable for WebKit/obj-c
      web_state());

  // wait for it to be received by the bridge
  ASSERT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        return bridge_.requestReceived;
      }));

  // verify bridge received correct info sent via fetch()
  EXPECT_EQ(GURL("https://brave.com/resource.json"),
            net::GURLWithNSURL(bridge_.lastRequestURL));
  EXPECT_EQ("xmlhttprequest",
            base::SysNSStringToUTF8(bridge_.lastResourceType));
  EXPECT_EQ(GURL("https://brave.com"),
            net::GURLWithNSURL(bridge_.lastSourceURL));

  // Assert that the request was blocked
  __block NSString* result = nil;
  ASSERT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        result = base::apple::ObjCCast<NSString>(
            web::test::ExecuteJavaScript(@"window.sval", web_state()));
        return result != nil;
      }));
  EXPECT_EQ("blocked", base::SysNSStringToUTF8(result));
}

// Tests that an `XMLHttpRequest` issued by the page forwards the request to the
// bridge `ShouldBlock` and that a blocking decision rejects the page's request.
TEST_F(RequestBlockingJavaScriptFeatureTest, TestXHR) {
  bridge_.shouldBlock = YES;
  ASSERT_FALSE(bridge_.requestReceived);
  ASSERT_FALSE(bridge_.lastResourceType);
  ASSERT_FALSE(bridge_.lastSourceURL);

  // Perform an XMLHttpRequest and store the value on window.sval
  web::test::ExecuteJavaScript(
      @"window.sval = undefined;"
      @"var xhr = new XMLHttpRequest();"
      @"xhr.open('GET', 'https://brave.com/resource.json');"
      @"xhr.onload = () => { window.sval = 'fetched'; };"
      @"xhr.onerror = () => { window.sval = 'blocked'; };"
      @"xhr.send();"
      @"true;",  // keep response serializable for WebKit/obj-c
      web_state());

  // wait for it to be received by the bridge
  ASSERT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        return bridge_.requestReceived;
      }));

  // verify bridge received correct info sent via XMLHttpRequest
  EXPECT_EQ(GURL("https://brave.com/resource.json"),
            net::GURLWithNSURL(bridge_.lastRequestURL));
  EXPECT_EQ("xmlhttprequest",
            base::SysNSStringToUTF8(bridge_.lastResourceType));
  EXPECT_EQ(GURL("https://brave.com"),
            net::GURLWithNSURL(bridge_.lastSourceURL));

  // Assert that the request was blocked
  __block NSString* result = nil;
  ASSERT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
      base::test::ios::kWaitForJSCompletionTimeout, ^bool {
        result = base::apple::ObjCCast<NSString>(
            web::test::ExecuteJavaScript(@"window.sval", web_state()));
        return result != nil;
      }));
  EXPECT_EQ("blocked", base::SysNSStringToUTF8(result));
}
