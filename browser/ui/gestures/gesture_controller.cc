// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_controller.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/gestures/core/browser/input/touch/touch_centroid_util.h"
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
#include "url/gurl.h"

namespace gestures {

DEFINE_USER_DATA(GestureController);

GestureController::GestureController(tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab),
      config_tracker_(
          *user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext())),
      mouse_modifier_config_tracker_(
          *user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext())),
      draw_session_(
          base::BindRepeating(&GestureController::web_contents,
                              base::Unretained(this)),
          GetPrefs(),
          config_tracker_.directional_gesture_mapping(),
          config_tracker_.custom_gesture_recognizer(),
          config_tracker_.smart_gesture_mapping(),
          base::BindRepeating(&GestureController::SuppressNextContextMenu,
                              base::Unretained(this))),
      action_dispatcher_(tab),
      mouse_rocker_recognizer_(
          mouse_modifier_config_tracker_,
          action_dispatcher_,
          base::BindRepeating(&GestureController::IsAllowedForSite,
                              base::Unretained(this)),
          base::BindRepeating(
              &GestureController::CancelDrawingAndSuppressContextMenu,
              base::Unretained(this))),
      mouse_wheel_recognizer_(
          mouse_modifier_config_tracker_,
          action_dispatcher_,
          base::BindRepeating(
              &GestureController::CancelDrawingAndSuppressContextMenu,
              base::Unretained(this))),
      input_registrar_(
          base::BindRepeating(&GestureController::HandleMouseEvent,
                              base::Unretained(this)),
          base::BindRepeating(&GestureController::InputEventCallback,
                              base::Unretained(this))),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {
  input_registrar_.UpdateObservedWidgets(web_contents());
}

GestureController::~GestureController() = default;

// static
GestureController* GestureController::From(tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
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
    base::OnceClosure pending_context_menu_show_callback) {
  pending_context_menu_show_callback_ =
      std::move(pending_context_menu_show_callback);
}

void GestureController::RenderViewHostChanged(
    content::RenderViewHost* /*old_host*/,
    content::RenderViewHost* /*new_host*/) {
  // A cross-process navigation can swap the view mid-draw, leaving the
  // trail/overlay and the old widget's wheel threshold stale.
  draw_session_.Cancel();
  mouse_wheel_recognizer_.Reset();
  input_registrar_.UpdateObservedWidgets(web_contents());
}

void GestureController::RenderFrameHostChanged(
    content::RenderFrameHost* /*old_host*/,
    content::RenderFrameHost* new_host) {
  if (new_host && new_host->IsInPrimaryMainFrame()) {
    // A same-process reload can replace the main frame's widget under
    // RenderDocument without `RenderViewHostChanged` ever firing.
    draw_session_.Cancel();
    mouse_wheel_recognizer_.Reset();
  }
  // Also fires for subframes, e.g. a sponsored new tab takeover own
  // out-of-process iframe gaining or losing its own widget.
  input_registrar_.UpdateObservedWidgets(web_contents());
}

void GestureController::RenderFrameCreated(
    content::RenderFrameHost* /*render_frame_host*/) {
  input_registrar_.UpdateObservedWidgets(web_contents());
}

void GestureController::RenderFrameDeleted(
    content::RenderFrameHost* /*render_frame_host*/) {
  input_registrar_.UpdateObservedWidgets(web_contents());
}

void GestureController::RenderViewDeleted(
    content::RenderViewHost* render_view_host) {
  // The widget is still valid here, unlike in our own destructor; only
  // unregister if it's actually one we're attached to.
  content::RenderWidgetHost* const widget =
      render_view_host ? render_view_host->GetWidget() : nullptr;
  if (!widget || !input_registrar_.IsObservingWidget(*widget)) {
    return;
  }
  input_registrar_.UnregisterWidget(*widget);
}

void GestureController::OnVisibilityChanged(content::Visibility visibility) {
  if (visibility != content::Visibility::VISIBLE) {
    draw_session_.Cancel();
  }
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
  // Torn down, not just cancelled, since the overlay widget caches
  // `old_contents` for its own lifetime.
  draw_session_.DiscardOverlayWidget();
  mouse_wheel_recognizer_.Reset();
  input_registrar_.UnregisterAllWidgets();
  tabs::ContentsObservingTabFeature::OnDiscardContents(tab, old_contents,
                                                       new_contents);
  input_registrar_.UpdateObservedWidgets(new_contents);
}

void GestureController::InputEventCallback(
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
        StartDrawing(TouchCentroid(web_touch_event),
                     DrawInputSourceType::kTouchDrag);
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
  // Neither button's mousedown/mouseup is consumed by a draw; only the
  // moves in between are, so a click's down/up still reach Blink.
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
  const GURL& url = web_contents()->GetLastCommittedURL();
  return config_tracker_.IsAllowedForHost(url.scheme(), url.host());
}

bool GestureController::HandleMouseDownEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  if (mouse_rocker_recognizer_.HandleMouseDownEvent(web_mouse_event)) {
    return true;
  }

  if (web_mouse_event.button == blink::WebMouseEvent::Button::kRight) {
    return HandleRightButtonDownEvent(web_mouse_event);
  }
  if (web_mouse_event.button == blink::WebMouseEvent::Button::kLeft) {
    return HandleAltLeftButtonDownEvent(web_mouse_event);
  }
  return false;
}

bool GestureController::HandleRightButtonDownEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  // An earlier rocker/wheel gesture may have set this without it ever being
  // consumed on button-down menu platforms; it can't belong to this press.
  should_suppress_next_context_menu_ = false;
  const int modifiers = web_mouse_event.GetModifiers();
  if (!draw_session_.is_drawing() &&
      (modifiers & blink::WebInputEvent::kShiftKey) == 0) {
    // Outcome unknown yet; `IsDrawingGesture` defers the menu show until
    // `FinishDrawing` resolves it.
    StartDrawing(web_mouse_event.PositionInWidget(),
                 DrawInputSourceType::kRightButtonDrag);
  }
  return false;
}

bool GestureController::HandleAltLeftButtonDownEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  const int modifiers = web_mouse_event.GetModifiers();
  if (!draw_session_.is_drawing() &&
      (modifiers & blink::WebInputEvent::kAltKey) != 0) {
    StartDrawing(web_mouse_event.PositionInWidget(),
                 DrawInputSourceType::kAltLeftButtonDrag);
  }
  // Never consumed, like the right button; only the moves in between are
  // swallowed below, so a motionless click still reaches Blink untouched.
  return false;
}

bool GestureController::HandleMouseMoveEvent(
    const blink::WebMouseEvent& web_mouse_event) {
  if (!draw_session_.is_drawing() || draw_session_.is_drawing_via_touch()) {
    return draw_session_.is_drawing_via_alt_left_button();
  }
  // The starting button can be released outside the window, losing capture
  // and the mouse-up event; checked via this event's own modifiers instead.
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
    if (!draw_session_.is_drawing_via_touch()) {
      FinishDrawing();
    }
    return false;
  }
  return false;
}

void GestureController::HandleMouseWheelEvent(
    const blink::WebMouseWheelEvent& web_mouse_wheel_event) {
  if (!mouse_modifier_config_tracker_.IsMouseWheelGesturesEnabled() ||
      !IsAllowedForSite()) {
    return;
  }
  mouse_wheel_recognizer_.HandleMouseWheelEvent(web_mouse_wheel_event);
}

void GestureController::StartDrawing(gfx::PointF location,
                                     DrawInputSourceType input_source) {
  if (!IsAllowedForSite()) {
    return;
  }
  draw_session_.Start(location, input_source);
}

void GestureController::FinishDrawing() {
  if (!draw_session_.is_drawing()) {
    // Already cancelled (idle-hold timeout, focus/visibility loss); resolve
    // the deferred menu, but leave the flag, since rocker/wheel may need it.
    if (!should_suppress_next_context_menu_ &&
        pending_context_menu_show_callback_) {
      std::move(pending_context_menu_show_callback_).Run();
    } else {
      pending_context_menu_show_callback_.Reset();
    }
    return;
  }

  // Captured before `Finish` resets the draw's input source.
  const bool was_drawing_via_right_button =
      !draw_session_.is_drawing_via_touch() &&
      !draw_session_.is_drawing_via_alt_left_button();

  draw_session_.Finish(base::BindOnce(&GestureController::FinishDrawingCallback,
                                      weak_factory_.GetWeakPtr(),
                                      was_drawing_via_right_button));
}

void GestureController::FinishDrawingCallback(
    bool was_drawing_via_right_button,
    std::optional<GestureActions> actions) {
  if (!actions || actions->empty()) {
    // On no match, a plain click runs any deferred menu show, but a
    // drawn-yet-unmatched stroke still suppresses it, same as a match.
    const bool had_movement = should_suppress_next_context_menu_;
    should_suppress_next_context_menu_ = false;
    if (had_movement) {
      pending_context_menu_show_callback_.Reset();
    } else if (pending_context_menu_show_callback_) {
      std::move(pending_context_menu_show_callback_).Run();
    }
    return;
  }

  // Only a right-button draw suppresses the next context menu; this also
  // clears a stale suppression `SuppressNextContextMenu` set on movement.
  should_suppress_next_context_menu_ = was_drawing_via_right_button;
  pending_context_menu_show_callback_.Reset();
  action_dispatcher_.DispatchActionsAsync(
      std::move(*actions), draw_session_.TakeSmartGestureContent(),
      base::DoNothing());
}

void GestureController::CancelDrawingAndSuppressContextMenu() {
  draw_session_.Cancel();
  should_suppress_next_context_menu_ = true;
}

void GestureController::SuppressNextContextMenu() {
  should_suppress_next_context_menu_ = true;
}

}  // namespace gestures
