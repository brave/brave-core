// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_draw_session.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/browser/ui/gestures/session/gesture_command_display_name_util.h"
#include "brave/browser/ui/gestures/widget/gesture_widget.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_key_util.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_match_info.h"
#include "brave/components/gestures/core/common/gesture_match_types.h"
#include "brave/components/gestures/core/common/gesture_match_util.h"
#include "brave/components/gestures/core/common/gesture_path_distance_util.h"
#include "brave/components/gestures/core/common/gesture_path_resample_util.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

GestureDrawSession::GestureDrawSession(
    base::RepeatingCallback<content::WebContents*()> web_contents_getter,
    PrefService& prefs,
    const DirectionalGestureMapping& directional_gesture_mapping,
    const CustomGestureRecognizer& custom_gesture_recognizer,
    const SmartGestureMapping& smart_gesture_mapping,
    base::RepeatingClosure movement_callback)
    : web_contents_getter_(std::move(web_contents_getter)),
      prefs_(prefs),
      directional_gesture_mapping_(directional_gesture_mapping),
      custom_gesture_recognizer_(custom_gesture_recognizer),
      smart_gesture_mapping_(smart_gesture_mapping),
      movement_callback_(std::move(movement_callback)) {}

GestureDrawSession::~GestureDrawSession() = default;

void GestureDrawSession::Start(gfx::PointF location,
                               DrawInputSourceType input_source) {
  if (pending_finish_callback_) {
    // The previous draw's content request never replied before this new
    // draw began; it can't match on content it will now never receive.
    std::move(pending_finish_callback_).Run(GestureActions{});
  }
  draw_input_source_type_ = input_source;
  // `location` is pushed as the first point so a custom gesture match can be
  // attempted even if the hold never moves far enough for a directional stroke.
  drawn_gesture_path_.clear();
  drawn_gesture_path_.push_back(location);
  smart_gesture_content_.reset();
  GetSmartGestureContentAtPoint(location);

  const double drag_threshold_pixels =
      prefs_->GetDouble(prefs::kDirectionalGestureSensitivityInPixels);
  directional_gesture_recognizer_.Reset(location, base::TimeTicks::Now(),
                                        drag_threshold_pixels);

  idle_hold_poll_timer_.Start(
      FROM_HERE, features::kGestureSessionIdleHoldPollInterval.Get(), this,
      &GestureDrawSession::CheckIdleHoldTimeout);
}

void GestureDrawSession::GetSmartGestureContentAtPoint(gfx::PointF location) {
  content::WebContents* const web_contents = web_contents_getter_.Run();
  // The main frame, not whichever has focus; `location` is a root-frame
  // point, and Blink's hit test delegates into subframes from there.
  content::RenderFrameHost* const main_render_frame_host =
      web_contents ? web_contents->GetPrimaryMainFrame() : nullptr;
  if (!main_render_frame_host) {
    return;
  }
  smart_gesture_content_extractor_.reset();
  main_render_frame_host->GetRemoteInterfaces()->GetInterface(
      smart_gesture_content_extractor_.BindNewPipeAndPassReceiver());
  // The hit frame is always in this same process, even if it turns out to
  // differ from `main_render_frame_host` itself.
  smart_gesture_content_extractor_->GetContentAtPoint(
      location,
      base::BindOnce(&GestureDrawSession::GetSmartContentAtPointCallback,
                     weak_factory_.GetWeakPtr(), location,
                     main_render_frame_host->GetProcess()->GetDeprecatedID()));
}

void GestureDrawSession::GetSmartContentAtPointCallback(
    gfx::PointF location,
    int process_id,
    mojom::SmartGestureContentPtr content,
    const url::Origin& hit_frame_origin,
    const blink::LocalFrameToken& hit_frame_token) {
  smart_gesture_content_extractor_.reset();
  if (content) {
    if (content->is_link_url()) {
      smart_gesture_content_ =
          SmartGestureContentInfo{.type = SmartGestureContentType::kLink,
                                  .value = content->get_link_url().spec(),
                                  .point = location,
                                  .origin = hit_frame_origin,
                                  .frame_token = hit_frame_token,
                                  .process_id = process_id};
    } else if (content->is_image_url()) {
      smart_gesture_content_ =
          SmartGestureContentInfo{.type = SmartGestureContentType::kImage,
                                  .value = content->get_image_url().spec(),
                                  .point = location,
                                  .origin = hit_frame_origin,
                                  .frame_token = hit_frame_token,
                                  .process_id = process_id};
    } else {
      smart_gesture_content_ = SmartGestureContentInfo{
          .type = SmartGestureContentType::kSelectedText,
          .value = content->get_selected_text(),
          .point = location,
          .origin = hit_frame_origin,
          .frame_token = hit_frame_token,
          .process_id = process_id};
    }
  }
  if (pending_finish_callback_) {
    std::move(pending_finish_callback_).Run(MatchDrawnGesture());
  }
}

std::optional<SmartGestureContentInfo>
GestureDrawSession::TakeSmartGestureContent() {
  return std::exchange(smart_gesture_content_, std::nullopt);
}

void GestureDrawSession::Continue(gfx::PointF location) {
  if (!draw_input_source_type_) {
    return;
  }
  // Marks this as movement, not a plain click; signaled here, not at
  // mouse-up, since the right-button context menu can appear mid-drag.
  movement_callback_.Run();

  // `drawn_gesture_path_` is kept independently of
  // `directional_gesture_recognizer_`, since custom matching needs the shape.
  const double min_point_distance_pixels =
      features::kGestureSessionMinPointDistancePixels.Get();
  // Rejects points too close to the last accepted one, so jitter from a
  // high-polling-rate mouse or trackpad doesn't skew later resampling.
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

void GestureDrawSession::Finish(
    base::OnceCallback<void(std::optional<GestureActions>)> callback) {
  if (!draw_input_source_type_) {
    // Posted, not run inline, so callers see one consistent ordering
    // regardless of whether a content reply is still pending below.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::nullopt));
    return;
  }
  Cancel();

  if (smart_gesture_content_extractor_) {
    // `Start`'s content request hasn't replied yet; resolving now would
    // make a quick swipe lose it to that race.
    pending_finish_callback_ = std::move(callback);
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), MatchDrawnGesture()));
}

GestureActions GestureDrawSession::MatchDrawnGesture() const {
  // `smart_gesture_content_` stays set here, so `TakeSmartGestureContent`
  // can hand the dispatcher the captured link/image/selection.
  const GestureMatchInfo gesture_match = MatchGesture(
      *smart_gesture_mapping_, smart_gesture_content_,
      *directional_gesture_mapping_, *custom_gesture_recognizer_,
      ClassifyResampledDirectionalGestureStrokes(), drawn_gesture_path_);
  return gesture_match.actions;
}

void GestureDrawSession::Cancel() {
  draw_input_source_type_.reset();
  idle_hold_poll_timer_.Stop();
  if (gesture_widget_) {
    gesture_widget_->Hide();
  }
}

void GestureDrawSession::DiscardOverlayWidget() {
  Cancel();
  gesture_widget_.reset();
}

void GestureDrawSession::CheckIdleHoldTimeout() {
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

  // Re-evaluated on every call mid-drag, so the live preview matches what
  // `Finish` will resolve at release.
  const DirectionalGestureStrokes strokes =
      ClassifyResampledDirectionalGestureStrokes();
  const GestureMatchInfo gesture_match =
      MatchGesture(*smart_gesture_mapping_, smart_gesture_content_,
                   *directional_gesture_mapping_, *custom_gesture_recognizer_,
                   strokes, drawn_gesture_path_);

  std::u16string label;
  switch (gesture_match.match_type) {
    case GestureMatchType::kDirectional:
      label =
          base::UTF8ToUTF16(ToDirectionalGestureStrokeGlyph(strokes.back())) +
          u" " + ToGestureActionsDisplayName(gesture_match.actions);
      break;
    case GestureMatchType::kCustom:
    case GestureMatchType::kSmart:
      label = ToGestureActionsDisplayName(gesture_match.actions);
      break;
    case GestureMatchType::kUnmatched:
      label = l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_UNKNOWN_GESTURE);
      break;
  }
  gesture_widget_->UpdateTrail(should_show_trail, drawn_gesture_path_, label);
}

DirectionalGestureStrokes
GestureDrawSession::ClassifyResampledDirectionalGestureStrokes() const {
  const double min_stroke_pixels =
      prefs_->GetDouble(prefs::kDirectionalGestureSensitivityInPixels);
  const GesturePath resampled_gesture_path = ResampleGesturePathAtInterval(
      drawn_gesture_path_,
      base::saturated_cast<float>(min_stroke_pixels / 3.0));
  return ClassifyDirectionalGestureStrokes(
      resampled_gesture_path, min_stroke_pixels,
      features::kDirectionalGestureMinStrokeStraightness.Get(),
      features::kMaxDirectionalGestureStrokes.Get());
}

}  // namespace gestures
