/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/context_menu_interceptor.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/context_menu_data/untrustworthy_context_menu_params.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/mojom/context_menu/context_menu.mojom-shared.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_f.h"
#include "url/gurl.h"

namespace {

// Mirrors the shape of a real canvas app: the canvas is covered by a
// transparent overlay that takes the input, so the contextmenu target is the
// overlay and not the canvas. Tinkercad's is a `div.hud`. "opaque" covers a
// second canvas the way ordinary page content covers a decorative background
// canvas, and "plain" is off on its own with no canvas underneath.
constexpr char kTestPage[] = R"(data:text/html,
    <html><body style="margin:0">
      <div style="position:relative;width:100px;height:100px">
        <canvas width="100" height="100"
                style="position:absolute;inset:0"></canvas>
        <div id="overlay" style="position:absolute;inset:0"></div>
      </div>
      <div style="position:relative;width:100px;height:100px">
        <canvas width="100" height="100"
                style="position:absolute;inset:0"></canvas>
        <div id="opaque"
             style="position:absolute;inset:0;background:white"></div>
      </div>
      <div id="plain" style="width:100px;height:100px"></div>
      <script>
        for (const c of document.querySelectorAll('canvas')) {
          c.getContext('2d');
        }
        document.addEventListener('contextmenu', e => e.preventDefault());
      </script>
    </body></html>)";

// A bare canvas on a page that does not block the context menu.
constexpr char kUnblockedTestPage[] = R"(data:text/html,
    <html><body style="margin:0">
      <canvas id="canvas" width="200" height="200"></canvas>
      <script>
        document.getElementById('canvas').getContext('2d');
      </script>
    </body></html>)";

}  // namespace

class ForceContextMenuOnShiftRightClickBrowserTest
    : public InProcessBrowserTest {
 public:
  ForceContextMenuOnShiftRightClickBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        blink::features::kForceContextMenuOnShiftRightClick);
  }

 protected:
  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  gfx::Point ShiftRightClickOn(std::string_view id) {
    // GetCenterCoordinatesOfElementWithId() CHECK-fails on a missing element,
    // which reads as a browser crash rather than a test failure. A truncated
    // test page is the likely cause, so say so.
    if (!content::EvalJs(
             web_contents(),
             content::JsReplace("!!document.getElementById($1)", id))
             .ExtractBool()) {
      ADD_FAILURE() << "no #" << id << " on the page; is it truncated?";
      return gfx::Point();
    }

    const gfx::PointF center =
        content::GetCenterCoordinatesOfElementWithId(web_contents(), id);
    const gfx::Point point(center.x(), center.y());
    content::SimulateMouseClickAt(web_contents(),
                                  blink::WebInputEvent::kShiftKey,
                                  blink::WebMouseEvent::Button::kRight, point);
    return point;
  }

  // media_type can't identify which element a menu came from on a page with a
  // document-level contextmenu listener: that listener makes
  // ContextMenuController::GetContextMenuNodeWithImageContents() treat image
  // selection as blocked and report kNone even for a hit that penetrates to a
  // canvas. Where the menu was raised is the signal that still works.
  static void ExpectMenuRaisedAt(const gfx::Point& expected,
                                 content::ContextMenuInterceptor& interceptor) {
    const auto params = interceptor.get_params();
    EXPECT_EQ(expected, gfx::Point(params.x, params.y));
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Shift + Right Click still overrides preventDefault() away from a canvas.
IN_PROC_BROWSER_TEST_F(ForceContextMenuOnShiftRightClickBrowserTest,
                       OverridesPreventDefaultOffCanvas) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestPage)));

  content::ContextMenuInterceptor interceptor(
      web_contents()->GetPrimaryMainFrame(),
      content::ContextMenuInterceptor::ShowBehavior::kPreventShow);
  const gfx::Point plain = ShiftRightClickOn("plain");
  interceptor.Wait();

  ExpectMenuRaisedAt(plain, interceptor);
}

// A canvas under a transparent overlay is still a canvas, so preventDefault()
// is honored. See https://github.com/brave/brave-browser/issues/56333.
IN_PROC_BROWSER_TEST_F(ForceContextMenuOnShiftRightClickBrowserTest,
                       HonorsPreventDefaultOnCanvasUnderOverlay) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestPage)));

  content::ContextMenuInterceptor interceptor(
      web_contents()->GetPrimaryMainFrame(),
      content::ContextMenuInterceptor::ShowBehavior::kPreventShow);
  ShiftRightClickOn("overlay");

  // The overlay click must raise nothing, so the menu we wait for is the one
  // from #plain. If the override leaked through, the overlay's menu arrives
  // first and this sees its position instead.
  const gfx::Point plain = ShiftRightClickOn("plain");
  interceptor.Wait();

  ExpectMenuRaisedAt(plain, interceptor);
}

// Opaque content between the cursor and a canvas means the click didn't land
// on an app surface, so the override still applies.
IN_PROC_BROWSER_TEST_F(ForceContextMenuOnShiftRightClickBrowserTest,
                       OverridesPreventDefaultOnCanvasUnderOpaqueContent) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestPage)));

  content::ContextMenuInterceptor interceptor(
      web_contents()->GetPrimaryMainFrame(),
      content::ContextMenuInterceptor::ShowBehavior::kPreventShow);
  const gfx::Point opaque = ShiftRightClickOn("opaque");
  interceptor.Wait();

  ExpectMenuRaisedAt(opaque, interceptor);
}

// The carve-out only applies to the preventDefault() override: a canvas that
// doesn't block the context menu still gets one.
IN_PROC_BROWSER_TEST_F(ForceContextMenuOnShiftRightClickBrowserTest,
                       ShowsMenuOnCanvasWithoutPreventDefault) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL(kUnblockedTestPage)));

  content::ContextMenuInterceptor interceptor(
      web_contents()->GetPrimaryMainFrame(),
      content::ContextMenuInterceptor::ShowBehavior::kPreventShow);
  ShiftRightClickOn("canvas");
  interceptor.Wait();

  EXPECT_EQ(blink::mojom::ContextMenuDataMediaType::kCanvas,
            interceptor.get_params().media_type);
}
