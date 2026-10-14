// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_draw_session.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "brave/browser/ui/gestures/dispatch/gesture_command_browser_util.h"
#include "brave/browser/ui/gestures/session/gesture_config_tracker.h"
#include "brave/browser/ui/gestures/widget/gesture_widget.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_match_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_util.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_geometry_util.h"
#include "brave/components/gestures/core/common/gesture_match_info.h"
#include "brave/components/gestures/core/common/gesture_match_types.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_direction_util.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

namespace {

// Two independently-resampled candidates that are both excellent matches can
// land a few ULPs apart from floating-point noise alone, not because either
// is actually the stronger match; favoring directional outside this margin
// would make the outcome depend on which resampling path happened to
// accumulate less error, rather than which shape was actually drawn.
constexpr double kDirectionalPreferenceEpsilon = 1e-6;

// A smart gesture is meant to be a quick swipe; a draw that's traveled
// farther than this is more likely an attempt at a longer directional or
// custom shape over that same content, so it should fall through to the
// normal cascade instead of being locked into the smart-gesture outcome.
bool IsWithinSmartGestureMaxDistance(const GesturePath& gesture_path) {
  return GesturePathLength(gesture_path) <=
         features::kSmartGestureMaxDistancePixels.Get();
}

// Compares a resolved directional-stroke sequence against the actual drawn
// `raw_path` on the same normalized point-sequence distance scale used for
// custom gestures (via an idealized polyline for `strokes`, see
// `DirectionalGestureStrokesToCanonicalPath`), and against the best-matching
// custom gesture, picking whichever is closer and within the match
// threshold. A directional-stroke sequence doesn't win just because it
// resolved at all,
// since a weak directional read that only explains part of the drawn shape
// can still lose to a strong custom-gesture match, or to no match at all.
// Used by both `MaybeFinish` and the live preview in `UpdateOverlayWidget`,
// so the two never disagree about what a finished draw would resolve to.
GestureMatchInfo ResolveGestureMatch(const GestureConfigTracker& config_tracker,
                                     const DirectionalGestureStrokes& strokes,
                                     const GesturePath& gesture_path) {
  const GestureActions& directional_gesture_actions =
      config_tracker.directional_gesture_mapping().ActionsForStrokes(strokes);

  const size_t custom_gesture_resample_point_count =
      features::kCustomGestureResamplePointCount.Get();
  const double match_threshold = features::kCustomGestureMaxMatchDistance.Get();

  GestureMatchInfo gesture_match;
  if (!directional_gesture_actions.empty()) {
    const GesturePath normalized_path = ResampleAndNormalizePath(
        gesture_path, custom_gesture_resample_point_count);
    const GesturePath normalized_template = ResampleAndNormalizePath(
        DirectionalGestureStrokesToCanonicalPath(strokes),
        custom_gesture_resample_point_count);
    gesture_match.directional_distance = DistanceToCandidate(
        normalized_path, normalized_template, /*candidate_is_closed=*/false);
  }
  const bool directional_gesture_matches =
      !directional_gesture_actions.empty() &&
      gesture_match.directional_distance <= match_threshold;

  const CustomGestureMatchInfo best_acceptable_custom_match =
      config_tracker.custom_gesture_recognizer().BestCustomGestureMatch(
          gesture_path);
  const bool custom_gesture_matches =
      static_cast<bool>(best_acceptable_custom_match.custom_gesture);

  if (directional_gesture_matches &&
      (!custom_gesture_matches || gesture_match.directional_distance <=
                                      best_acceptable_custom_match.distance +
                                          kDirectionalPreferenceEpsilon)) {
    gesture_match.type = GestureMatchType::kDirectional;
    gesture_match.actions = directional_gesture_actions;
  } else if (custom_gesture_matches) {
    gesture_match.type = GestureMatchType::kCustom;
    gesture_match.actions =
        best_acceptable_custom_match.custom_gesture->actions;
  }
  return gesture_match;
}

}  // namespace

GestureDrawSession::GestureDrawSession(
    base::RepeatingCallback<content::WebContents*()> web_contents_getter,
    PrefService& prefs,
    const GestureConfigTracker& config_tracker,
    base::RepeatingClosure on_movement)
    : web_contents_getter_(std::move(web_contents_getter)),
      prefs_(prefs),
      config_tracker_(config_tracker),
      on_movement_(std::move(on_movement)) {}

GestureDrawSession::~GestureDrawSession() = default;

void GestureDrawSession::Start(gfx::PointF location,
                               DrawInputSourceType input_source) {
  draw_input_source_ = input_source;
  // `location` is pushed as the first point so a custom gesture match can be
  // attempted even if the hold never moves far enough to register as a
  // directional stroke.
  drawn_gesture_path_.clear();
  drawn_gesture_path_.push_back(location);
  smart_gesture_content_.reset();
  RequestSmartGestureContentAtPoint(location);

  const double drag_threshold_pixels =
      prefs_->GetDouble(prefs::kDirectionalGestureSensitivityInPixels);
  directional_gesture_recognizer_.Reset(location, base::TimeTicks::Now(),
                                        drag_threshold_pixels);

  idle_hold_poll_timer_.Start(
      FROM_HERE, features::kGestureSessionIdleHoldPollInterval.Get(), this,
      &GestureDrawSession::CheckHoldTimeout);
}

void GestureDrawSession::RequestSmartGestureContentAtPoint(
    gfx::PointF location) {
  content::WebContents* const web_contents = web_contents_getter_.Run();
  content::RenderFrameHost* const focused_render_frame_host =
      web_contents ? web_contents->GetFocusedFrame() : nullptr;
  if (!focused_render_frame_host) {
    return;
  }
  smart_gesture_content_extractor_.reset();
  focused_render_frame_host->GetRemoteInterfaces()->GetInterface(
      smart_gesture_content_extractor_.BindNewPipeAndPassReceiver());
  smart_gesture_content_extractor_->GetContentAtPoint(
      location,
      base::BindOnce(&GestureDrawSession::OnSmartGestureContentReceived,
                     weak_factory_.GetWeakPtr(), location));
}

void GestureDrawSession::OnSmartGestureContentReceived(
    gfx::PointF location,
    mojom::SmartGestureContentPtr content) {
  smart_gesture_content_extractor_.reset();
  if (!content) {
    return;
  }
  if (content->is_link_url()) {
    smart_gesture_content_ =
        SmartGestureContentInfo{SmartGestureContentType::kLink,
                                content->get_link_url().spec(), location};
  } else if (content->is_image_url()) {
    smart_gesture_content_ =
        SmartGestureContentInfo{SmartGestureContentType::kImage,
                                content->get_image_url().spec(), location};
  } else {
    smart_gesture_content_ =
        SmartGestureContentInfo{SmartGestureContentType::kSelectedText,
                                content->get_selected_text(), location};
  }
}

std::optional<SmartGestureContentInfo>
GestureDrawSession::TakeSmartGestureContent() {
  return std::exchange(smart_gesture_content_, std::nullopt);
}

void GestureDrawSession::Continue(gfx::PointF location) {
  TRACE_EVENT2("input", "GestureDrawSession::Continue", "x", location.x(), "y",
               location.y());
  if (!draw_input_source_) {
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
  // Rejects points too close to the last accepted one, so jitter from a
  // high-polling-rate mouse or trackpad doesn't bloat the path or skew the
  // arc-length-based resampling done when matching/classifying it later.
  const double min_point_distance_pixels =
      features::kGestureSessionMinPointDistancePixels.Get();
  if (IsFarEnoughFromLastDrawnPoint(drawn_gesture_path_.back(), location,
                                    min_point_distance_pixels)) {
    drawn_gesture_path_.push_back(location);
  }
  const base::TimeTicks now = base::TimeTicks::Now();
  directional_gesture_recognizer_.OnMouseMoved(location, now);
  if (directional_gesture_recognizer_.TimedOut(now)) {
    Cancel();
    return;
  }

  // Deferred from `Start`, since a plain click that never turns into a drag
  // should never show the overlay, not even briefly.
  content::WebContents* const web_contents_for_widget =
      web_contents_getter_.Run();
  const bool gesture_widget_already_shown =
      gesture_widget_ && gesture_widget_->IsVisible();
  if (web_contents_for_widget && !gesture_widget_already_shown &&
      (prefs_->GetBoolean(prefs::kShowTrail) ||
       prefs_->GetBoolean(prefs::kShowMatchedGesture))) {
    if (!gesture_widget_) {
      gesture_widget_ =
          std::make_unique<GestureWidget>(web_contents_for_widget);
    }
    gesture_widget_->Show();
  }
  UpdateOverlayWidget();
}

std::optional<GestureActions> GestureDrawSession::MaybeFinish() {
  if (!draw_input_source_) {
    return std::nullopt;
  }
  Cancel();

  // A smart gesture (the draw started on a link, image, or selection) takes
  // priority over the directional/custom cascade below, since it's
  // a different mechanism entirely: a single swipe over specific page
  // content, not a shape. `smart_gesture_content_` itself is left set for
  // `TakeSmartGestureContent` to hand the dispatcher the actual captured
  // link/image/selection; only whether an action is assigned is checked
  // here. `IsWithinSmartGestureMaxDistance` keeps a longer draw over that
  // same content free to resolve as directional/custom instead.
  if (smart_gesture_content_ &&
      IsWithinSmartGestureMaxDistance(drawn_gesture_path_)) {
    const SmartGestureDirectionType smart_direction_type =
        ClassifySmartGestureDirection(drawn_gesture_path_);
    const GestureActions& smart_actions =
        config_tracker_->smart_gesture_mapping().ActionsFor(
            smart_gesture_content_->type, smart_direction_type);
    if (!smart_actions.empty()) {
      return smart_actions;
    }
  }

  const DirectionalGestureStrokes strokes =
      ClassifyResampledDirectionalStrokes();
  const GestureMatchInfo gesture_match =
      ResolveGestureMatch(*config_tracker_, strokes, drawn_gesture_path_);
  return gesture_match.actions;
}

void GestureDrawSession::Cancel() {
  draw_input_source_.reset();
  idle_hold_poll_timer_.Stop();
  if (gesture_widget_) {
    gesture_widget_->Hide();
  }
}

void GestureDrawSession::DiscardOverlayWidget() {
  Cancel();
  gesture_widget_.reset();
}

void GestureDrawSession::CheckHoldTimeout() {
  if (directional_gesture_recognizer_.TimedOut(base::TimeTicks::Now())) {
    Cancel();
  }
}

void GestureDrawSession::UpdateOverlayWidget() {
  if (!gesture_widget_) {
    return;
  }

  const bool should_show_trail = prefs_->GetBoolean(prefs::kShowTrail);
  if (!prefs_->GetBoolean(prefs::kShowMatchedGesture)) {
    gesture_widget_->UpdateTrail(should_show_trail, drawn_gesture_path_,
                                 /*label=*/u"");
    return;
  }

  // A smart gesture, like in `MaybeFinish`, takes priority over the
  // directional/custom cascade below; re-evaluated fresh on every
  // call here too, so the live preview matches what `MaybeFinish` would
  // actually resolve to.
  if (smart_gesture_content_ &&
      IsWithinSmartGestureMaxDistance(drawn_gesture_path_)) {
    const SmartGestureDirectionType smart_direction_type =
        ClassifySmartGestureDirection(drawn_gesture_path_);
    const GestureActions& smart_actions =
        config_tracker_->smart_gesture_mapping().ActionsFor(
            smart_gesture_content_->type, smart_direction_type);
    if (!smart_actions.empty()) {
      gesture_widget_->UpdateTrail(should_show_trail, drawn_gesture_path_,
                                   ToActionsDisplayName(smart_actions));
      return;
    }
  }

  // Re-evaluated fresh on every call here mid-drag, from the full path drawn
  // so far, so the live preview always reflects this draw's honest,
  // moment-to-moment state; `MaybeFinish` resolves the same way at release,
  // so what fires always matches what was last shown here.
  const DirectionalGestureStrokes strokes =
      ClassifyResampledDirectionalStrokes();
  const GestureMatchInfo gesture_match =
      ResolveGestureMatch(*config_tracker_, strokes, drawn_gesture_path_);

  std::u16string label;
  switch (gesture_match.type) {
    case GestureMatchType::kDirectional:
      label =
          base::UTF8ToUTF16(DirectionalGestureStrokeToKeyCode(strokes.back())) +
          u" " + ToActionsDisplayName(gesture_match.actions);
      break;
    case GestureMatchType::kCustom:
      label = ToActionsDisplayName(gesture_match.actions);
      break;
    case GestureMatchType::kUnmatched:
      label = l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_UNKNOWN_GESTURE);
      break;
  }
  gesture_widget_->UpdateTrail(should_show_trail, drawn_gesture_path_, label);
}

DirectionalGestureStrokes
GestureDrawSession::ClassifyResampledDirectionalStrokes() const {
  const double min_stroke_pixels =
      prefs_->GetDouble(prefs::kDirectionalGestureSensitivityInPixels);
  // Resampling first makes the result depend only on the shape drawn, not on
  // how unevenly the original points happened to be sampled (e.g. a brief
  // slowdown mid-drag producing two unusually close consecutive points,
  // which could otherwise fail a sustain check that a steadier draw of the
  // exact same shape would have passed).
  const GesturePath resampled_gesture_path = ResampleGesturePathAtInterval(
      drawn_gesture_path_,
      base::saturated_cast<float>(min_stroke_pixels / 3.0));
  return ClassifyDirectionalGestureStrokes(
      resampled_gesture_path, min_stroke_pixels,
      features::kDirectionalGestureMinStrokeStraightness.Get(),
      features::kMaxDirectionalGestureStrokes.Get());
}

}  // namespace gestures
