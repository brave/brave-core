// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_controller.h"

#include "base/functional/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/text_param_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "brave/components/gestures/core/common/test/gesture_path_test_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
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
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "url/gurl.h"

namespace gestures {

namespace {
constexpr gfx::Point kDragStart(300, 300);
}  // namespace

class GestureControllerBrowserTest : public PlatformBrowserTest {
 public:
  GestureControllerBrowserTest() {
    // Pinned explicitly, rather than relying on the feature's own defaults,
    // since several tests below assert on stroke-commit/shape-match outcomes
    // computed by hand against these exact values.
    gestures_feature_list_.InitAndEnableFeatureWithParameters(
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
    return *tabs::BraveTabFeatures::FromTabFeatures(
                active_tab().GetTabFeatures())
                ->gesture_controller();
  }

  // `content::SimulateMouseEvent` always sends a 0 modifiers bitmask, but
  // `HandleMouseMoveEvent` reads the right-button-held bit from a real move
  // event to decide whether the draw is still active, so this forwards a
  // manually-built event carrying it instead.
  void SimulateMouseButtonEvent(blink::WebInputEvent::Type type,
                                blink::WebMouseEvent::Button button,
                                int modifiers,
                                const gfx::Point& point) {
    blink::WebMouseEvent mouse_event(type, modifiers, ui::EventTimeForNow());
    mouse_event.button = button;
    mouse_event.click_count = 1;
    mouse_event.SetPositionInWidget(base::saturated_cast<float>(point.x()),
                                    base::saturated_cast<float>(point.y()));
    active_web_contents()
        ->GetRenderWidgetHostView()
        ->GetRenderWidgetHost()
        ->ForwardMouseEvent(mouse_event);
  }

  void SimulateRightMouseButtonDown() {
    SimulateMouseButtonEvent(blink::WebInputEvent::Type::kMouseDown,
                             blink::WebMouseEvent::Button::kRight,
                             blink::WebInputEvent::kRightButtonDown,
                             kDragStart);
  }

  void SimulateRightMouseButtonUp(const gfx::Point& point) {
    SimulateMouseButtonEvent(blink::WebInputEvent::Type::kMouseUp,
                             blink::WebMouseEvent::Button::kRight,
                             /*modifiers=*/0, point);
  }

  // Drags the right mouse button `total_distance` px to the left of
  // `kDragStart`, 10px per move event, matching the swipe shape
  // `GestureDrawSession`'s own unit tests rely on to reliably commit a
  // `kLeft` stroke.
  void SimulateRightMouseButtonDownDragLeftAndRelease(int total_distance) {
    SimulateRightMouseButtonDown();
    for (int moved = 10; moved <= total_distance; moved += 10) {
      SimulateMouseButtonEvent(
          blink::WebInputEvent::Type::kMouseMove,
          blink::WebMouseEvent::Button::kRight,
          blink::WebInputEvent::kRightButtonDown,
          gfx::Point(kDragStart.x() - moved, kDragStart.y()));
    }
    SimulateRightMouseButtonUp(
        gfx::Point(kDragStart.x() - total_distance, kDragStart.y()));
  }

 private:
  base::test::ScopedFeatureList gestures_feature_list_;
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
  SimulateRightMouseButtonDown();

  EXPECT_TRUE(gesture_controller().IsDrawingGesture());

  // Releasing without ever matching a gesture must not leave drawing stuck.
  SimulateRightMouseButtonUp(kDragStart);
  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldSuppressContextMenuForDirectionalMatch) {
  SimulateRightMouseButtonDownDragLeftAndRelease(/*total_distance=*/100);

  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
  EXPECT_TRUE(gesture_controller().ConsumeSuppressNextContextMenu());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldNotSuppressContextMenuForClickWithoutMovement) {
  SimulateRightMouseButtonDown();
  SimulateRightMouseButtonUp(kDragStart);

  EXPECT_FALSE(gesture_controller().ConsumeSuppressNextContextMenu());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldShowDeferredContextMenuAfterClickWithoutMovement) {
  bool context_menu_shown = false;
  gesture_controller().DeferContextMenuShow(base::BindOnce(
      [](bool* context_menu_shown) { *context_menu_shown = true; },
      &context_menu_shown));

  SimulateRightMouseButtonDown();
  SimulateRightMouseButtonUp(kDragStart);

  EXPECT_TRUE(context_menu_shown);
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldCancelDrawingWhenPageVisibilityChanges) {
  SimulateRightMouseButtonDown();
  ASSERT_TRUE(gesture_controller().IsDrawingGesture());

  gesture_controller().OnVisibilityChanged(content::Visibility::HIDDEN);

  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldCancelDrawingWhenPageLosesFocus) {
  SimulateRightMouseButtonDown();
  ASSERT_TRUE(gesture_controller().IsDrawingGesture());

  gesture_controller().OnWebContentsLostFocus(/*render_widget_host=*/nullptr);

  EXPECT_FALSE(gesture_controller().IsDrawingGesture());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldDispatchActionForCustomMatch) {
  // A closed, non-directional shape, so only the custom-gesture match can
  // apply. The same path drives both the registered shape below and the
  // mouse moves that trace it.
  const GesturePath shape_path = test::MakeCircleGesturePath();

  base::ListValue points;
  for (const gfx::PointF& point : shape_path) {
    points.Append(base::DictValue()
                      .Set("x", static_cast<double>(point.x()))
                      .Set("y", static_cast<double>(point.y())));
  }
  base::ListValue custom_gestures;
  custom_gestures.Append(
      base::DictValue()
          .Set("id", "circle-gesture")
          .Set("points", std::move(points))
          .Set("actions", base::ListValue().Append(
                              base::DictValue().Set("command", IDC_NEW_TAB))));
  GetPrefs().SetList(prefs::kCustomGestures, std::move(custom_gestures));

  const int tab_count_before = browser()->tab_strip_model()->count();
  const gfx::Point shape_origin = gfx::ToRoundedPoint(shape_path.front());

  SimulateRightMouseButtonDown();
  // Starts from index 1: `shape_path[0]` is `kDragStart` itself, already the
  // position `SimulateRightMouseButtonDown` pressed at.
  for (size_t i = 1; i < shape_path.size(); ++i) {
    SimulateMouseButtonEvent(
        blink::WebInputEvent::Type::kMouseMove,
        blink::WebMouseEvent::Button::kRight,
        blink::WebInputEvent::kRightButtonDown,
        kDragStart + (gfx::ToRoundedPoint(shape_path[i]) - shape_origin));
  }
  SimulateRightMouseButtonUp(
      kDragStart + (gfx::ToRoundedPoint(shape_path.back()) - shape_origin));

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return browser()->tab_strip_model()->count() == tab_count_before + 1;
  }));
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldExecuteChainedActionsInConfiguredOrder) {
  const GURL target_url = embedded_test_server()->GetURL("/title2.html");
  // A circle mapped to [new tab, navigate to URL]; the navigation only
  // lands on the new tab, not the original one, if new tab genuinely ran
  // first rather than the actions firing out of order.
  const GesturePath shape_path = test::MakeCircleGesturePath();

  base::ListValue points;
  for (const gfx::PointF& point : shape_path) {
    points.Append(base::DictValue()
                      .Set("x", static_cast<double>(point.x()))
                      .Set("y", static_cast<double>(point.y())));
  }
  base::ListValue custom_gestures;
  custom_gestures.Append(
      base::DictValue()
          .Set("id", "circle-gesture")
          .Set("points", std::move(points))
          .Set("actions",
               base::ListValue()
                   .Append(base::DictValue().Set("command", IDC_NEW_TAB))
                   .Append(base::DictValue()
                               .Set("command", kNavigateToUrl)
                               .Set(kTextParamKey, target_url.spec()))));
  GetPrefs().SetList(prefs::kCustomGestures, std::move(custom_gestures));

  const int tab_count_before = browser()->tab_strip_model()->count();
  const gfx::Point shape_origin = gfx::ToRoundedPoint(shape_path.front());

  SimulateRightMouseButtonDown();
  for (size_t i = 1; i < shape_path.size(); ++i) {
    SimulateMouseButtonEvent(
        blink::WebInputEvent::Type::kMouseMove,
        blink::WebMouseEvent::Button::kRight,
        blink::WebInputEvent::kRightButtonDown,
        kDragStart + (gfx::ToRoundedPoint(shape_path[i]) - shape_origin));
  }
  SimulateRightMouseButtonUp(
      kDragStart + (gfx::ToRoundedPoint(shape_path.back()) - shape_origin));

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return browser()->tab_strip_model()->count() == tab_count_before + 1;
  }));
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return active_web_contents()->GetLastCommittedURL() == target_url;
  }));
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldNotDispatchActionForUnrecognizedGesture) {
  const GURL url_before_gesture = active_web_contents()->GetLastCommittedURL();
  const int tab_count_before = browser()->tab_strip_model()->count();

  // Movement too short to resolve as a directional stroke, and no custom
  // gesture is registered to match it either.
  SimulateRightMouseButtonDown();
  SimulateMouseButtonEvent(blink::WebInputEvent::Type::kMouseMove,
                           blink::WebMouseEvent::Button::kRight,
                           blink::WebInputEvent::kRightButtonDown,
                           gfx::Point(kDragStart.x() + 5, kDragStart.y()));
  SimulateRightMouseButtonUp(gfx::Point(kDragStart.x() + 5, kDragStart.y()));

  // Give any dispatch a message-loop turn to run before asserting it never
  // did.
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(tab_count_before, browser()->tab_strip_model()->count());
  EXPECT_EQ(url_before_gesture, active_web_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(GestureControllerBrowserTest,
                       ShouldNotDispatchActionAfterDrawingIsCancelled) {
  // History to go "back" through if the left-swipe below incorrectly
  // dispatches despite being cancelled.
  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      active_web_contents(), embedded_test_server()->GetURL("/title2.html")));
  const GURL url_before_swipe = active_web_contents()->GetLastCommittedURL();

  SimulateRightMouseButtonDown();
  for (int moved = 10; moved <= 100; moved += 10) {
    SimulateMouseButtonEvent(
        blink::WebInputEvent::Type::kMouseMove,
        blink::WebMouseEvent::Button::kRight,
        blink::WebInputEvent::kRightButtonDown,
        gfx::Point(kDragStart.x() - moved, kDragStart.y()));
  }
  gesture_controller().OnVisibilityChanged(content::Visibility::HIDDEN);
  ASSERT_FALSE(gesture_controller().IsDrawingGesture());
  SimulateRightMouseButtonUp(gfx::Point(kDragStart.x() - 100, kDragStart.y()));

  // `MaybeFinish` returns early when nothing is being drawn, but give any
  // dispatch a message-loop turn to run before asserting it never did.
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(url_before_swipe, active_web_contents()->GetLastCommittedURL());
}

}  // namespace gestures
