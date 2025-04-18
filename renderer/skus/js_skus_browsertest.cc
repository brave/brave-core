/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/renderer/skus_render_frame_observer.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/skus/common/features.h"
#include "content/public/test/render_view_test.h"
#include "url/gurl.h"

namespace skus {

class JsSkusBrowserTest : public content::RenderViewTest {
 public:
  JsSkusBrowserTest() {
    scoped_feature_list_.InitWithFeatures({skus::features::kSkusFeature}, {});
  }
  ~JsSkusBrowserTest() override = default;

  bool ExecuteJavascript(const std::u16string& script) {
    int result = -1;
    EXPECT_TRUE(ExecuteJavaScriptAndReturnIntValue(script, &result));
    return result == 1;
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(JsSkusBrowserTest, AttachSkus) {
  SkusRenderFrameObserver observer(GetMainRenderFrame());
  std::u16string command =
      u"Number((window.chrome !== undefined) && (window.chrome.braveSkus !== "
      u"undefined) && "
      u"(window.chrome.braveSkus.refresh_order !== undefined))";
  LoadHTMLWithUrlOverride(R"(<html><body> </body></html>)",
                          "https://account.some.other");
  EXPECT_FALSE(ExecuteJavascript(command));
  GURL url("https://account.brave.software");
  LoadHTMLWithUrlOverride(R"(<html><body> </body></html>)", url.spec().c_str());
  EXPECT_TRUE(ExecuteJavascript(command));
  // Reloading the url doesn't seem to work any more presumably because of the
  // change https://source.chromium.org/chromium/chromium/src/+/
  // ae845bfbaace3a356b66de078d6d70c84192c7f7, which causes an empty
  // security origin and skus::IsSafeOrigin then obviously returns false.
  // Reload(url);
  // EXPECT_TRUE(ExecuteJavascript(command));
  std::u16string overwrite =
      u"Number((window.chrome.braveSkus = ['test']) && "
      u"(window.chrome.braveSkus[0] === 'test'))";
  EXPECT_FALSE(ExecuteJavascript(overwrite));
  EXPECT_TRUE(ExecuteJavascript(command));
}

}  // namespace skus
