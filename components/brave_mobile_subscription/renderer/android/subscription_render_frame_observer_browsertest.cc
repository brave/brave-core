/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_mobile_subscription/renderer/android/subscription_render_frame_observer.h"
#include "brave/components/skus/renderer/skus_render_frame_observer.h"

#include "base/test/scoped_feature_list.h"
//#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/skus/common/features.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/render_view_test.h"
#include "url/gurl.h"

// namespace {
// const int kMaxTimeoutPerLoadedURL = 200;
// void WaitForTimeout(int timeout) {
//     base::RunLoop run_loop;
//     base::OneShotTimer timeout_1;
//     timeout_1.Start(FROM_HERE, base::Seconds(0), run_loop.QuitClosure());
//     run_loop.Run();
// //   base::test::ScopedRunLoopTimeout file_download_timeout(
// //       FROM_HERE, base::Seconds(kMaxTimeoutPerLoadedURL + 1));
// //   base::RunLoop run_loop;
// //   base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
// //       FROM_HERE, run_loop.QuitClosure(), base::Seconds(timeout));
// //   run_loop.Run();
// }
// }

namespace brave_subscription {

class SubscriptionRenderFrameObserverBrowserTest
    : public content::RenderViewTest {
 public:
  SubscriptionRenderFrameObserverBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN,
         ai_chat::features::kAIChat, skus::features::kSkusFeature},
        {});
  }
  ~SubscriptionRenderFrameObserverBrowserTest() override = default;

  bool ExecuteJavascript(const std::u16string& script) {
    int result = -1;
    LOG(ERROR) << "!!!here0";
    EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(script, &result));
    LOG(ERROR) << "!!!here01";
    return result == 1;
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(SubscriptionRenderFrameObserverBrowserTest, StatusCheck) {
  SubscriptionRenderFrameObserver observer(GetMainRenderFrame(),
                                            content::ISOLATED_WORLD_ID_GLOBAL);
  //skus::SkusRenderFrameObserver observer(GetMainRenderFrame());
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.com/?intent=link-order&product=leo");

  base::RunLoop().RunUntilIdle();
  LOG(ERROR) << "!!!before wait";
  //WaitForTimeout(kMaxTimeoutPerLoadedURL);
  //task_environment_.AdvanceClock(base::Microseconds(100));
  LOG(ERROR) << "!!!after wait";
  std::u16string command = //u"Number((window.chrome  !== undefined) && (window.chrome.braveSkus !== "
      //u"undefined) && "
      //u"(window.chrome.braveSkus.refresh_order !== undefined))";
  /*u"Number(console.log('hi') == undefined)";*/u"Number(linkState != undefined)";
  EXPECT_TRUE(ExecuteJavascript(command));
}

TEST_F(SubscriptionRenderFrameObserverBrowserTest, IsAllowed) {
  SubscriptionRenderFrameObserver observer(GetMainRenderFrame(),
                                           content::ISOLATED_WORLD_ID_GLOBAL);
  // VPN
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.com/?intent=connect-receipt&product=vpn");

  EXPECT_TRUE(observer.IsAllowed());

  // Leo
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.com/?intent=link-order&product=leo");

  EXPECT_TRUE(observer.IsAllowed());

  // http
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "http://account.brave.com/?intent=connect-receipt&product=vpn");

  EXPECT_FALSE(observer.IsAllowed());

  // https://account.bravesoftware.com
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.bravesoftware.com/?intent=connect-receipt&product=vpn");

  EXPECT_TRUE(observer.IsAllowed());

  // https://account.brave.software
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.software/?intent=connect-receipt&product=vpn");

  EXPECT_TRUE(observer.IsAllowed());

  // no receipt
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.software/?intent=&product=vpn");

  EXPECT_FALSE(observer.IsAllowed());

  // wrong receipt
  LoadHTMLWithUrlOverride(R"(<html><body></body></html>)",
                          "https://account.brave.software/?product=vpn");

  EXPECT_FALSE(observer.IsAllowed());

  // wrong receipt
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.software/?intent=wrong&product=vpn");

  EXPECT_FALSE(observer.IsAllowed());

  // no product
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.software/?intent=connect-receipt&product=");

  EXPECT_FALSE(observer.IsAllowed());

  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.software/?intent=connect-receipt");

  EXPECT_FALSE(observer.IsAllowed());

  // wrong product
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.software/?intent=connect-receipt&product=wrong");

  EXPECT_FALSE(observer.IsAllowed());
}

}  // namespace brave_subscription
