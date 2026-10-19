// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_overlay_widget.h"

#include <utility>

#include "base/check_deref.h"
#include "brave/components/gestures/core/common/smoothed_path_util.h"
#include "cc/paint/paint_flags.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace gestures {

namespace {

constexpr SkColor kTrailColor = SkColorSetRGB(0xFF, 0x1F, 0x01);
constexpr uint8_t kTrailAlpha = 180;
constexpr SkColor kLabelColor = SK_ColorWHITE;
constexpr SkColor kLabelBackgroundColor = SkColorSetARGB(200, 32, 32, 32);
constexpr float kTrailStrokeWidth = 6.0F;
constexpr int kLabelHorizontalPadding = 10;
constexpr int kLabelVerticalPadding = 6;
constexpr int kLabelCornerRadius = 6;
constexpr int kLabelOffsetBelowCursor = 24;

}  // namespace

class GestureTrailView final : public views::View {
  METADATA_HEADER(GestureTrailView, views::View)

 public:
  GestureTrailView() { SetCanProcessEventsWithinSubtree(false); }

  GestureTrailView(const GestureTrailView&) = delete;
  GestureTrailView& operator=(const GestureTrailView&) = delete;

  ~GestureTrailView() override = default;

  void SetTrail(const std::vector<gfx::PointF>& path,
                bool show_trail,
                const std::u16string& label) {
    path_ = path;
    show_trail_ = show_trail;
    label_ = label;
    SchedulePaint();
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    View::OnPaint(canvas);

    cc::PaintFlags trail_paint_flags;
    trail_paint_flags.setColor(kTrailColor);
    trail_paint_flags.setStrokeWidth(kTrailStrokeWidth);
    trail_paint_flags.setStyle(cc::PaintFlags::kStroke_Style);
    trail_paint_flags.setStrokeCap(cc::PaintFlags::kRound_Cap);
    trail_paint_flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
    trail_paint_flags.setAntiAlias(true);

    // Drawn as a single path rather than one DrawLine() per segment: with many
    // closely-spaced points, a round cap at every segment's endpoint shows up
    // as a visible bead at each joint instead of a smooth line.
    //
    // The path is stroked fully opaque into its own layer, then that layer is
    // composited at `kTrailAlpha`: a self-crossing gesture would otherwise
    // double up the translucent stroke's alpha wherever it overlaps itself,
    // showing as a visibly darker blotch.
    if (show_trail_ && path_.size() >= 2) {
      canvas->SaveLayerAlpha(kTrailAlpha);
      canvas->DrawPath(SmoothedPath(path_), trail_paint_flags);
      canvas->Restore();
    }

    if (label_.empty() || path_.empty()) {
      return;
    }

    const gfx::FontList font_list;
    const int string_width = gfx::Canvas::GetStringWidth(label_, font_list);
    const gfx::Size label_size(
        string_width + kLabelHorizontalPadding * 2,
        font_list.GetHeight() + kLabelVerticalPadding * 2);
    // Centered on the cursor's x, offset further below it than the trail's
    // own width so the label doesn't sit directly under the cursor tip.
    const gfx::Point cursor = gfx::ToRoundedPoint(path_.back());
    const gfx::Point label_origin(cursor.x() - label_size.width() / 2,
                                  cursor.y() + kLabelOffsetBelowCursor);
    const gfx::Rect label_bounds(label_origin, label_size);

    cc::PaintFlags label_background_flags;
    label_background_flags.setColor(kLabelBackgroundColor);
    label_background_flags.setAntiAlias(true);
    canvas->DrawRoundRect(label_bounds, kLabelCornerRadius,
                          label_background_flags);
    canvas->DrawStringRectWithFlags(label_, font_list, kLabelColor,
                                    label_bounds,
                                    gfx::Canvas::TEXT_ALIGN_CENTER);
  }

 private:
  std::vector<gfx::PointF> path_;
  bool show_trail_ = false;
  std::u16string label_;
};

BEGIN_METADATA(GestureTrailView)
END_METADATA

GestureOverlayWidget::GestureOverlayWidget(content::WebContents* web_contents)
    : web_contents_(CHECK_DEREF(web_contents)) {}

GestureOverlayWidget::~GestureOverlayWidget() = default;

void GestureOverlayWidget::Show() {
  if (!widget_) {
    widget_ = std::make_unique<views::Widget>();
    views::Widget::InitParams params(
        views::Widget::InitParams::CLIENT_OWNS_WIDGET,
        views::Widget::InitParams::TYPE_POPUP);
    params.z_order = ui::ZOrderLevel::kFloatingUIElement;
    params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
    params.accept_events = false;
    params.activatable = views::Widget::InitParams::Activatable::kNo;
    params.bounds = web_contents_->GetContainerBounds();
    params.context = web_contents_->GetTopLevelNativeWindow();
    widget_->Init(std::move(params));
    trail_view_ =
        widget_->SetContentsView(std::make_unique<GestureTrailView>());
  }
  widget_->SetBounds(web_contents_->GetContainerBounds());
  widget_->ShowInactive();
}

bool GestureOverlayWidget::IsShown() const {
  return widget_ && widget_->IsVisible();
}

void GestureOverlayWidget::Hide() {
  if (widget_) {
    widget_->Hide();
  }
}

void GestureOverlayWidget::UpdateTrail(const std::vector<gfx::PointF>& path,
                                       bool show_trail,
                                       const std::u16string& label) {
  if (trail_view_) {
    trail_view_->SetTrail(path, show_trail, label);
  }
}

}  // namespace gestures
