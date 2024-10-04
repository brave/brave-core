/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/overlay/brave_video_overlay_window_views.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/views/overlay/brave_back_to_tab_label_button.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/overlay/close_image_button.h"
#include "chrome/browser/ui/views/overlay/constants.h"
#include "chrome/browser/ui/views/overlay/hang_up_button.h"
#include "chrome/browser/ui/views/overlay/playback_image_button.h"
#include "chrome/browser/ui/views/overlay/simple_overlay_window_image_button.h"
#include "chrome/browser/ui/views/overlay/toggle_camera_button.h"
#include "chrome/browser/ui/views/overlay/toggle_microphone_button.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/hit_test.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"

namespace {

constexpr int kTopControlIconSize = 20;

std::u16string ToString(base::TimeDelta time) {
  const int time_in_seconds = time.InSecondsF();
  const int hours = time_in_seconds / 3600;
  const int minutes = (time_in_seconds % 3600) / 60;
  const int seconds = (time_in_seconds % 60);

  return base::ASCIIToUTF16(
      hours ? base::StringPrintf("%02d:%02d:%02d", hours, minutes, seconds)
            : base::StringPrintf("%02d:%02d", minutes, seconds));
}

std::u16string ToString(const media_session::MediaPosition& position) {
  return base::StrCat({ToString(position.GetPosition()), u" / ",
                       ToString(position.duration())});
}

class Seeker : public views::Slider,
               public views::SliderListener,
               public views::ViewTargeterDelegate {
  METADATA_HEADER(Seeker, views::Slider)
 public:
  static constexpr int kThumbRadius = 6;
  static constexpr int kPreferredHeight = kThumbRadius * 2;
  static constexpr int kLineHeight = 4;

  explicit Seeker(views::SliderListener* listener)
      : Slider(this), listener_(listener) {
    SetValueIndicatorRadius(0);
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
    layer()->SetName("Seeker");
    SetRenderingStyle(RenderingStyle::kMinimalStyle);
    SetPreferredSize(gfx::Size(kPreferredHeight, kPreferredHeight));
    thumb_animation_.SetSlideDuration(base::Milliseconds(150));

    SetEventTargeter(std::make_unique<views::ViewTargeter>(this));
  }
  ~Seeker() override = default;

  // views::Slider:
  void OnPaint(gfx::Canvas* canvas) override {
    auto* colors = GetColorProvider();
    // Paint the background for progress line
    cc::PaintFlags flags;
    flags.setColor(colors->GetColor(nala::kColorWhite));
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAlphaf(0.4f);
    auto line_bounds = gfx::RectF(GetLocalBounds());
    line_bounds.Inset(
        gfx::InsetsF::VH((kPreferredHeight - kLineHeight) / 2, 0));
    canvas->DrawRect(line_bounds, flags);

    // Paint the progress line
    flags.setColor(colors->GetColor(nala::kColorPrimitivePrimary40));
    line_bounds.set_width(line_bounds.width() * GetAnimatingValue());
    flags.setAlphaf(1.0f);
    canvas->DrawRect(line_bounds, flags);

    if (ShouldShowThumb() || thumb_animation_.is_animating()) {
      // Paint the thumb button only when user is interacting with this seeker.
      canvas->DrawCircle(
          gfx::Point(line_bounds.right(), line_bounds.CenterPoint().y() - 1),
          thumb_animation_.is_animating()
              ? thumb_animation_.CurrentValueBetween(kLineHeight / 2,
                                                     kThumbRadius)
              : kThumbRadius,
          flags);
    }
  }

  void OnMouseEntered(const ui::MouseEvent& event) override {
    Slider::OnMouseEntered(event);
    thumb_animation_.Show();
  }

  void OnMouseExited(const ui::MouseEvent& event) override {
    Slider::OnMouseExited(event);
    if (!ShouldShowThumb()) {
      thumb_animation_.Hide();
    }
  }

  void AnimationProgressed(const gfx::Animation* animation) override {
    if (animation == &thumb_animation_) {
      SchedulePaint();
      return;
    }

    Slider::AnimationProgressed(animation);
  }

  void AnimationEnded(const gfx::Animation* animation) override {
    if (animation == &thumb_animation_) {
      return;
    }
    Slider::AnimationEnded(animation);
  }

  // views::SliderListener:
  void SliderValueChanged(views::Slider* sender,
                          float value,
                          float old_value,
                          views::SliderChangeReason reason) override {
    listener_->SliderValueChanged(sender, value, old_value, reason);
  }

  void SliderDragStarted(views::Slider* sender) override {
    dragging_ = true;
    listener_->SliderDragStarted(sender);
  }

  void SliderDragEnded(views::Slider* sender) override {
    dragging_ = false;
    if (!ShouldShowThumb()) {
      thumb_animation_.Hide();
    }
    listener_->SliderDragEnded(sender);
  }

  // views::ViewTargeterDelegate:
  bool DoesIntersectRect(const views::View* target,
                         const gfx::Rect& rect) const override {
    if (!GetEnabled() || !IsDrawn()) {
      return false;
    }

    // Exclude resize area for the window from hit test.
    // Note that we're using half of the width of resize area specified in
    // video_overlay_window_views.cc for corners.
    constexpr int kResizeAreaWidth = 8;
    auto seeker_bounds = GetLocalBounds();
    seeker_bounds.Inset(gfx::Insets::TLBR(0, kResizeAreaWidth,
                                          (kPreferredHeight - kLineHeight) / 2,
                                          kResizeAreaWidth));
    return target == this && rect.Intersects(seeker_bounds);
  }

 private:
  bool ShouldShowThumb() const {
    return GetEnabled() && (dragging_ || IsMouseHovered());
  }

  raw_ptr<views::SliderListener> listener_ = nullptr;

  bool dragging_ = false;

  gfx::SlideAnimation thumb_animation_{this};
};

BEGIN_METADATA(Seeker)
END_METADATA

}  // namespace

BraveVideoOverlayWindowViews::BraveVideoOverlayWindowViews(
    content::VideoPictureInPictureWindowController* controller)
    : VideoOverlayWindowViews(controller) {}

BraveVideoOverlayWindowViews::~BraveVideoOverlayWindowViews() = default;

void BraveVideoOverlayWindowViews::SetUpViews() {
  VideoOverlayWindowViews::SetUpViews();
  // Use CloseImageButton in order to use the same style with the close button
  fullscreen_button_ =
      controls_container_view_->AddChildView(std::make_unique<CloseImageButton>(
          base::BindRepeating(&BraveVideoOverlayWindowViews::RequestFullscreen,
                              base::Unretained(this))));
  const std::u16string fullscreen_button_label =
      l10n_util::GetStringUTF16(IDS_ACCNAME_FULLSCREEN);
  fullscreen_button_->SetTooltipText(fullscreen_button_label);
  fullscreen_button_->SetAccessibleName(fullscreen_button_label);
  fullscreen_button_->SetPaintToLayer(ui::LAYER_TEXTURED);
  fullscreen_button_->layer()->SetFillsBoundsOpaquely(false);
  fullscreen_button_->layer()->SetName("FullscreenButton");

  timestamp_ =
      controls_container_view_->AddChildView(std::make_unique<views::Label>());
  timestamp_->SetEnabledColorId(kColorPipWindowForeground);
  timestamp_->SetSubpixelRenderingEnabled(false);
  timestamp_->SetAutoColorReadabilityEnabled(false);
  timestamp_->SetElideBehavior(gfx::NO_ELIDE);
  timestamp_->SetPaintToLayer(ui::LAYER_TEXTURED);
  timestamp_->layer()->SetFillsBoundsOpaquely(false);
  timestamp_->layer()->SetName("Timestamp");

  seeker_ =
      controls_container_view_->AddChildView(std::make_unique<Seeker>(this));

  // Before we get the media position, we should hide timestamp and seeker.
  timestamp_->SetVisible(false);
  seeker_->SetVisible(false);

  UpdateControlIcons();
}

void BraveVideoOverlayWindowViews::OnUpdateControlsBounds() {
  VideoOverlayWindowViews::OnUpdateControlsBounds();

  // Copies size in order to avoid ASAN failure
  // https://github.com/brave/internal/issues/1108
  const auto window_size = GetBounds().size();

  // Lay out controls on top of window
  constexpr int kTopControlSpacing = 16;
  const auto close_button_insets = close_controls_view_->GetInsets();
  const gfx::Size close_button_size(
      kTopControlIconSize + close_button_insets.width(),
      kTopControlIconSize + close_button_insets.height());
  // Upcasting in order to call a base class's SetPosition() which is hidden by
  // the CloseImageButton::SetPosition().
  close_controls_view_->SetSize(close_button_size);
  views::AsViewClass<OverlayWindowImageButton>(close_controls_view_.get())
      ->SetPosition({window_size.width() -
                         (kTopControlSpacing - close_button_insets.right()) -
                         close_controls_view_->width(),
                     kTopControlSpacing - close_button_insets.top()});

  fullscreen_button_->SetSize(close_button_size);
  fullscreen_button_->SetPosition(
      {close_controls_view_->origin().x() -
           (kTopControlSpacing - close_button_insets.left() -
            fullscreen_button_->GetInsets().right()) -
           fullscreen_button_->size().width(),
       close_controls_view_->origin().y()});

  if (back_to_tab_label_button_) {
    back_to_tab_label_button_->SetMinSize(close_button_size);
    back_to_tab_label_button_->SetMaxSize(close_button_size);
    back_to_tab_label_button_->SetSize(close_button_size);
    back_to_tab_label_button_->SetPosition(
        {fullscreen_button_->origin().x() -
             (kTopControlSpacing - close_button_insets.left() -
              back_to_tab_label_button_->GetInsets().right()) -
             back_to_tab_label_button_->size().width(),
         close_controls_view_->origin().y()});
  }

  // Lay out controls in the middle of window
  std::vector<views::View*> visible_controls;
  if (previous_track_controls_view_->GetVisible()) {
    visible_controls.push_back(previous_track_controls_view_.get());
  }
  if (play_pause_controls_view_->GetVisible()) {
    visible_controls.push_back(play_pause_controls_view_.get());
  }
  if (next_track_controls_view_->GetVisible()) {
    visible_controls.push_back(next_track_controls_view_.get());
  }
  if (previous_slide_controls_view_->GetVisible()) {
    visible_controls.push_back(previous_slide_controls_view_.get());
  }
  if (next_slide_controls_view_->GetVisible()) {
    visible_controls.push_back(next_slide_controls_view_.get());
  }
  if (toggle_microphone_button_->GetVisible()) {
    visible_controls.push_back(toggle_microphone_button_.get());
  }
  if (toggle_camera_button_->GetVisible()) {
    visible_controls.push_back(toggle_camera_button_.get());
  }
  if (hang_up_button_->GetVisible()) {
    visible_controls.push_back(hang_up_button_.get());
  }

  constexpr int kCenterControlIconSize = 32;
  constexpr int kCenterControlSize =
      kCenterControlIconSize + kPipWindowIconPadding * 2;
  constexpr int kCenterControlSpacing = 24 - kPipWindowIconPadding * 2;

  const auto visible_controls_size = visible_controls.size();
  int x = (window_size.width() - visible_controls_size * kCenterControlSize -
           kCenterControlSpacing * (visible_controls_size - 1)) /
          2;
  const int y = (window_size.height() - kCenterControlSize) / 2;
  for (size_t i = 0; i < visible_controls_size; i++) {
    visible_controls[i]->SetBounds(x, y, kCenterControlSize,
                                   kCenterControlSize);
    x += kCenterControlSize + kCenterControlSpacing;
  }

  // Layout our own controls: timestamp and seeker
  const auto slider_height = seeker_->GetPreferredSize().height();
  seeker_->SetBounds(0, window_size.height() - slider_height,
                     window_size.width(), slider_height);

  UpdateTimestampPosition();
}

void BraveVideoOverlayWindowViews::UpdateControlIcons() {
  close_controls_view_->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoCloseIcon, kColorPipWindowForeground,
                                     kTopControlIconSize));
  fullscreen_button_->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoFullscreenOnIcon,
                                     kColorPipWindowForeground,
                                     kTopControlIconSize));

  if (back_to_tab_label_button_) {
    back_to_tab_label_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(kLeoPictureInPictureReturnIcon,
                                       kColorPipWindowForeground,
                                       kTopControlIconSize));
    auto text = back_to_tab_label_button_->GetText();
    // Calling this will clear accessible name as well. We should reset it and
    // tooltip text.
    back_to_tab_label_button_->SetText({});

    back_to_tab_label_button_->SetAccessibleName(text);
    back_to_tab_label_button_->SetTooltipText(
        back_to_tab_label_button_->GetAccessibleName());
  }

  previous_track_controls_view_->override_icon(kLeoPreviousOutlineIcon);
  next_track_controls_view_->override_icon(kLeoNextOutlineIcon);
}

void BraveVideoOverlayWindowViews::SetMediaPosition(
    const std::optional<media_session::MediaPosition>& media_position) {
  CHECK(timestamp_);

  media_position_ = media_position;

  timestamp_update_timer_.Stop();
  UpdateTimestampPeriodically();
  UpdateTimestampPosition();
}

void BraveVideoOverlayWindowViews::SetPlaybackState(
    PlaybackState playback_state) {
  VideoOverlayWindowViews::SetPlaybackState(playback_state);
  playback_state_ = playback_state;
  if (playback_state != PlaybackState::kPlaying) {
    timestamp_update_timer_.Stop();
  }
}

bool BraveVideoOverlayWindowViews::ControlsHitTestContainsPoint(
    const gfx::Point& point) {
  gfx::Point point_in_seeker = views::View::ConvertPointToTarget(
      seeker_->parent(), seeker_.get(), point);
  if (seeker_->HitTestPoint(point_in_seeker)) {
    return true;
  }

  if (fullscreen_button_->GetMirroredBounds().Contains(point)) {
    return true;
  }

  return VideoOverlayWindowViews::ControlsHitTestContainsPoint(point);
}

void BraveVideoOverlayWindowViews::SetSeekerEnabled(bool enabled) {
  seeker_->SetEnabled(enabled);
}

void BraveVideoOverlayWindowViews::ShowInactive() {
  VideoOverlayWindowViews::ShowInactive();
  UpdateTimestampPeriodically();
}

void BraveVideoOverlayWindowViews::Close() {
  timestamp_update_timer_.Stop();
  VideoOverlayWindowViews::Close();
}

void BraveVideoOverlayWindowViews::Hide() {
  timestamp_update_timer_.Stop();
  VideoOverlayWindowViews::Hide();
}

int BraveVideoOverlayWindowViews::GetNonClientComponent(
    const gfx::Point& point) {
  if (seeker_ && seeker_->GetWidget() == this) {
    gfx::Point point_in_seeker(point);
    views::View::ConvertPointFromWidget(seeker_.get(), &point_in_seeker);
    if (seeker_->HitTestPoint(point_in_seeker)) {
      // We want to handel mouse event on seeker when it's visible, rather than
      // consider it as non-client area
      return HTCLIENT;
    }
  }

  return VideoOverlayWindowViews::GetNonClientComponent(point);
}

void BraveVideoOverlayWindowViews::OnKeyEvent(ui::KeyEvent* event) {
  if (event->type() == ui::EventType::kKeyPressed && media_position_) {
    if (event->key_code() == ui::VKEY_LEFT) {
      controller_->SeekTo(media_position_->GetPosition() - base::Seconds(10));
    } else if (event->key_code() == ui::VKEY_RIGHT) {
      controller_->SeekTo(media_position_->GetPosition() + base::Seconds(10));
    }
    event->SetHandled();
    return;
  }

  VideoOverlayWindowViews::OnKeyEvent(event);
}

void BraveVideoOverlayWindowViews::SliderValueChanged(
    views::Slider* sender,
    float value,
    float old_value,
    views::SliderChangeReason reason) {
  if (reason == views::SliderChangeReason::kByApi) {
    return;
  }

  if (!media_position_) {
    return;
  }

  controller_->SeekTo(media_position_->duration() * value);
}

void BraveVideoOverlayWindowViews::SliderDragStarted(views::Slider* sender) {
  timestamp_update_timer_.Stop();
  is_seeking_ = true;

  if (was_playing_before_seeking_ = playback_state_ == PlaybackState::kPlaying;
      was_playing_before_seeking_) {
    controller_->TogglePlayPause();
  }
}

void BraveVideoOverlayWindowViews::SliderDragEnded(views::Slider* sender) {
  is_seeking_ = false;
  UpdateTimestampPeriodically();
  if (was_playing_before_seeking_ &&
      playback_state_ == PlaybackState::kPaused) {
    controller_->TogglePlayPause();
  }
}

void BraveVideoOverlayWindowViews::UpdateTimestampPosition() {
  CHECK(timestamp_);

  timestamp_->SetVisible(media_position_.has_value());
  if (!timestamp_->GetVisible()) {
    return;
  }

  timestamp_->SetPosition(
      {kPipWindowIconPadding, GetBounds().size().height() -
                                  timestamp_->GetPreferredSize().height() -
                                  seeker_->height()});
}

void BraveVideoOverlayWindowViews::UpdateTimestampPeriodically() {
  // Update timestamp related UI controls

  // We don't need to show seeker and timestamp when the duration is less than
  // a second, infinite or zero.
  if (media_position_ &&
      (!media_position_->duration().is_inf() &&
       !media_position_->duration().is_zero() &&
       static_cast<int>(media_position_->duration().InSecondsF()) > 0)) {
    auto new_time = ToString(*media_position_);
    if (new_time != timestamp_->GetText()) {
      timestamp_->SetText(new_time);
      timestamp_->SizeToPreferredSize();
    }

    if (!is_seeking_) {
      seeker_->SetValue(media_position_->GetPosition().InSecondsF() /
                        media_position_->duration().InSecondsF());
    }

    if (!seeker_->GetVisible()) {
      seeker_->SetVisible(true);
    }
  } else {
    timestamp_->SetText({});
    seeker_->SetValue(0);
    seeker_->SetVisible(false);
  }

  // Update repeating timer state.
  const bool should_update_timestamp_periodically =
      media_position_ && IsVisible() && !is_seeking_ &&
      playback_state_ == PlaybackState::kPlaying;

  if (should_update_timestamp_periodically ==
      timestamp_update_timer_.IsRunning()) {
    return;
  }

  if (should_update_timestamp_periodically) {
    // 350 is the value defined by standard for progress event. This value
    // would be good for updating timestamp too.
    // https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/core/html/media/html_media_element.cc;l=1838
    timestamp_update_timer_.Start(
        FROM_HERE, base::Milliseconds(350),
        base::BindRepeating(
            &BraveVideoOverlayWindowViews::UpdateTimestampPeriodically,
            base::Unretained(this)));
  } else {
    timestamp_update_timer_.Stop();
  }
}

void BraveVideoOverlayWindowViews::RequestFullscreen() {
  controller_->RequestFullscreen();
}
