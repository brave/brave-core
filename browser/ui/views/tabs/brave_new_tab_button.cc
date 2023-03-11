/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "cc/paint/paint_flags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/accelerator_table.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/controls/label.h"

// static
const gfx::Size BraveNewTabButton::kButtonSize{24, 24};

constexpr int kInsetForVerticalTab = 22;

// static
SkPath BraveNewTabButton::GetBorderPath(const gfx::Point& origin,
                                        float scale,
                                        bool extend_to_top,
                                        int corner_radius,
                                        const gfx::Size& contents_bounds) {
  // Overriden to use Brave's non-circular shape
  gfx::PointF scaled_origin(origin);
  scaled_origin.Scale(scale);
  const float radius = corner_radius * scale;

  SkPath path;
  const gfx::Rect path_rect(
      scaled_origin.x(), extend_to_top ? 0 : scaled_origin.y(),
      contents_bounds.width() * scale,
      (extend_to_top ? scaled_origin.y() : 0) +
          std::min(contents_bounds.width(), contents_bounds.height()) * scale);
  path.addRoundRect(RectToSkRect(path_rect), radius, radius);
  path.close();
  return path;
}

gfx::Size BraveNewTabButton::CalculatePreferredSize() const {
  // Overriden so that we use Brave's custom button size
  gfx::Size size = kButtonSize;
  const auto insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs) &&
      tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {
    size.set_height(kHeightForVerticalTabs);
  }

  return size;
}

SkPath BraveNewTabButton::GetBorderPath(const gfx::Point& origin,
                                        float scale,
                                        bool extend_to_top) const {
  return GetBorderPath(origin, scale, extend_to_top, GetCornerRadius(),
                       GetContentsBounds().size());
}

BraveNewTabButton::BraveNewTabButton(TabStrip* tab_strip,
                                     PressedCallback callback)
    : NewTabButton(tab_strip, std::move(callback)) {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return;

  text_ = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_ACCNAME_NEWTAB)));
  text_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
  text_->SetVerticalAlignment(gfx::VerticalAlignment::ALIGN_MIDDLE);
  text_->SetVisible(false);

  shortcut_text_ = AddChildView(std::make_unique<views::Label>());
  shortcut_text_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_RIGHT);
  shortcut_text_->SetVerticalAlignment(gfx::VerticalAlignment::ALIGN_MIDDLE);
  shortcut_text_->SetVisible(false);
}

BraveNewTabButton::~BraveNewTabButton() = default;

void BraveNewTabButton::SetShortcutText(const std::u16string& text) {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs));
  DCHECK(shortcut_text_);
  shortcut_text_->SetText(text);
}

void BraveNewTabButton::PaintIcon(gfx::Canvas* canvas) {
  gfx::ScopedCanvas scoped_canvas(canvas);
  // Incorrect offset that base class will use
  const int chromium_offset = GetCornerRadius();
  // Shim base implementation's painting
  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs) &&
      tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {
    DCHECK(text_);
    const bool should_align_icon_center = text_->bounds().IsEmpty();
    auto contents_bounds = GetContentsBounds();

    // Offset that we want to use
    const int correct_h_offset = should_align_icon_center
                                     ? (contents_bounds.width() / 2)
                                     : kInsetForVerticalTab;
    const int correct_v_offset = (contents_bounds.height() / 2);
    // Difference
    const int h_offset = correct_h_offset - chromium_offset;
    const int v_offset = correct_v_offset - chromium_offset;
    canvas->Translate(gfx::Vector2d(h_offset, v_offset));
  } else {
    // Overriden to fix chromium assumption that border radius
    // will be 50% of width.
    // Offset that we want to use
    const int correct_h_offset = (GetContentsBounds().width() / 2);

    // Difference
    const int h_offset = correct_h_offset - chromium_offset;
    canvas->Translate(gfx::Vector2d(h_offset, h_offset));
  }

  NewTabButton::PaintIcon(canvas);
}

gfx::Insets BraveNewTabButton::GetInsets() const {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs) &&
      tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {
    return {};
  }

  // Give an additional left margin to make more space from tab.
  // TabStripRegionView::UpdateNewTabButtonBorder() gives this button's inset.
  // So, adding more insets here is easy solution.
  return NewTabButton::GetInsets() + gfx::Insets::TLBR(0, 6, 0, 0);
}

void BraveNewTabButton::PaintFill(gfx::Canvas* canvas) const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    NewTabButton::PaintFill(canvas);
    return;
  }

  if (!tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {
    NewTabButton::PaintFill(canvas);
    return;
  }

  if (tab_strip_->GetCustomBackgroundId(BrowserFrameActiveState::kUseCurrent)
          .has_value()) {
    NewTabButton::PaintFill(canvas);
    return;
  }

  auto* cp = GetColorProvider();
  DCHECK(cp);

  // Override fill color
  {
    gfx::ScopedCanvas scoped_canvas(canvas);
    canvas->UndoDeviceScaleFactor();
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(cp->GetColor(kColorToolbar));
    canvas->DrawPath(GetBorderPath(gfx::Point(), canvas->image_scale(), false),
                     flags);
  }

  // Draw split line on the top
  gfx::Rect separator_bounds = GetLocalBounds();
  separator_bounds.set_height(1);
  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(cp->GetColor(kColorBraveVerticalTabSeparator));
  canvas->DrawRect(gfx::RectF(separator_bounds), flags);
}

void BraveNewTabButton::FrameColorsChanged() {
  NewTabButton::FrameColorsChanged();
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return;

  DCHECK(text_ && shortcut_text_);
  auto text_color = GetForegroundColor();
  text_->SetEnabledColor(text_color);
  shortcut_text_->SetEnabledColor(text_color);
}

void BraveNewTabButton::Layout() {
  NewTabButton::Layout();
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return;

  DCHECK(text_ && shortcut_text_);
  const bool show_vertical_tabs =
      tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser());
  text_->SetVisible(show_vertical_tabs);
  shortcut_text_->SetVisible(show_vertical_tabs);
  if (!text_->GetVisible())
    return;

  constexpr int kTextLeftMargin = 20 + kInsetForVerticalTab;
  gfx::Rect text_bounds = GetLocalBounds();
  text_bounds.Inset(
      gfx::Insets().set_left(kTextLeftMargin).set_right(kInsetForVerticalTab));
  text_->SetBoundsRect(text_bounds);
  shortcut_text_->SetBoundsRect(text_bounds);
}
