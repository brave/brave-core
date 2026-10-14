// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_controller.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/browser/ui/gestures/touch_centroid_util.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "third_party/blink/public/common/input/web_touch_event.h"

namespace gestures {

GestureController::GestureController(tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab),
      config_tracker_(
          *user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext())),
      draw_session_(
          base::BindRepeating(&GestureController::web_contents,
                              base::Unretained(this)),
          GetPrefs(),
          config_tracker_,
          base::BindRepeating(&GestureController::SuppressNextContextMenu,
                              base::Unretained(this))),
      action_dispatcher_(tab),
      mouse_rocker_recognizer_(
          config_tracker_,
          action_dispatcher_,
          base::BindRepeating(&GestureController::IsAllowedForSite,
                              base::Unretained(this)),
          base::BindRepeating(
              &GestureController::CancelDrawingAndSuppressContextMenu,
              base::Unretained(this))),
      mouse_wheel_recognizer_(
          config_tracker_,
          action_dispatcher_,
          base::BindRepeating(
              &GestureController::CancelDrawingAndSuppressContextMenu,
              base::Unretained(this))),
      input_registrar_(base::BindRepeating(&GestureController::HandleMouseEvent,
                                           base::Unretained(this)),
                       base::BindRepeating(&GestureController::OnInputEvent,
                                           base::Unretained(this))) {
  input_registrar_.SyncWithFrameTree(web_contents());
}

GestureController::~GestureController() {
  // `WebContentsObserver`'s `RenderViewHostChanged`/`RenderViewDeleted`
  // usually unregister this first, but destruction order isn't guaranteed
  // to run them before this destructor during tab teardown; leaving this
  // registered on a `RenderWidgetHost` that outlives it would crash the
  // next time it notifies observers of an input event.
  input_registrar_.UnregisterAll();
}

bool GestureController::ConsumeSuppressNextContextMenu() {
  const bool suppress = should_suppress_next_context_menu_;
  should_suppress_next_context_menu_ = false;
  return suppress;
}

bool GestureController::IsDrawingGesture() const {
  return draw_session_.is_drawing();
}

void GestureController::DeferContextMenuShow(
    base::OnceClosure pending_context_menu_show) {
  pending_context_menu_show_ = std::move(pending_context_menu_show);
}

void GestureController::RenderViewHostChanged(
    content::RenderViewHost* /*old_host*/,
    content::RenderViewHost* /*new_host*/) {
  // A cross-process navigation can swap the observed view mid-draw, leaving
  // the trail/overlay stale. A pref-driven threshold tied to the old
  // widget's scroll stream shouldn't carry over to the new one either.
  draw_session_.Cancel();
  mouse_wheel_recognizer_.Reset();
  input_registrar_.SyncWithFrameTree(web_contents());
}

void GestureController::RenderFrameHostChanged(
    content::RenderFrameHost* /*old_host*/,
    content::RenderFrameHost* new_host) {
  if (new_host && new_host->IsInPrimaryMainFrame()) {
    // A same-process navigation (e.g. a reload) can still replace the main
    // frame's `RenderFrameHost`, and therefore its `RenderWidgetHost`, under
    // RenderDocument, without `RenderViewHostChanged` firing at all.
    draw_session_.Cancel();
    mouse_wheel_recognizer_.Reset();
  }
  // Also fires for subframes, e.g. a sponsored rich-media background's own
  // out-of-process iframe gaining or losing its own widget.
  input_registrar_.SyncWithFrameTree(web_contents());
}

void GestureController::RenderFrameCreated(
    content::RenderFrameHost* /*render_frame_host*/) {
  input_registrar_.SyncWithFrameTree(web_contents());
}

void GestureController::RenderFrameDeleted(
    content::RenderFrameHost* /*render_frame_host*/) {
  input_registrar_.SyncWithFrameTree(web_contents());
}

void GestureController::RenderViewDeleted(
    content::RenderViewHost* render_view_host) {
  // `render_view_host`'s widget is still valid here, unlike by the time
  // our own destructor runs. Only unregister if it's actually one of the
  // widgets we're attached to; this `RenderViewHost` may belong to an
  // unrelated frame.
  if (render_view_host &&
      input_registrar_.IsObservingWidget(*render_view_host->GetWidget())) {
    input_registrar_.UnregisterWidget(*render_view_host->GetWidget());
  }
}

void GestureController::OnVisibilityChanged(content::Visibility visibility) {
  if (visibility == content::Visibility::VISIBLE) {
    return;
  }
  draw_session_.Cancel();
}

void GestureController::OnWebContentsLostFocus(
    content::RenderWidgetHost* /*render_widget_host*/) {
  // The tab can stay visible but lose input focus (e.g. alt-tabbing to
  // another app) mid-draw, leaving the trail/overlay stale.
  draw_session_.Cancel();
}

void GestureController::OnDiscardContents(tabs::TabInterface* tab,
                                          content::WebContents* old_contents,
                                          content::WebContents* new_contents) {
  // The live overlay widget, if any, caches `old_contents` for its own
  // lifetime; it must be torn down before `new_contents` becomes what
  // `draw_session_` fetches it from, not just stopped as a plain cancel.
  draw_session_.DiscardOverlay();
  mouse_wheel_recognizer_.Reset();
  input_registrar_.UnregisterAll();
  tabs::ContentsObservingTabFeature::OnDiscardContents(tab, old_contents,
                                                       new_contents);
  input_registrar_.SyncWithFrameTree(new_contents);
}

void GestureController::OnInputEvent(
    const blink::WebInputEvent& web_input_event,
    input::InputEventSource /*source*/) {
  if (web_input_event.GetType() == blink::WebInputEvent::Type::kMouseWheel) {
    HandleMouseWheelEvent(
        static_cast<const blink::WebMouseWheelEvent&>(web_input_event));
    return;
  }

  if (!blink::WebInputEvent::IsTouchEventType(web_input_event.GetType())) {
    return;
  }

  const auto& web_touch_event =
      static_cast<const blink::WebTouchEvent&>(web_input_event);
  switch (web_touch_event.GetType()) {
    case blink::WebInputEvent::Type::kTouchStart: {
      if (web_touch_event.touches_length == 2 && !draw_session_.is_drawing()) {
        StartDrawing(TouchCentroid(web_touch_event), /*via_touch=*/true,
                     /*via_alt_left_button=*/false);
      }
      break;
    }

    case blink::WebInputEvent::Type::kTouchMove: {
      if (draw_session_.is_drawing_via_touch()) {
        draw_session_.Continue(TouchCentroid(web_touch_event));
      }
      break;
    }

    case blink::WebInputEvent::Type::kTouchEnd:
    case blink::WebInputEvent::Type::kTouchCancel: {
      if (draw_session_.is_drawing_via_touch()) {
        FinishDrawing();
      }
      break;
    }

    default: {
      break;
    }
  }
}

bool GestureController::HandleMouseEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  // Right-button events are never consumed. Gestures only observe raw input
  // and suppress the resulting context menu separately. Left-button events
  // are the one exception, consumed only while a gesture is actively
  // drawing via Alt+left-drag, unlike the right button, since left-button
  // drag normally triggers native text/link selection, and Alt+left is
  // repurposed entirely as a gesture trigger rather than a real drag.
  switch (web_mouse_event.GetType()) {
    case blink::WebInputEvent::Type::kMouseDown: {
      return HandleMouseDownEvent(web_mouse_event);
    }

    case blink::WebInputEvent::Type::kMouseMove: {
      return HandleMouseMoveEvent(web_mouse_event);
    }

    case blink::WebInputEvent::Type::kMouseUp: {
      return HandleMouseUpEvent(web_mouse_event);
    }

    default: {
      return false;
    }
  }
}

PrefService& GestureController::GetPrefs() const {
  return *user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext());
}

bool GestureController::IsAllowedForSite() const {
  return config_tracker_.IsAllowedForHost(
      web_contents()->GetLastCommittedURL().host());
}

bool GestureController::HandleMouseDownEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  if (mouse_rocker_recognizer_.HandleMouseDown(web_mouse_event)) {
    return false;
  }

  const int modifiers = web_mouse_event.GetModifiers();
  if (web_mouse_event.button == blink::WebMouseEvent::Button::kRight) {
    if (!draw_session_.is_drawing() &&
        (modifiers & blink::WebInputEvent::kShiftKey) == 0) {
      // Outcome (plain click vs. an actual gesture) isn't known yet; the
      // context menu show is deferred by `IsDrawingGesture` returning true
      // until `FinishDrawing` resolves it.
      StartDrawing(web_mouse_event.PositionInWidget(), /*via_touch=*/false,
                   /*via_alt_left_button=*/false);
    }
    return false;
  }

  if (web_mouse_event.button == blink::WebMouseEvent::Button::kLeft) {
    if (!draw_session_.is_drawing() &&
        (modifiers & blink::WebInputEvent::kAltKey) != 0) {
      StartDrawing(web_mouse_event.PositionInWidget(), /*via_touch=*/false,
                   /*via_alt_left_button=*/true);
    }
    return draw_session_.is_drawing_via_alt_left_button();
  }

  return false;
}

bool GestureController::HandleMouseMoveEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  if (!draw_session_.is_drawing() || draw_session_.is_drawing_via_touch()) {
    return draw_session_.is_drawing_via_alt_left_button();
  }
  // The button that started this draw can be released while the cursor is
  // outside the window entirely, which can lose mouse capture and never
  // deliver a mouse-up event here. Checked on the event's own modifiers
  // rather than assuming the button is still held just because a mouse-up
  // was never seen, so returning to the window doesn't appear to resume a
  // draw whose button was actually let go while outside.
  const int modifiers = web_mouse_event.GetModifiers();
  const bool button_still_held =
      draw_session_.is_drawing_via_alt_left_button()
          ? (modifiers & blink::WebInputEvent::kLeftButtonDown) != 0
          : (modifiers & blink::WebInputEvent::kRightButtonDown) != 0;
  if (!button_still_held) {
    FinishDrawing();
    return false;
  }
  draw_session_.Continue(web_mouse_event.PositionInWidget());
  return draw_session_.is_drawing_via_alt_left_button();
}

bool GestureController::HandleMouseUpEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  if (web_mouse_event.button == blink::WebMouseEvent::Button::kRight) {
    if (!draw_session_.is_drawing_via_touch()) {
      FinishDrawing();
    }
    return false;
  }
  if (web_mouse_event.button == blink::WebMouseEvent::Button::kLeft) {
    const bool consume = draw_session_.is_drawing_via_alt_left_button();
    if (!draw_session_.is_drawing_via_touch()) {
      FinishDrawing();
    }
    return consume;
  }
  return false;
}

void GestureController::HandleMouseWheelEvent(
    const blink::WebMouseWheelEvent& web_mouse_wheel_event) {
  if (!config_tracker_.IsMouseWheelGesturesEnabled() || !IsAllowedForSite()) {
    return;
  }
  mouse_wheel_recognizer_.HandleEvent(web_mouse_wheel_event);
}

void GestureController::CancelDrawingAndSuppressContextMenu() {
  draw_session_.Cancel();
  should_suppress_next_context_menu_ = true;
}

void GestureController::SuppressNextContextMenu() {
  should_suppress_next_context_menu_ = true;
}

void GestureController::StartDrawing(gfx::PointF location,
                                     bool via_touch,
                                     bool via_alt_left_button) {
  if (!IsAllowedForSite()) {
    return;
  }
  draw_session_.Start(location, via_touch, via_alt_left_button);
}

void GestureController::FinishDrawing() {
  // Only a right-button draw can have a deferred context menu show to
  // resolve; recorded before calling `draw_session_.MaybeFinish()`, since
  // that call resets this.
  const bool was_right_button_draw =
      !draw_session_.is_drawing_via_touch() &&
      !draw_session_.is_drawing_via_alt_left_button();

  std::optional<std::vector<GestureActionInfo>> actions =
      draw_session_.MaybeFinish();
  if (!actions || actions->empty()) {
    // No match, or the draw was already cancelled before this ran (e.g. an
    // idle-hold timeout, focus loss or visibility change). A plain click (no
    // real movement) runs any deferred context menu show; a drawn-but
    // unmatched-or-cancelled stroke still suppresses it, same as a
    // successful match.
    const bool had_movement = should_suppress_next_context_menu_;
    should_suppress_next_context_menu_ = false;
    if (had_movement) {
      pending_context_menu_show_.Reset();
    } else if (pending_context_menu_show_) {
      std::move(pending_context_menu_show_).Run();
    }
    return;
  }

  if (was_right_button_draw) {
    should_suppress_next_context_menu_ = true;
  }
  pending_context_menu_show_.Reset();
  action_dispatcher_.DispatchActionsAsync(std::move(*actions));
}

}  // namespace gestures
