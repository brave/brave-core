// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTROLLER_H_

#include "base/functional/callback.h"
#include "brave/browser/ui/gestures/gesture_action_dispatcher.h"
#include "brave/browser/ui/gestures/gesture_config_tracker.h"
#include "brave/browser/ui/gestures/gesture_draw_session.h"
#include "brave/browser/ui/gestures/gesture_input_registrar.h"
#include "brave/browser/ui/gestures/gesture_mouse_rocker_recognizer.h"
#include "brave/browser/ui/gestures/gesture_mouse_wheel_recognizer.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "ui/gfx/geometry/point_f.h"

class PrefService;

namespace blink {
class WebInputEvent;
class WebMouseEvent;
class WebMouseWheelEvent;
}  // namespace blink

namespace content {
class RenderFrameHost;
class RenderViewHost;
class RenderWidgetHost;
class WebContents;
enum class Visibility;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace gestures {

// Detects drag gestures over page content and dispatches the mapped browser
// command on release. A gesture can be a right-button drag, a two-finger
// touchscreen drag (not a trackpad: Chromium typically turns a trackpad's
// two-finger input into a scroll/pinch gesture before it ever reaches this
// class as a raw touch event), or an Alt+left-button drag as a
// trackpad-friendly alternative. Also detects rocker gestures: pressing the
// other mouse button while one is already held. Owned by `BraveTabFeatures`,
// one instance per tab.
//
// Input is observed, never consumed, via `input_registrar_` (see its own
// class comment). Native drag/selection behavior is therefore never
// interfered with directly; the context menu is instead suppressed
// separately, by each platform's context menu override calling
// `ConsumeSuppressContextMenuForWebContents` below.
class GestureController final : public tabs::ContentsObservingTabFeature {
 public:
  explicit GestureController(tabs::TabInterface& tab);

  GestureController(const GestureController&) = delete;
  GestureController& operator=(const GestureController&) = delete;

  ~GestureController() override;

  // Returns true and clears the flag if a gesture was just executed and the
  // upcoming context menu show should be suppressed.
  bool ConsumeSuppressNextContextMenu();

  // True while a right-button hold is still being drawn and its outcome (a
  // matched gesture vs. a plain click) is not yet known. On some platforms
  // the native context menu is shown synchronously on button-down, before
  // any button-up/`FinishDrawing` can resolve that outcome.
  bool IsDrawingGesture() const;

  // Defers actually showing the context menu until the in-progress gesture
  // resolves: run by `FinishDrawing` if the hold turns out to be a plain
  // click with no matched gesture, discarded without running otherwise.
  // Replaces any previously deferred closure.
  void DeferContextMenuShow(base::OnceClosure pending_context_menu_show);

  // content::WebContentsObserver:
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  // Fires on same-process navigations (e.g. a reload) too, under
  // RenderDocument, where the main frame's `RenderFrameHost` -- and its
  // `RenderWidgetHost` -- is replaced without `RenderViewHostChanged` firing.
  // Also fires for subframes, e.g. a sponsored rich-media background's own
  // out-of-process iframe, which otherwise has its own `RenderWidgetHost`
  // the main frame's never sees input from.
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void RenderViewDeleted(content::RenderViewHost* render_view_host) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void OnWebContentsLostFocus(
      content::RenderWidgetHost* render_widget_host) override;

 private:
  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // For prefs not owned by `config_tracker_`: drawing sensitivity and overlay
  // display options, which aren't tied to a particular gesture type.
  PrefService& GetPrefs() const;

  // Whether gestures are allowed to fire on the currently-committed URL.
  bool IsAllowedForSite() const;

  // Entry points for each raw input type, routed here from
  // `input_registrar_`.
  void OnInputEvent(const blink::WebInputEvent& web_input_event,
                    input::InputEventSource source);
  bool HandleMouseEvent(const blink::WebMouseEvent& web_mouse_event);
  bool HandleMouseDownEvent(const blink::WebMouseEvent& web_mouse_event);
  bool HandleMouseMoveEvent(const blink::WebMouseEvent& web_mouse_event);
  bool HandleMouseUpEvent(const blink::WebMouseEvent& web_mouse_event);
  void HandleMouseWheelEvent(
      const blink::WebMouseWheelEvent& web_mouse_wheel_event);

  // Gate shared by every way a draw can start;
  // `via_touch`/`via_alt_left_button` just forward to `draw_session_.Start`,
  // matching how each caller already knows which input triggered it.
  void StartDrawing(gfx::PointF location,
                    bool via_touch,
                    bool via_alt_left_button);
  // Resolves the outcome of a finished draw: no-op if none was in progress,
  // runs the deferred context menu show if one was but matched no gesture,
  // or dispatches the matched chained actions otherwise.
  void FinishDrawing();
  // Shared by rocker gestures and a wheel gesture bound to the right mouse
  // button: that button also drives directional-gesture drawing and the
  // native context menu, so firing any of these needs to cancel a
  // conflicting in-progress draw and suppress the menu show that would
  // otherwise follow this button's release.
  void CancelDrawingAndSuppressContextMenu();
  // Called by `draw_session_` on real movement, before the draw's outcome
  // is known: on some platforms the context menu can be shown mid-drag,
  // before mouse-up ever resolves whether the hold is a gesture or a plain
  // click.
  void SuppressNextContextMenu();

  GestureConfigTracker config_tracker_;

  bool should_suppress_next_context_menu_ = false;

  // See `DeferContextMenuShow`.
  base::OnceClosure pending_context_menu_show_;

  GestureDrawSession draw_session_;
  GestureActionDispatcher action_dispatcher_;
  GestureMouseRockerRecognizer mouse_rocker_recognizer_;
  GestureMouseWheelRecognizer mouse_wheel_recognizer_;

  GestureInputRegistrar input_registrar_;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTROLLER_H_
