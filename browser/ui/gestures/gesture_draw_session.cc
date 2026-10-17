// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_draw_session.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "brave/browser/ui/gestures/gesture_command_browser_mapping.h"
#include "brave/browser/ui/gestures/gesture_config_tracker.h"
#include "brave/browser/ui/gestures/gesture_overlay_widget.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

GestureDrawSession::GestureDrawSession(
    base::RepeatingCallback<content::WebContents*()> web_contents_getter,
    PrefService& prefs,
    const GestureConfigTracker& config,
    base::RepeatingClosure on_movement)
    : web_contents_getter_(std::move(web_contents_getter)),
      prefs_(prefs),
      config_tracker_(config),
      on_movement_(std::move(on_movement)) {}

GestureDrawSession::~GestureDrawSession() = default;

void GestureDrawSession::Start(gfx::PointF location,
                               bool via_touch,
                               bool via_alt_left_button) {
  is_drawing_ = true;
  is_drawing_via_touch_ = via_touch;
  is_drawing_via_alt_left_button_ = via_alt_left_button;
  // `location` is pushed as the first point so a custom gesture match can be
  // attempted even if the hold never moves far enough to register as a
  // directional stroke.
  raw_traced_path_.clear();
  raw_traced_path_.push_back(location);

  const double drag_threshold_pixels =
      prefs_->GetDouble(prefs::kSensitivityInPixels);
  directional_gesture_recognizer_.Reset(location, base::TimeTicks::Now(),
                                        drag_threshold_pixels);

  idle_hold_poll_timer_.Start(FROM_HERE, features::kIdleHoldPollInterval.Get(),
                              this, &GestureDrawSession::CheckHoldTimeout);
}

void GestureDrawSession::Continue(gfx::PointF location) {
  TRACE_EVENT2("input", "GestureDrawSession::Continue", "x", location.x(), "y",
               location.y());
  if (!is_drawing_) {
    return;
  }
  // Real movement, as opposed to a plain click never turning into a drag.
  // Signaled here, rather than waiting for `MaybeFinish` at mouse-up, since on
  // some platforms the right-button contextual menu can appear mid-drag,
  // before mouse-up ever fires.
  on_movement_.Run();
  // Tracked independently of `directional_gesture_recognizer_`'s own stroke
  // classification below, since custom gesture matching needs the actual
  // shape drawn, not just which of the 8 directions each segment resolved to.
  raw_traced_path_.push_back(location);
  const base::TimeTicks now = base::TimeTicks::Now();
  directional_gesture_recognizer_.OnMouseMoved(location, now);
  if (directional_gesture_recognizer_.TimedOut(now)) {
    Cancel();
    return;
  }

  // Deferred from `Start`, since a plain click that never turns into a drag
  // should never show the overlay, not even briefly.
  const bool overlay_already_shown = overlay_ && overlay_->IsShown();
  if (!overlay_already_shown &&
      (prefs_->GetBoolean(prefs::kShowTrail) ||
       prefs_->GetBoolean(prefs::kShowMatchedGesture))) {
    if (!overlay_) {
      overlay_ =
          std::make_unique<GestureOverlayWidget>(web_contents_getter_.Run());
    }
    overlay_->Show();
  }
  UpdateOverlay();
}

std::optional<std::vector<GestureActionInfo>>
GestureDrawSession::MaybeFinish() {
  if (!is_drawing_) {
    return std::nullopt;
  }
  Cancel();

  std::vector<GestureActionInfo> actions =
      config_tracker_->directional_gesture_mapping().ActionsForStrokes(
          ClassifyResampledDirectionalStrokes());
  if (!actions.empty()) {
    return actions;
  }
  return config_tracker_->custom_gesture_recognizer().ActionsForMatch(
      raw_traced_path_);
}

void GestureDrawSession::Cancel() {
  is_drawing_ = false;
  is_drawing_via_touch_ = false;
  is_drawing_via_alt_left_button_ = false;
  idle_hold_poll_timer_.Stop();
  if (overlay_) {
    overlay_->Hide();
  }
}

void GestureDrawSession::DiscardOverlay() {
  Cancel();
  overlay_.reset();
}

void GestureDrawSession::CheckHoldTimeout() {
  if (directional_gesture_recognizer_.TimedOut(base::TimeTicks::Now())) {
    Cancel();
  }
}

void GestureDrawSession::UpdateOverlay() {
  if (!overlay_) {
    return;
  }

  const bool should_show_trail = prefs_->GetBoolean(prefs::kShowTrail);
  if (!prefs_->GetBoolean(prefs::kShowMatchedGesture)) {
    overlay_->UpdateTrail(raw_traced_path_, should_show_trail,
                          std::u16string());
    return;
  }

  const std::vector<DirectionalGestureDirectionType> strokes =
      ClassifyResampledDirectionalStrokes();
  const std::vector<GestureActionInfo>& stroke_actions =
      config_tracker_->directional_gesture_mapping().ActionsForStrokes(strokes);
  if (!stroke_actions.empty()) {
    const std::u16string label =
        base::UTF8ToUTF16(DirectionToGestureKeyCode(strokes.back())) + u" " +
        ToDisplayName(stroke_actions.front().command);
    overlay_->UpdateTrail(raw_traced_path_, should_show_trail, label);
    return;
  }

  // Falls back to a live preview of the custom gesture match, re-evaluated on
  // every call here mid-drag, matching what `MaybeFinish` will resolve to once
  // the button is actually released. Without this, the label would show
  // "Unmatched" for the entire draw of a custom gesture, only resolving to the
  // real match the instant the button comes up.
  const std::vector<GestureActionInfo>& custom_gesture_actions =
      config_tracker_->custom_gesture_recognizer().ActionsForMatch(
          raw_traced_path_);
  const std::u16string label =
      custom_gesture_actions.empty()
          ? l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_UNKNOWN_GESTURE)
          : ToDisplayName(custom_gesture_actions.front().command);
  overlay_->UpdateTrail(raw_traced_path_, should_show_trail, label);
}

std::vector<DirectionalGestureDirectionType>
GestureDrawSession::ClassifyResampledDirectionalStrokes() const {
  const double min_stroke_pixels =
      prefs_->GetDouble(prefs::kSensitivityInPixels);
  // Resampling first makes the result depend only on the shape drawn, not on
  // how unevenly the original points happened to be sampled (e.g. a brief
  // slowdown mid-drag producing two unusually close consecutive points,
  // which could otherwise fail a sustain check that a steadier draw of the
  // exact same shape would have passed).
  const std::vector<gfx::PointF> resampled_path = ResampleAtInterval(
      raw_traced_path_, static_cast<float>(min_stroke_pixels / 3.0));
  return ClassifyDirectionalStrokes(resampled_path, min_stroke_pixels,
                                    features::kMaxDirectionalStrokes.Get());
}

}  // namespace gestures
