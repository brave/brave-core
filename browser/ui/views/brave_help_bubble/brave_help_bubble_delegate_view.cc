// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_delegate_view.h"

#include <array>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_border_arrow_utils.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/widget/widget_delegate.h"

using views::BubbleBorder;
using views::BubbleDialogDelegateView;
using views::BubbleFrameView;
using views::NonClientFrameView;

namespace {
constexpr SkColor kBgColor = SkColorSetARGB(0xFF, 0x20, 0x4A, 0xE3);

// This class paints a border with only an arrow, skipping the painting of
// shadow and border around the overall bounds.
class BorderWithArrow : public views::BubbleBorder {
 public:
  enum BubbleArrowPart { kFill, kBorder };

  explicit BorderWithArrow(Arrow arrow, ui::ColorId color_id)
      : views::BubbleBorder(arrow,
                            BubbleBorder::Shadow::STANDARD_SHADOW,
                            color_id) {
    set_visible_arrow(true);
  }

  BorderWithArrow(const BorderWithArrow&) = delete;
  BorderWithArrow& operator=(const BorderWithArrow&) = delete;
  ~BorderWithArrow() override = default;

  // views::BubbleBorder:
  void Paint(const views::View& view, gfx::Canvas* canvas) override {
    PaintVisibleArrow(view, canvas);
  }

 private:
  // We are copying this function from the upstream code because we only need to
  // paint an arrow.
  void PaintVisibleArrow(const views::View& view, gfx::Canvas* canvas) {
    // The BubbleBorder subclass provides access to the visible_arrow_rect_
    // member only through a test function.
    gfx::Point arrow_origin = GetVisibibleArrowRectForTesting().origin();

    views::View::ConvertPointFromScreen(&view, &arrow_origin);
    const gfx::Rect arrow_bounds(arrow_origin,
                                 GetVisibibleArrowRectForTesting().size());

    // Clip the canvas to a box that's big enough to hold the shadow in every
    // dimension but won't overlap the bubble itself.
    gfx::ScopedCanvas scoped(canvas);
    gfx::Rect clip_rect = arrow_bounds;
    const views::BubbleArrowSide side = GetBubbleArrowSide(arrow());
    clip_rect.Inset(
        gfx::Insets::TLBR(side == views::BubbleArrowSide::kBottom ? 0 : -2,
                          side == views::BubbleArrowSide::kRight ? 0 : -2,
                          side == views::BubbleArrowSide::kTop ? 0 : -2,
                          side == views::BubbleArrowSide::kLeft ? 0 : -2));
    canvas->ClipRect(clip_rect);

    cc::PaintFlags flags;
    flags.setStrokeCap(cc::PaintFlags::kRound_Cap);

    flags.setColor(
        view.GetColorProvider()->GetColor(ui::kColorBubbleBorderShadowLarge));
    flags.setStyle(cc::PaintFlags::kStroke_Style);
    flags.setStrokeWidth(1.2);
    flags.setAntiAlias(true);
    canvas->DrawPath(
        GetVisibleArrowPath(arrow(), arrow_bounds, BubbleArrowPart::kBorder),
        flags);

    flags.setColor(color());
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setStrokeWidth(1.0);
    flags.setAntiAlias(true);
    canvas->DrawPath(
        GetVisibleArrowPath(arrow(), arrow_bounds, BubbleArrowPart::kFill),
        flags);
  }

  SkPath GetVisibleArrowPath(BubbleBorder::Arrow arrow,
                             const gfx::Rect& bounds,
                             BubbleArrowPart part) {
    constexpr size_t kNumPoints = 4;
    gfx::RectF bounds_f(bounds);
    std::array<SkPoint, kNumPoints> points;
    switch (GetBubbleArrowSide(arrow)) {
      case views::BubbleArrowSide::kRight:
        points[0] = {bounds_f.x(), bounds_f.y()};
        points[1] = {bounds_f.right(),
                     bounds_f.y() + BubbleBorder::kVisibleArrowRadius - 1};
        points[2] = {bounds_f.right(),
                     bounds_f.y() + BubbleBorder::kVisibleArrowRadius};
        points[3] = {bounds_f.x(), bounds_f.bottom() - 1};
        break;
      case views::BubbleArrowSide::kLeft:
        points[0] = {bounds_f.right(), bounds_f.bottom() - 1};
        points[1] = {bounds_f.x(),
                     bounds_f.y() + BubbleBorder::kVisibleArrowRadius};
        points[2] = {bounds_f.x(),
                     bounds_f.y() + BubbleBorder::kVisibleArrowRadius - 1};
        points[3] = {bounds_f.right(), bounds_f.y()};
        break;
      case views::BubbleArrowSide::kTop:
        points[0] = {bounds_f.x(), bounds_f.bottom()};
        points[1] = {bounds_f.x() + BubbleBorder::kVisibleArrowRadius - 1,
                     bounds_f.y()};
        points[2] = {bounds_f.x() + BubbleBorder::kVisibleArrowRadius,
                     bounds_f.y()};
        points[3] = {bounds_f.right() - 1, bounds_f.bottom()};
        break;
      case views::BubbleArrowSide::kBottom:
        points[0] = {bounds_f.right() - 1, bounds_f.y()};
        points[1] = {bounds_f.x() + BubbleBorder::kVisibleArrowRadius,
                     bounds_f.bottom()};
        points[2] = {bounds_f.x() + BubbleBorder::kVisibleArrowRadius - 1,
                     bounds_f.bottom()};
        points[3] = {bounds_f.x(), bounds_f.y()};
        break;
    }

    return SkPath::Polygon(points.data(), kNumPoints,
                           part == BubbleArrowPart::kFill);
  }
};
}  // namespace

BraveHelpBubbleDelegateView::BraveHelpBubbleDelegateView(
    View* anchor_view,
    const std::string& text)
    : BubbleDialogDelegateView(anchor_view, BubbleBorder::Arrow::TOP_CENTER) {
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  set_shadow(BubbleBorder::Shadow::STANDARD_SHADOW);
  set_corner_radius(10);
  set_color(kBgColor);
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  views::Label* blocked_trackers_label =
      AddChildView(std::make_unique<views::Label>());
  blocked_trackers_label->SetBorder(
      views::CreateEmptyBorder(gfx::Insets::TLBR(10, 0, 8, 0)));
  SetUpLabel(blocked_trackers_label, base::UTF8ToUTF16(text), 16,
             gfx::Font::Weight::SEMIBOLD);

  views::Label* view_label = AddChildView(std::make_unique<views::Label>());
  SetUpLabel(view_label,
             l10n_util::GetStringUTF16(
                 IDS_BRAVE_SHIELDS_ONBOARDING_CLICK_TO_VIEW_LABEL),
             14, gfx::Font::Weight::NORMAL);

  AddChildView(view_label);
}

BraveHelpBubbleDelegateView::~BraveHelpBubbleDelegateView() = default;

std::unique_ptr<NonClientFrameView>
BraveHelpBubbleDelegateView::CreateNonClientFrameView(views::Widget* widget) {
  std::unique_ptr<NonClientFrameView> frame =
      BubbleDialogDelegateView::CreateNonClientFrameView(widget);
  CHECK(frame);

  std::unique_ptr<BorderWithArrow> border =
      std::make_unique<BorderWithArrow>(arrow(), color());
  border->SetColor(color());

  if (GetParams().round_corners) {
    border->SetCornerRadius(GetCornerRadius());
  }

  static_cast<BubbleFrameView*>(frame.get())
      ->SetBubbleBorder(std::move(border));
  return frame;
}

void BraveHelpBubbleDelegateView::SetUpLabel(views::Label* label,
                                             const std::u16string& text,
                                             int font_size,
                                             gfx::Font::Weight font_weight) {
  label->SetMultiLine(true);
  label->SetMaximumWidth(390);
  label->SetText(text);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColor(SK_ColorWHITE);

  const auto& font_list = label->font_list();
  label->SetFontList(
      font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
          .DeriveWithWeight(font_weight));

  label->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
}

BEGIN_METADATA(BraveHelpBubbleDelegateView)
END_METADATA
