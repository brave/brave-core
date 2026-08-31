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
#include "third_party/blink/public/common/context_menu_data/untrustworthy_context_menu_params.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/mojom/context_menu/context_menu.mojom-shared.h"
#include "ui/gfx/geometry/point.h"

namespace {

// Mirrors the shape of a real canvas app: the canvas is covered by a
// transparent overlay that takes the input, so the contextmenu target is the
// overlay and not the canvas. Tinkercad's is a `div.hud`. "opaque" covers a
// second canvas the way ordinary page content covers a decorative background
// canvas, and "plain" is off on its own with no canvas underneath.
constexpr char kTestPage[] = R"(data:text/html,
    <html><body style="margin:0">
      <div style="position:relative;width:200px;height:200px">
        <canvas width="200" height="200"
                style="position:absolute;inset:0"></canvas>
        <div id="overlay" style="position:absolute;inset:0"></div>
      </div>
      <div style="position:relative;width:200px;height:200px">
        <canvas width="200" height="200"
                style="position:absolute;inset:0"></canvas>
        <div id="opaque"
             style="position:absolute;inset:0;background:#fff"></div>
      </div>
      <div id="plain" style="width:200px;height:200px"></div>
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

  void ShiftRightClickOn(std::string_view id) {
    const gfx::PointF center =
        content::GetCenterCoordinatesOfElementWithId(web_contents(), id);
    content::SimulateMouseClickAt(web_contents(),
                                  blink::WebInputEvent::kShiftKey,
                                  blink::WebMouseEvent::Button::kRight,
                                  gfx::Point(center.x(), center.y()));
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
  ShiftRightClickOn("plain");
  interceptor.Wait();

  EXPECT_EQ(blink::mojom::ContextMenuDataMediaType::kNone,
            interceptor.get_params().media_type);
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

  // A menu for the overlay would arrive before this one, and would carry
  // kCanvas because ContextMenuController penetrates to the canvas too. So the
  // media type of the first menu we see says whether the click was honored.
  ShiftRightClickOn("plain");
  interceptor.Wait();

  EXPECT_EQ(blink::mojom::ContextMenuDataMediaType::kNone,
            interceptor.get_params().media_type);
}

// Opaque content between the cursor and a canvas means the click didn't land
// on an app surface, so the override still applies.
IN_PROC_BROWSER_TEST_F(ForceContextMenuOnShiftRightClickBrowserTest,
                       OverridesPreventDefaultOnCanvasUnderOpaqueContent) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kTestPage)));

  content::ContextMenuInterceptor interceptor(
      web_contents()->GetPrimaryMainFrame(),
      content::ContextMenuInterceptor::ShowBehavior::kPreventShow);
  ShiftRightClickOn("opaque");
  interceptor.Wait();

  EXPECT_EQ(blink::mojom::ContextMenuDataMediaType::kNone,
            interceptor.get_params().media_type);
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
