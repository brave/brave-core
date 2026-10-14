// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_input_registrar.h"

#include "base/functional/bind.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"

namespace gestures {

namespace {

bool NeverConsumeMouseEvent(const blink::WebMouseEvent& /*web_mouse_event*/) {
  return false;
}

}  // namespace

class GestureInputRegistrarBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory("content/test/data");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  static GestureInputRegistrar MakeInputRegistrar() {
    return {base::BindRepeating(&NeverConsumeMouseEvent),
            base::BindRepeating(
                [](const blink::WebInputEvent&, input::InputEventSource) {})};
  }

  content::WebContents* NavigateToCrossOriginFrameTree() {
    content::WebContents* const web_contents =
        chrome_test_utils::GetActiveWebContents(this);
    EXPECT_TRUE(chrome_test_utils::NavigateToURL(
        web_contents, embedded_test_server()->GetURL(
                          "a.com", "/cross_site_iframe_factory.html?a(b)")));
    return web_contents;
  }
};

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       SyncWithFrameTreeAttachesToTheMainFrameWidget) {
  content::WebContents* const web_contents =
      chrome_test_utils::GetActiveWebContents(this);
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      web_contents, embedded_test_server()->GetURL("a.com", "/title1.html")));
  content::RenderWidgetHost* const main_frame_widget =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_widget);
  GestureInputRegistrar registrar = MakeInputRegistrar();

  registrar.SyncWithFrameTree(web_contents);

  EXPECT_TRUE(registrar.IsObservingWidget(*main_frame_widget));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       SyncWithFrameTreeAttachesToCrossOriginIframesOwnWidget) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const main_frame_widget =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_widget);
  content::RenderWidgetHost* const child_frame_widget =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(child_frame_widget);
  GestureInputRegistrar registrar = MakeInputRegistrar();

  registrar.SyncWithFrameTree(web_contents);

  EXPECT_TRUE(registrar.IsObservingWidget(*main_frame_widget));
  EXPECT_TRUE(registrar.IsObservingWidget(*child_frame_widget));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       SyncWithFrameTreeIsNoOpWhenTheTreeIsUnchanged) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderWidgetHost* const main_frame_widget =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_widget);
  GestureInputRegistrar registrar = MakeInputRegistrar();
  registrar.SyncWithFrameTree(web_contents);
  ASSERT_TRUE(registrar.IsObservingWidget(*main_frame_widget));

  // Resyncing against the same, unchanged frame tree must not toggle
  // attachment: `SyncWithFrameTree`'s own class comment promises this is a
  // no-op for widgets whose attachment doesn't change.
  registrar.SyncWithFrameTree(web_contents);

  EXPECT_TRUE(registrar.IsObservingWidget(*main_frame_widget));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       SyncWithFrameTreeDetachesWidgetsNoLongerInTheTree) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const child_frame_widget =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(child_frame_widget);
  GestureInputRegistrar registrar = MakeInputRegistrar();
  registrar.SyncWithFrameTree(web_contents);
  ASSERT_TRUE(registrar.IsObservingWidget(*child_frame_widget));

  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      web_contents, embedded_test_server()->GetURL("a.com", "/title1.html")));
  registrar.SyncWithFrameTree(web_contents);

  EXPECT_FALSE(registrar.IsObservingWidget(*child_frame_widget));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       UnregisterWidgetDetachesOnlyThatWidget) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const main_frame_widget =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_widget);
  content::RenderWidgetHost* const child_frame_widget =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(child_frame_widget);
  GestureInputRegistrar registrar = MakeInputRegistrar();
  registrar.SyncWithFrameTree(web_contents);

  registrar.UnregisterWidget(*child_frame_widget);

  EXPECT_FALSE(registrar.IsObservingWidget(*child_frame_widget));
  EXPECT_TRUE(registrar.IsObservingWidget(*main_frame_widget));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       UnregisterAllDetachesEveryWidget) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const main_frame_widget =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_widget);
  content::RenderWidgetHost* const child_frame_widget =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(child_frame_widget);
  GestureInputRegistrar registrar = MakeInputRegistrar();
  registrar.SyncWithFrameTree(web_contents);

  registrar.UnregisterAll();

  EXPECT_FALSE(registrar.IsObservingWidget(*main_frame_widget));
  EXPECT_FALSE(registrar.IsObservingWidget(*child_frame_widget));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       SyncWithFrameTreeDetachesEverythingWhenPassedNull) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderWidgetHost* const main_frame_widget =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_widget);
  GestureInputRegistrar registrar = MakeInputRegistrar();
  registrar.SyncWithFrameTree(web_contents);

  registrar.SyncWithFrameTree(nullptr);

  EXPECT_FALSE(registrar.IsObservingWidget(*main_frame_widget));
}

}  // namespace gestures
