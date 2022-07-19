/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/renderer/android/vpn_render_frame_observer.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/skus/common/features.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/render_view_test.h"
#include "url/gurl.h"

namespace brave_vpn {

class VpnRenderFrameObserverBrowserTest : public content::RenderViewTest {
 public:
  VpnRenderFrameObserverBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
  }
  ~VpnRenderFrameObserverBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(VpnRenderFrameObserverBrowserTest, IsAllowed) {
  VpnRenderFrameObserver observer(GetMainRenderFrame(),
                                  content::ISOLATED_WORLD_ID_GLOBAL);
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.com/?intent=connect-receipt&product=vpn");

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

  // no recepit
  LoadHTMLWithUrlOverride(
      R"(<html><body></body></html>)",
      "https://account.brave.software/?intent=&product=vpn");

  EXPECT_FALSE(observer.IsAllowed());

  // wrong recepit
  LoadHTMLWithUrlOverride(R"(<html><body></body></html>)",
                          "https://account.brave.software/?product=vpn");

  EXPECT_FALSE(observer.IsAllowed());

  // wrong recepit
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

}  // namespace brave_vpn
