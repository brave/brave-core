// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_DRAW_SESSION_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_DRAW_SESSION_H_

#include <memory>
#include <optional>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/timer/timer.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "ui/gfx/geometry/point_f.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace gestures {

class GestureConfigTracker;
class GestureOverlayWidget;

// Owns the lifecycle of a single in-progress directional/custom gesture draw
// and the live trail overlay: the raw traced path, the directional stroke
// recognizer, and the idle-hold timeout. Reused across draws rather than
// recreated per-draw, since `GestureController` needs to query `is_drawing()`
// and friends between draws too.
class GestureDrawSession final {
 public:
  // `web_contents_getter` is called fresh each time, rather than captured
  // once, since the backing `WebContents` can change over the owning
  // `GestureController`'s lifetime (e.g. tab discard). `prefs` doesn't need
  // the same treatment: it's tied to the tab's `BrowserContext`, which a
  // discard never changes. `on_movement` runs on every `Continue` call
  // while a draw is in progress, so the owner can suppress the context
  // menu in real time, since on some platforms it can be shown mid-drag,
  // before `MaybeFinish` ever resolves the draw's outcome.
  GestureDrawSession(
      base::RepeatingCallback<content::WebContents*()> web_contents_getter,
      PrefService& prefs,
      const GestureConfigTracker& config,
      base::RepeatingClosure on_movement);

  GestureDrawSession(const GestureDrawSession&) = delete;
  GestureDrawSession& operator=(const GestureDrawSession&) = delete;

  ~GestureDrawSession();

  bool is_drawing() const { return is_drawing_; }
  bool is_drawing_via_touch() const { return is_drawing_via_touch_; }
  bool is_drawing_via_alt_left_button() const {
    return is_drawing_via_alt_left_button_;
  }

  void Start(gfx::PointF location, bool via_touch, bool via_alt_left_button);
  void Continue(gfx::PointF location);
  // Returns `nullopt` if no draw was in progress, an empty vector if one was
  // but matched no gesture, or the matched chained actions otherwise.
  std::optional<std::vector<GestureActionInfo>> MaybeFinish();
  void Cancel();

  // Must be called whenever `web_contents_getter_` is about to start returning
  // a different `WebContents` (e.g. tab discard): `overlay_`, once created,
  // caches the `WebContents` it was shown on for its own lifetime, so
  // continuing to reuse it against a new one would show it against, and
  // eventually dereference, a dangling `WebContents`.
  void DiscardOverlay();

 private:
  void CheckHoldTimeout();
  void UpdateOverlay();
  // Resamples `raw_traced_path_` to even spacing before classifying it into
  // directional strokes, so the result doesn't depend on how unevenly the
  // original points happened to be sampled (see `ResampleAtInterval`).
  std::vector<DirectionalGestureDirectionType>
  ClassifyResampledDirectionalStrokes() const;

  base::RepeatingCallback<content::WebContents*()> web_contents_getter_;
  const raw_ref<PrefService> prefs_;
  const raw_ref<const GestureConfigTracker> config_tracker_;
  base::RepeatingClosure on_movement_;

  // Reconstructed on each `Start`, since `min_stroke_pixels` reads the
  // sensitivity pref at that point and `DirectionalGestureRecognizer`'s config
  // is fixed at construction.
  DirectionalGestureRecognizer directional_gesture_recognizer_;

  std::unique_ptr<GestureOverlayWidget> overlay_;

  bool is_drawing_ = false;

  // Whether the in-progress drawing was started by a two-finger touch drag
  // rather than a right-button drag.
  bool is_drawing_via_touch_ = false;

  // Whether the in-progress drawing was started by Alt+left-button drag.
  bool is_drawing_via_alt_left_button_ = false;

  // "Raw" since `directional_gesture_recognizer_` reduces the same movement
  // to one of 8 discrete directions per stroke, discarding the actual shape;
  // custom gesture matching and the live overlay both need the unreduced,
  // point-by-point path instead.
  std::vector<gfx::PointF> raw_traced_path_;

  // Polls for a stationary hold timeout; a one-shot timer from button-down
  // would misfire on gestures still being actively drawn past that mark.
  base::RepeatingTimer idle_hold_poll_timer_;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_DRAW_SESSION_H_
