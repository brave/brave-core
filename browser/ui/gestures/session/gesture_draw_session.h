// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_DRAW_SESSION_H_
#define BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_DRAW_SESSION_H_

#include <memory>
#include <optional>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/draw_input_source_types.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "brave/components/gestures/core/common/mojom/smart_gesture_content_extractor.mojom.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/geometry/point_f.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace gestures {

class GestureConfigTracker;
class GestureWidget;

// Owns a single in-progress directional/custom gesture draw and its live trail
// overlay. Reused across draws, since `GestureController` queries
// `is_drawing()` between draws too.
class GestureDrawSession final {
 public:
  // `web_contents_getter` is called fresh each time, since the backing
  // `WebContents` can change over the session's lifetime (e.g. tab discard).
  GestureDrawSession(
      base::RepeatingCallback<content::WebContents*()> web_contents_getter,
      PrefService& prefs,
      const GestureConfigTracker& config_tracker,
      base::RepeatingClosure on_movement);

  GestureDrawSession(const GestureDrawSession&) = delete;
  GestureDrawSession& operator=(const GestureDrawSession&) = delete;

  ~GestureDrawSession();

  bool is_drawing() const { return static_cast<bool>(draw_input_source_); }
  bool is_drawing_via_touch() const {
    return draw_input_source_ == DrawInputSourceType::kTouchDrag;
  }
  bool is_drawing_via_alt_left_button() const {
    return draw_input_source_ == DrawInputSourceType::kAltLeftButtonDrag;
  }

  void Start(gfx::PointF location, DrawInputSourceType input_source);
  void Continue(gfx::PointF location);
  // Returns `nullopt` if no draw was in progress, an empty vector if one was
  // but matched no gesture, or the matched chained actions otherwise. Once
  // this returns, `TakeSmartGestureContent()` has whatever this draw's start
  // point found, if anything.
  std::optional<GestureActions> MaybeFinish();
  void Cancel();

  // The link/image/selection found at this draw's start point, if any;
  // consumed (and cleared) by the caller once read, since it's only valid
  // for the single `GestureActions` just returned by `MaybeFinish`.
  std::optional<SmartGestureContentInfo> TakeSmartGestureContent();

  // Must be called whenever `web_contents_getter_` starts returning a different
  // `WebContents`, or `gesture_widget_`'s cached one would dangle.
  void DiscardOverlayWidget();

 private:
  void CheckHoldTimeout();

  void UpdateOverlayWidget();

  // Fires `smart_gesture_content_extractor_`'s request for whatever's at
  // `location`, the draw's start point; the reply is almost always back well
  // before `MaybeFinish` needs it, but if not, that draw simply has no content.
  void RequestSmartGestureContentAtPoint(gfx::PointF location);
  void OnSmartGestureContentReceived(gfx::PointF location,
                                     mojom::SmartGestureContentPtr content);

  // Resamples `drawn_gesture_path_` to even spacing before classifying it into
  // directional strokes, so the result doesn't depend on how unevenly the
  // original points happened to be sampled (see
  // `ResampleGesturePathAtInterval`).
  DirectionalGestureStrokes ClassifyResampledDirectionalStrokes() const;

  base::RepeatingCallback<content::WebContents*()> web_contents_getter_;
  const raw_ref<PrefService> prefs_;
  const raw_ref<const GestureConfigTracker> config_tracker_;
  base::RepeatingClosure on_movement_;

  // Reconstructed on each `Start`, since `min_stroke_pixels` reads the
  // sensitivity pref at that point and `DirectionalGestureRecognizer`'s config
  // is fixed at construction.
  DirectionalGestureRecognizer directional_gesture_recognizer_;

  std::unique_ptr<GestureWidget> gesture_widget_;

  // `nullopt` when no draw is in progress.
  std::optional<DrawInputSourceType> draw_input_source_;

  // "Raw" since `directional_gesture_recognizer_` reduces the same movement to
  // one of 8 discrete directions per stroke, discarding the actual shape;
  // custom gesture matching and the live overlay both need the unreduced,
  // point-by-point path instead.
  GesturePath drawn_gesture_path_;

  // Polls for a stationary hold timeout; a one-shot timer from button-down
  // would misfire on gestures still being actively drawn past that mark.
  base::RepeatingTimer idle_hold_poll_timer_;

  // Kept alive here for `RequestSmartGestureContentAtPoint`'s reply to arrive;
  // a local variable would be destroyed, dropping the connection, before the
  // renderer could respond.
  mojo::Remote<mojom::SmartGestureContentExtractor>
      smart_gesture_content_extractor_;

  // Set by `OnSmartGestureContentReceived`, read and cleared by
  // `TakeSmartGestureContent`; `nullopt` once no draw is in progress or nothing
  // was found.
  std::optional<SmartGestureContentInfo> smart_gesture_content_;

  base::WeakPtrFactory<GestureDrawSession> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_DRAW_SESSION_H_
