// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_controller.h"

#include <optional>

#include "base/functional/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/ui/gestures/session/test/web_mouse_event_test_util.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/text_param_parser.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "brave/components/gestures/core/common/test/gesture_path_test_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "url/gurl.h"

namespace gestures {

class GestureControllerBrowserTest : public PlatformBrowserTest {
 public:
  GestureControllerBrowserTest() {
    // Pinned, not relying on defaults, since tests below assert on
    // stroke-commit/shape-match outcomes computed by hand from these values.
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature,
        {{"DirectionalGestureMinStrokePixels", "30"},
         {"CustomGestureResamplePointCount", "32"},
         {"CustomGestureMaxMatchDistance", "0.14"}});
  }

 protected:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(chrome_test_utils::NavigateToURL(
        active_web_contents(), embedded_test_server()->GetURL("/title1.html")));
  }

  content::WebContents* active_web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  PrefService& GetPrefs() {
    return *chrome_test_utils::GetProfile(this)->GetPrefs();
  }

  tabs::TabInterface& active_tab() {
    return *chrome_test_utils::GetActiveTab(this);
  }

  GestureController& gesture_controller() {
    return *GestureController::From(&active_tab());
  }

  // `content::SimulateMouseEvent` always sends a 0 modifiers bitmask, but
  // `HandleMouseMoveEvent` needs the held-button bits carried here.
  void SimulateMouseEvent(blink::WebInputEvent::Type type,
                          blink::WebMouseEvent::Button button,
                          int modifiers,
                          const gfx::Point& point) {
    blink::WebMouseEvent mouse_event =
        test::MakeWebMouseButtonEvent(type, button, modifiers);
    mouse_event.SetPositionInWidget(base::saturated_cast<float>(point.x()),
                                    base::saturated_cast<float>(point.y()));
    active_web_contents()
        ->GetRenderWidgetHostView()
        ->GetRenderWidgetHost()
        ->ForwardMouseEvent(mouse_event);
  }

  static int MouseButtonModifierFor(blink::WebMouseEvent::Button button) {
    switch (button) {
      case blink::WebMouseEvent::Button::kLeft:
        return blink::WebInputEvent::kLeftButtonDown;
      case blink::WebMouseEvent::Button::kMiddle:
        return blink::WebInputEvent::kMiddleButtonDown;
      case blink::WebMouseEvent::Button::kRight:
        return blink::WebInputEvent::kRightButtonDown;
      default:
        NOTREACHED();
    }
  }

  // Presses at `last_mouse_position_` if another button is already held
  // (the mouse can't have moved), else (300, 300), safely inside the page.
  void SimulateMouseButtonDown(blink::WebMouseEvent::Button button) {
    const gfx::Point point =
        last_mouse_position_.value_or(gfx::Point(300, 300));
    held_mouse_button_modifiers_ |= MouseButtonModifierFor(button);
    last_mouse_position_ = point;
    SimulateMouseEvent(blink::WebInputEvent::Type::kMouseDown, button,
                       held_mouse_button_modifiers_, point);
  }

  void SimulateMouseMove(const gfx::Point& point) {
    last_mouse_position_ = point;
    SimulateMouseEvent(blink::WebInputEvent::Type::kMouseMove,
                       blink::WebMouseEvent::Button::kNoButton,
                       held_mouse_button_modifiers_, point);
  }

  void SimulateMouseButtonUp(blink::WebMouseEvent::Button button) {
    held_mouse_button_modifiers_ &= ~MouseButtonModifierFor(button);
    SimulateMouseEvent(blink::WebInputEvent::Type::kMouseUp, button,
                       held_mouse_button_modifiers_, *last_mouse_position_);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  int held_mouse_button_modifiers_ = 0;
  std::optional<gfx::Point> last_mouse_position_;
};

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldNotBeDrawingBeforeAnyInput) {
  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldNotSuppressContextMenuBeforeAnyInput) {
  EXPECT_FALSE(gesture_controller().ConsumeSuppressNextContextMenu());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldStartDrawingWhenRightMouseButtonIsPressed) {
  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);

  EXPECT_TRUE(gesture_controller().IsDrawingGesture());

  // Releasing without ever matching a gesture must not leave drawing stuck.
  SimulateMouseButtonUp(blink::WebMouseEvent::Button::kRight);
  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldConsumeMouseDownEventWhenRockerGestureFires) {
  ASSERT_TRUE(content::ExecJs(active_web_contents(), R"(
      window.sawRightMouseDown = false;
      document.addEventListener('mousedown', (event) => {
        if (event.button === 2) {
          window.sawRightMouseDown = true;
        }
      });
  )"));

  // Right button pressed while the left is already held fires a rocker
  // gesture, so it must be consumed rather than also delivered to the page.
  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kLeft);
  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);

  EXPECT_EQ(false,
            content::EvalJs(active_web_contents(), "window.sawRightMouseDown"));
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldSuppressContextMenuForDirectionalMatch) {
  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  // Drags 100px left of the start point, 10px per move, matching the shape
  // `GestureDrawSession`'s own tests commit as `kLeft`.
  for (int moved = 10; moved <= 100; moved += 10) {
    SimulateMouseMove(gfx::Point(300 - moved, 300));
  }
  SimulateMouseButtonUp(blink::WebMouseEvent::Button::kRight);

  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
  EXPECT_TRUE(gesture_controller().ConsumeSuppressNextContextMenu());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldNotSuppressContextMenuForClickWithoutMovement) {
  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  SimulateMouseButtonUp(blink::WebMouseEvent::Button::kRight);

  EXPECT_FALSE(gesture_controller().ConsumeSuppressNextContextMenu());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldShowDeferredContextMenuAfterClickWithoutMovement) {
  base::test::TestFuture<void> context_menu_shown;
  gesture_controller().DeferContextMenuShow(context_menu_shown.GetCallback());

  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  SimulateMouseButtonUp(blink::WebMouseEvent::Button::kRight);

  // `Finish` can defer resolving until the in-flight smart-gesture-content
  // mojo reply from `Start` arrives, so this can't be asserted synchronously.
  ASSERT_TRUE(context_menu_shown.Wait());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldCancelDrawingWhenPageVisibilityChanges) {
  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  ASSERT_TRUE(gesture_controller().IsDrawingGesture());

  gesture_controller().OnVisibilityChanged(content::Visibility::HIDDEN);

  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldCancelDrawingWhenPageLosesFocus) {
  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  ASSERT_TRUE(gesture_controller().IsDrawingGesture());

  gesture_controller().OnWebContentsLostFocus(/*render_widget_host=*/nullptr);

  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldDispatchActionForCustomMatch) {
  // A closed, non-directional shape, so only a custom-gesture match can
  // apply.
  const GesturePath gesture_path = test::MakeCircleGesturePath();

  base::ListValue points;
  for (const gfx::PointF& point : gesture_path) {
    points.Append(base::DictValue().Set("x", point.x()).Set("y", point.y()));
  }
  GetPrefs().SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "circle-gesture")
              .Set("points", std::move(points))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_NEW_TAB)))));

  const int tab_count_before = browser()->tab_strip_model()->count();
  const gfx::Point shape_origin = gfx::ToRoundedPoint(gesture_path.front());

  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  // Starts from index 1 since `gesture_path[0]` is the (300, 300) start
  // point itself, already where the drag started.
  for (size_t i = 1; i < gesture_path.size(); ++i) {
    SimulateMouseMove(gfx::Point(300, 300) +
                      (gfx::ToRoundedPoint(gesture_path[i]) - shape_origin));
  }
  SimulateMouseButtonUp(blink::WebMouseEvent::Button::kRight);

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return browser()->tab_strip_model()->count() == tab_count_before + 1;
  }));
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldExecuteChainedActionsInConfiguredOrder) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  // A circle mapped to [new tab, navigate to URL]; the navigation lands on
  // the new tab only if new tab actually ran first, not out of order.
  const GesturePath gesture_path = test::MakeCircleGesturePath();

  base::ListValue points;
  for (const gfx::PointF& point : gesture_path) {
    points.Append(base::DictValue().Set("x", point.x()).Set("y", point.y()));
  }
  GetPrefs().SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "circle-gesture")
              .Set("points", std::move(points))
              .Set("actions",
                   base::ListValue()
                       .Append(base::DictValue().Set("command", IDC_NEW_TAB))
                       .Append(base::DictValue()
                                   .Set("command", kNavigateToUrl)
                                   .Set(kTextParamKey, target_url.spec())))));

  const int tab_count_before = browser()->tab_strip_model()->count();
  const gfx::Point shape_origin = gfx::ToRoundedPoint(gesture_path.front());

  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  for (size_t i = 1; i < gesture_path.size(); ++i) {
    SimulateMouseMove(gfx::Point(300, 300) +
                      (gfx::ToRoundedPoint(gesture_path[i]) - shape_origin));
  }
  SimulateMouseButtonUp(blink::WebMouseEvent::Button::kRight);

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return browser()->tab_strip_model()->count() == tab_count_before + 1;
  }));
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return active_web_contents()->GetLastCommittedURL() == target_url;
  }));
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldNotDispatchActionForUnrecognizedGesture) {
  const GURL last_committed_url_before_gesture =
      active_web_contents()->GetLastCommittedURL();
  const int tab_count_before = browser()->tab_strip_model()->count();

  // Movement too short to resolve as a directional stroke, and no custom
  // gesture is registered to match it either.
  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  SimulateMouseMove(/*point=*/gfx::Point(305, 300));
  SimulateMouseButtonUp(blink::WebMouseEvent::Button::kRight);

  // Give any dispatch a message-loop turn to run before asserting it never
  // did.
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(tab_count_before, browser()->tab_strip_model()->count());
  EXPECT_EQ(last_committed_url_before_gesture,
            active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldNotDispatchActionAfterDrawingIsCancelled) {
  // History to go "back" through if the left-swipe below incorrectly
  // dispatches despite being cancelled.
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(), embedded_test_server()->GetURL("/title2.html")));
  const GURL last_committed_url_before_swipe =
      active_web_contents()->GetLastCommittedURL();

  SimulateMouseButtonDown(blink::WebMouseEvent::Button::kRight);
  for (int moved = 10; moved <= 100; moved += 10) {
    SimulateMouseMove(gfx::Point(300 - moved, 300));
  }
  gesture_controller().OnVisibilityChanged(content::Visibility::HIDDEN);
  ASSERT_FALSE(gesture_controller().IsDrawingGesture());
  SimulateMouseButtonUp(blink::WebMouseEvent::Button::kRight);

  // `Finish` returns early when nothing is being drawn, but give any
  // dispatch a message-loop turn to run before asserting it never did.
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(last_committed_url_before_swipe,
            active_web_contents()->GetLastCommittedURL());
}

}  // namespace gestures
