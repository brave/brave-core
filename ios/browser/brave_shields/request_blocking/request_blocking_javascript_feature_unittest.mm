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
#include "ios/chrome/browser/shared/model/profile/test/test_profile_ios.h"
#include "ios/web/public/test/fakes/fake_web_client.h"
#include "ios/web/public/test/js_test_util.h"
#include "ios/web/public/test/scoped_testing_web_client.h"
#include "ios/web/public/test/web_state_test_util.h"
#include "ios/web/public/test/web_task_environment.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
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

class RequestBlockingJavaScriptFeatureTest : public PlatformTest {
 protected:
  RequestBlockingJavaScriptFeatureTest()
      : web_client_(std::make_unique<web::FakeWebClient>()) {}

  void SetUp() override {
    PlatformTest::SetUp();

    profile_ = TestProfileIOS::Builder().Build();

    web::WebState::CreateParams params(profile_.get());
    web_state_ = web::WebState::Create(params);
    web_state_->GetView();
    web_state_->SetKeepRenderProcessAlive(true);

    GetWebClient()->SetJavaScriptFeatures(
        {RequestBlockingJavaScriptFeature::GetInstance()});

    bridge_ = [[FakeRequestBlockingTabHelperBridge alloc] init];
    RequestBlockingTabHelper::CreateForWebState(web_state_.get());
    RequestBlockingTabHelper::FromWebState(web_state_.get())
        ->SetBridge(bridge_);

    // The feature only forwards requests for HTTP(S) frames, so load the page
    // from an HTTPS origin.
    web::test::LoadHtml(@"<html></html>", GURL("https://brave.com/"),
                        web_state_.get());
  }

  web::FakeWebClient* GetWebClient() {
    return static_cast<web::FakeWebClient*>(web_client_.Get());
  }

  web::WebState* web_state() { return web_state_.get(); }

  // Executes `script` in the feature's content world (where the injected
  // request blocking script patches `fetch`/`XMLHttpRequest`) and returns the
  // script's completion value.
  id ExecuteJavaScript(NSString* script) {
    return web::test::ExecuteJavaScriptForFeatureAndReturnResult(
        web_state(), script, RequestBlockingJavaScriptFeature::GetInstance());
  }

  web::ScopedTestingWebClient web_client_;
  web::WebTaskEnvironment task_environment_;
  std::unique_ptr<TestProfileIOS> profile_;
  std::unique_ptr<web::WebState> web_state_;
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
  ExecuteJavaScript(@"window.sval = undefined;"
                    @"fetch('https://brave.com/resource.json')"
                    @"    .then(() => { window.sval = 'fetched'; })"
                    @"    .catch(() => { window.sval = 'blocked'; });"
                    @"true;");  // keep response serializable for WebKit/obj-c

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
        result =
            base::apple::ObjCCast<NSString>(ExecuteJavaScript(@"window.sval"));
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
  ExecuteJavaScript(@"window.sval = undefined;"
                    @"var xhr = new XMLHttpRequest();"
                    @"xhr.open('GET', 'https://brave.com/resource.json');"
                    @"xhr.onload = () => { window.sval = 'fetched'; };"
                    @"xhr.onerror = () => { window.sval = 'blocked'; };"
                    @"xhr.send();"
                    @"true;");  // keep response serializable for WebKit/obj-c

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
        result =
            base::apple::ObjCCast<NSString>(ExecuteJavaScript(@"window.sval"));
        return result != nil;
      }));
  EXPECT_EQ("blocked", base::SysNSStringToUTF8(result));
}
