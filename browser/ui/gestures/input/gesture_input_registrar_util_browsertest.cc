// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/input/gesture_input_registrar_util.h"

#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace gestures {

class GestureInputRegistrarUtilBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory("content/test/data");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  content::WebContents* active_web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
};

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarUtilBrowserTest,
                       ShouldReturnNoWidgetsWhenNoPageIsProvided) {
  EXPECT_THAT(CollectRenderWidgetHosts(/*web_contents=*/nullptr),
              testing::IsEmpty());
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarUtilBrowserTest,
                       ShouldReturnOnlyMainFrameWidgetForSimplePage) {
  content::WebContents* const web_contents = active_web_contents();
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      web_contents, embedded_test_server()->GetURL("a.com", "/title1.html")));
  content::RenderWidgetHost* const render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(render_widget_host);

  EXPECT_THAT(CollectRenderWidgetHosts(web_contents),
              testing::ElementsAre(render_widget_host));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarUtilBrowserTest,
                       ShouldIncludeWidgetOwnedByCrossOriginIframe) {
  content::WebContents* const web_contents = active_web_contents();
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      web_contents, embedded_test_server()->GetURL(
                        "a.com", "/cross_site_iframe_factory.html?a(b)")));
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const main_frame_render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_render_widget_host);
  content::RenderWidgetHost* const child_frame_render_widget_host =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(child_frame_render_widget_host);
  ASSERT_NE(main_frame_render_widget_host, child_frame_render_widget_host);

  EXPECT_THAT(CollectRenderWidgetHosts(web_contents),
              testing::UnorderedElementsAre(main_frame_render_widget_host,
                                            child_frame_render_widget_host));
}

}  // namespace gestures
