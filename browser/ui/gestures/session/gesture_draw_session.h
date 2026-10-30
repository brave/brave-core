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
#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_recognizer.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "brave/components/gestures/core/common/input/draw_input_source_types.h"
#include "brave/components/gestures/core/common/mojom/smart_gesture_content_extractor.mojom.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/geometry/point_f.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace gestures {

class GestureWidget;

// Owns a single in-progress gesture draw and its live trail overlay;
// reused across draws, since `is_drawing()` is also queried between them.
class GestureDrawSession final {
 public:
  // `web_contents_getter` is called fresh each time, since the backing
  // `WebContents` can change over the session's lifetime (e.g. tab discard).
  GestureDrawSession(
      base::RepeatingCallback<content::WebContents*()> web_contents_getter,
      PrefService& prefs,
      const DirectionalGestureMapping& directional_gesture_mapping,
      const CustomGestureRecognizer& custom_gesture_recognizer,
      const SmartGestureMapping& smart_gesture_mapping,
      base::RepeatingClosure movement_callback);

  GestureDrawSession(const GestureDrawSession&) = delete;
  GestureDrawSession& operator=(const GestureDrawSession&) = delete;

  ~GestureDrawSession();

  bool is_drawing() const { return draw_input_source_type_.has_value(); }
  bool is_drawing_via_touch() const {
    return draw_input_source_type_ == DrawInputSourceType::kTouchDrag;
  }
  bool is_drawing_via_alt_left_button() const {
    return draw_input_source_type_ == DrawInputSourceType::kAltLeftButtonDrag;
  }

  void Start(gfx::PointF location, DrawInputSourceType input_source);
  void Continue(gfx::PointF location);
  void Finish(base::OnceCallback<void(std::optional<GestureActions>)> callback);
  void Cancel();

  // The link/image/selection found at this draw's start point, if any;
  // consumed and cleared once read, valid only for the result of `Finish`.
  std::optional<SmartGestureContentInfo> TakeSmartGestureContent();

  // Must be called whenever `web_contents_getter_` starts returning a different
  // `WebContents`, or `gesture_widget_`'s cached one would dangle.
  void DiscardOverlayWidget();

 private:
  void CheckIdleHoldTimeout();

  void UpdateOverlayWidget();

  void GetSmartGestureContentAtPoint(gfx::PointF location);
  void GetSmartContentAtPointCallback(
      gfx::PointF location,
      int process_id,
      mojom::SmartGestureContentPtr content,
      const url::Origin& hit_frame_origin,
      const blink::LocalFrameToken& hit_frame_token);

  // Resolves the match using this draw's path and whatever content, if any,
  // was found at its start point.
  GestureActions MatchDrawnGesture() const;

  // Resamples `drawn_gesture_path_` to even spacing first, so the result
  // doesn't depend on how unevenly the original points were sampled.
  DirectionalGestureStrokes ClassifyResampledDirectionalGestureStrokes() const;

  base::RepeatingCallback<content::WebContents*()> web_contents_getter_;
  const raw_ref<PrefService> prefs_;
  const raw_ref<const DirectionalGestureMapping> directional_gesture_mapping_;
  const raw_ref<const CustomGestureRecognizer> custom_gesture_recognizer_;
  const raw_ref<const SmartGestureMapping> smart_gesture_mapping_;
  base::RepeatingClosure movement_callback_;

  // Reconstructed on each `Start`, since its sensitivity config is fixed at
  // construction but must reflect the pref's current value.
  DirectionalGestureRecognizer directional_gesture_recognizer_;

  std::unique_ptr<GestureWidget> gesture_widget_;

  // `nullopt` when no draw is in progress.
  std::optional<DrawInputSourceType> draw_input_source_type_;

  // Kept in full since `directional_gesture_recognizer_` reduces movement to
  // discrete per-stroke directions; custom matching needs the real shape.
  GesturePath drawn_gesture_path_;

  // Polls for a stationary hold timeout; a one-shot timer from button-down
  // would misfire on gestures still being actively drawn past that mark.
  base::RepeatingTimer idle_hold_poll_timer_;

  // Kept alive here for the reply to arrive; a local variable would be
  // destroyed, dropping the connection, before the renderer could respond.
  mojo::Remote<mojom::SmartGestureContentExtractor>
      smart_gesture_content_extractor_;

  // Set by `GetSmartContentAtPointCallback`, read and cleared by
  // `TakeSmartGestureContent`.
  std::optional<SmartGestureContentInfo> smart_gesture_content_;

  // Set by `Finish` if a content reply is still pending; run once it
  // arrives, or abandoned by the next `Start` if it never does.
  base::OnceCallback<void(std::optional<GestureActions>)>
      pending_finish_callback_;

  base::WeakPtrFactory<GestureDrawSession> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_DRAW_SESSION_H_
