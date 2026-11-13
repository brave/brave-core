// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/request_blocking/request_blocking_javascript_feature.h"

#import <Foundation/Foundation.h>

#include <memory>
#include <string>

#include "base/apple/foundation_util.h"
#include "base/functional/bind.h"
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
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
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

namespace {

constexpr char kResourcePath[] = "/resource.json";

std::unique_ptr<net::test_server::HttpResponse> HandleResourceRequest(
    const net::test_server::HttpRequest& request) {
  if (request.relative_url != kResourcePath) {
    return nullptr;
  }
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content_type("application/json");
  response->set_content("{}");
  return response;
}

}  // namespace

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

    test_server_.RegisterRequestHandler(
        base::BindRepeating(&HandleResourceRequest));
    ASSERT_TRUE(test_server_.Start());

    web::test::LoadHtml(@"<html></html>", test_server_.base_url(), web_state());
  }

  GURL ResourceURL() { return test_server_.GetURL(kResourcePath); }

  // Runs `script` (with `$URL` replaced by the resource URL), waits for the
  // bridge to be consulted, verifies the forwarded parameters and returns the
  // resulting `window.sval`.
  std::string RunRequest(NSString* script) {
    NSString* url = base::SysUTF8ToNSString(ResourceURL().spec());
    web::test::ExecuteJavaScript(
        [script stringByReplacingOccurrencesOfString:@"$URL" withString:url],
        web_state());

    EXPECT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
        base::test::ios::kWaitForJSCompletionTimeout, ^bool {
          return bridge_.requestReceived;
        }));

    EXPECT_EQ(ResourceURL(), net::GURLWithNSURL(bridge_.lastRequestURL));
    EXPECT_EQ("xmlhttprequest",
              base::SysNSStringToUTF8(bridge_.lastResourceType));
    EXPECT_EQ(test_server_.base_url(),
              net::GURLWithNSURL(bridge_.lastSourceURL));

    __block NSString* result = nil;
    EXPECT_TRUE(base::test::ios::WaitUntilConditionOrTimeout(
        base::test::ios::kWaitForJSCompletionTimeout, ^bool {
          result = base::apple::ObjCCast<NSString>(
              web::test::ExecuteJavaScript(@"window.sval", web_state()));
          return result != nil;
        }));
    return base::SysNSStringToUTF8(result);
  }

  FakeRequestBlockingTabHelperBridge* bridge_;
  net::EmbeddedTestServer test_server_;
};

// `true;` at the end keeps the response serializable for WebKit/obj-c.
NSString* const kFetchScript =
    @"window.sval = undefined;"
    @"fetch('$URL')"
    @"    .then(() => { window.sval = 'fetched'; })"
    @"    .catch(() => { window.sval = 'blocked'; });"
    @"true;";

NSString* const kXHRScript =
    @"window.sval = undefined;"
    @"var xhr = new XMLHttpRequest();"
    @"xhr.open('GET', '$URL');"
    @"xhr.onload = () => { window.sval = 'fetched'; };"
    @"xhr.onerror = () => { window.sval = 'blocked'; };"
    @"xhr.send();"
    @"true;";

// Tests that a `fetch()` is forwarded to the bridge and a blocking decision
// rejects it.
TEST_F(RequestBlockingJavaScriptFeatureTest, TestFetchRequestBlocked) {
  bridge_.shouldBlock = YES;
  ASSERT_FALSE(bridge_.requestReceived);
  EXPECT_EQ("blocked", RunRequest(kFetchScript));
}

// Tests that a `fetch()` is forwarded to the bridge and an allow decision lets
// it complete.
TEST_F(RequestBlockingJavaScriptFeatureTest, TestFetchRequestAllowed) {
  bridge_.shouldBlock = NO;
  ASSERT_FALSE(bridge_.requestReceived);
  EXPECT_EQ("fetched", RunRequest(kFetchScript));
}

// Tests that an `XMLHttpRequest` is forwarded to the bridge and a blocking
// decision fails it.
TEST_F(RequestBlockingJavaScriptFeatureTest, TestXHRBlocked) {
  bridge_.shouldBlock = YES;
  ASSERT_FALSE(bridge_.requestReceived);
  EXPECT_EQ("blocked", RunRequest(kXHRScript));
}

// Tests that an `XMLHttpRequest` is forwarded to the bridge and an allow
// decision lets it complete.
TEST_F(RequestBlockingJavaScriptFeatureTest, TestXHRAllowed) {
  bridge_.shouldBlock = NO;
  ASSERT_FALSE(bridge_.requestReceived);
  EXPECT_EQ("fetched", RunRequest(kXHRScript));
}
