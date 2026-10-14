// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/input/gesture_input_registrar.h"

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
  content::WebContents* active_web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  static GestureInputRegistrar MakeInputRegistrar() {
    return {base::BindRepeating(&NeverConsumeMouseEvent),
            base::BindRepeating(
                [](const blink::WebInputEvent&, input::InputEventSource) {})};
  }

  content::WebContents* NavigateToCrossOriginFrameTree() {
    content::WebContents* const web_contents = active_web_contents();
    EXPECT_TRUE(chrome_test_utils::NavigateToURL(
        web_contents, embedded_test_server()->GetURL(
                          "a.com", "/cross_site_iframe_factory.html?a(b)")));
    return web_contents;
  }
};

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       ShouldObserveMainFrameWidget) {
  content::WebContents* const web_contents = active_web_contents();
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      web_contents, embedded_test_server()->GetURL("a.com", "/title1.html")));
  content::RenderWidgetHost* const render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(render_widget_host);
  GestureInputRegistrar input_registrar = MakeInputRegistrar();

  input_registrar.UpdateObservedWidgets(web_contents);

  EXPECT_TRUE(input_registrar.IsObservingWidget(*render_widget_host));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       ShouldObserveNewMainFrameWidgetAfterNavigation) {
  content::WebContents* const web_contents = active_web_contents();
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      web_contents, embedded_test_server()->GetURL("a.com", "/title1.html")));
  GestureInputRegistrar input_registrar = MakeInputRegistrar();
  input_registrar.UpdateObservedWidgets(web_contents);

  // A cross-site navigation swaps the main frame's `RenderWidgetHost`; the
  // registrar must pick up the new one once told to re-observe, not keep
  // pointing at whichever host used to be the main frame's.
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      web_contents, embedded_test_server()->GetURL("b.com", "/title1.html")));
  content::RenderWidgetHost* const new_render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(new_render_widget_host);

  input_registrar.UpdateObservedWidgets(web_contents);

  EXPECT_TRUE(input_registrar.IsObservingWidget(*new_render_widget_host));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       ShouldObserveWidgetOwnedByCrossOriginIframe) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const main_frame_render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_render_widget_host);
  content::RenderWidgetHost* const child_frame_render_widget_host =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(child_frame_render_widget_host);
  GestureInputRegistrar input_registrar = MakeInputRegistrar();

  input_registrar.UpdateObservedWidgets(web_contents);

  EXPECT_TRUE(
      input_registrar.IsObservingWidget(*main_frame_render_widget_host));
  EXPECT_TRUE(
      input_registrar.IsObservingWidget(*child_frame_render_widget_host));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       ShouldKeepWidgetObserversWhenFrameTreeIsUnchanged) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderWidgetHost* const render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(render_widget_host);
  GestureInputRegistrar input_registrar = MakeInputRegistrar();
  input_registrar.UpdateObservedWidgets(web_contents);
  ASSERT_TRUE(input_registrar.IsObservingWidget(*render_widget_host));

  // Resyncing against the same, unchanged frame tree must not toggle
  // attachment, a no-op for widgets whose attachment doesn't change.
  input_registrar.UpdateObservedWidgets(web_contents);

  EXPECT_TRUE(input_registrar.IsObservingWidget(*render_widget_host));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       ShouldStopObservingWidgetsRemovedFromFrameTree) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const render_widget_host =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(render_widget_host);
  GestureInputRegistrar input_registrar = MakeInputRegistrar();
  input_registrar.UpdateObservedWidgets(web_contents);
  ASSERT_TRUE(input_registrar.IsObservingWidget(*render_widget_host));

  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      web_contents, embedded_test_server()->GetURL("a.com", "/title1.html")));
  input_registrar.UpdateObservedWidgets(web_contents);

  EXPECT_FALSE(input_registrar.IsObservingWidget(*render_widget_host));
}

IN_PROC_BROWSER_TEST_F(
    GestureInputRegistrarBrowserTest,
    ShouldStopObservingUnregisteredWidgetWithoutAffectingOthers) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const main_frame_render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_render_widget_host);
  content::RenderWidgetHost* const child_frame_render_widget_host =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(child_frame_render_widget_host);
  GestureInputRegistrar input_registrar = MakeInputRegistrar();
  input_registrar.UpdateObservedWidgets(web_contents);

  input_registrar.UnregisterWidget(*child_frame_render_widget_host);

  EXPECT_FALSE(
      input_registrar.IsObservingWidget(*child_frame_render_widget_host));
  EXPECT_TRUE(
      input_registrar.IsObservingWidget(*main_frame_render_widget_host));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       ShouldStopObservingAllWidgetsAfterUnregistration) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderFrameHost* const child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  content::RenderWidgetHost* const main_frame_render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(main_frame_render_widget_host);
  content::RenderWidgetHost* const child_frame_render_widget_host =
      child_frame->GetRenderWidgetHost();
  ASSERT_TRUE(child_frame_render_widget_host);
  GestureInputRegistrar input_registrar = MakeInputRegistrar();
  input_registrar.UpdateObservedWidgets(web_contents);

  input_registrar.UnregisterAllWidgets();

  EXPECT_FALSE(
      input_registrar.IsObservingWidget(*main_frame_render_widget_host));
  EXPECT_FALSE(
      input_registrar.IsObservingWidget(*child_frame_render_widget_host));
}

IN_PROC_BROWSER_TEST_F(GestureInputRegistrarBrowserTest,
                       ShouldStopObservingAllWidgetsWhenNoPageIsProvided) {
  content::WebContents* const web_contents = NavigateToCrossOriginFrameTree();
  content::RenderWidgetHost* const render_widget_host =
      web_contents->GetPrimaryMainFrame()->GetRenderWidgetHost();
  ASSERT_TRUE(render_widget_host);
  GestureInputRegistrar input_registrar = MakeInputRegistrar();
  input_registrar.UpdateObservedWidgets(web_contents);

  input_registrar.UpdateObservedWidgets(/*web_contents=*/nullptr);

  EXPECT_FALSE(input_registrar.IsObservingWidget(*render_widget_host));
}

}  // namespace gestures
