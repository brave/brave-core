// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTROLLER_H_

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"
#include "brave/browser/ui/gestures/input/gesture_input_registrar.h"
#include "brave/browser/ui/gestures/session/gesture_config_tracker.h"
#include "brave/browser/ui/gestures/session/gesture_draw_session.h"
#include "brave/browser/ui/gestures/session/gesture_mouse_modifier_config_tracker.h"
#include "brave/browser/ui/gestures/session/gesture_mouse_rocker_recognizer.h"
#include "brave/browser/ui/gestures/session/gesture_mouse_wheel_recognizer.h"
#include "brave/components/gestures/core/common/input/draw_input_source_types.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"
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

// Detects drag/rocker gestures over page content and dispatches the mapped
// command on release. Owned by `BraveTabFeatures`, one instance per tab.
class GestureController final : public tabs::ContentsObservingTabFeature {
 public:
  DECLARE_USER_DATA(GestureController);

  explicit GestureController(tabs::TabInterface& tab);

  GestureController(const GestureController&) = delete;
  GestureController& operator=(const GestureController&) = delete;

  ~GestureController() override;

  // Owned by `BraveTabFeatures`; returns nullptr if `tab` is null or the
  // feature is disabled for it.
  static GestureController* From(tabs::TabInterface* tab);

  // Returns true and clears the flag if a gesture was just executed and the
  // upcoming context menu show should be suppressed.
  bool ConsumeSuppressNextContextMenu();

  // True while a right-button hold's outcome (gesture vs. plain click) is
  // still unknown; some platforms show the context menu before that resolves.
  bool IsDrawingGesture() const;

  // Defers showing the context menu until the draw resolves; run by
  // `FinishDrawing` only if it turned out to be a plain click.
  void DeferContextMenuShow(
      base::OnceClosure pending_context_menu_show_callback);

  // content::WebContentsObserver:
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
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

  // For drawing sensitivity/overlay prefs not owned by `config_tracker_`,
  // since those aren't tied to a particular gesture type.
  PrefService& GetPrefs() const;

  // Whether gestures are allowed to fire on the currently-committed URL.
  bool IsAllowedForSite() const;

  // Entry points for each input type, routed here from `input_registrar_`.
  void InputEventCallback(const blink::WebInputEvent& web_input_event,
                          input::InputEventSource source);
  bool HandleMouseEvent(const blink::WebMouseEvent& web_mouse_event);
  bool HandleMouseDownEvent(const blink::WebMouseEvent& web_mouse_event);
  bool HandleRightButtonDownEvent(const blink::WebMouseEvent& web_mouse_event);
  bool HandleAltLeftButtonDownEvent(
      const blink::WebMouseEvent& web_mouse_event);
  bool HandleMouseMoveEvent(const blink::WebMouseEvent& web_mouse_event);
  bool HandleMouseUpEvent(const blink::WebMouseEvent& web_mouse_event);
  void HandleMouseWheelEvent(
      const blink::WebMouseWheelEvent& web_mouse_wheel_event);

  // Gate shared by every way a draw can start; `input_source` just forwards to
  // `draw_session_.Start`, since each caller already knows it.
  void StartDrawing(gfx::PointF location, DrawInputSourceType input_source);
  // Resolves a finished draw's match, or if none was drawing, still runs or
  // resets the deferred menu show left over from `SuppressNextContextMenu`.
  void FinishDrawing();
  // Completes `FinishDrawing` once `draw_session_` resolves the match; may
  // run past this call if `StartDrawing`'s content request hasn't replied.
  void FinishDrawingCallback(bool was_drawing_via_right_button,
                             std::optional<GestureActions> actions);
  // Shared by rocker/wheel gestures; the right button also drives
  // directional drawing, so firing these must cancel that draw too.
  void CancelDrawingAndSuppressContextMenu();
  // Called by `draw_session_` on movement, since some platforms show the
  // context menu mid-drag, before mouse-up resolves the draw's outcome.
  void SuppressNextContextMenu();

  GestureConfigTracker config_tracker_;
  GestureMouseModifierConfigTracker mouse_modifier_config_tracker_;

  bool should_suppress_next_context_menu_ = false;

  // See `DeferContextMenuShow`.
  base::OnceClosure pending_context_menu_show_callback_;

  GestureDrawSession draw_session_;
  GestureActionDispatcher action_dispatcher_;
  GestureMouseRockerRecognizer mouse_rocker_recognizer_;
  GestureMouseWheelRecognizer mouse_wheel_recognizer_;

  GestureInputRegistrar input_registrar_;

  ui::ScopedUnownedUserData<GestureController> scoped_unowned_user_data_;

  // Last, so it invalidates first; `draw_session_.Finish()` can post
  // `FinishDrawingCallback` to run after this is destroyed.
  base::WeakPtrFactory<GestureController> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTROLLER_H_
