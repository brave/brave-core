/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include "chrome/browser/ui/views/tabs/tab_controller.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/scoped_canvas.h"

using namespace chrome_browser_ui_views_tabs_tab_cc;

// Same as Chomium Tab implementation except separators are
// achored to bottom (instead of floating in middle) and rounded ends
void BraveTab::PaintSeparators(gfx::Canvas* canvas) {
  const auto separator_opacities = GetSeparatorOpacities(false);
  if (!separator_opacities.left && !separator_opacities.right)
    return;

  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  const gfx::RectF aligned_bounds =
      ScaleAndAlignBounds(bounds(), scale, GetStrokeThickness());
  const int corner_radius = GetCornerRadius();
  const float separator_height = GetTabSeparatorHeight() * scale;
  gfx::RectF leading_separator_bounds(
      aligned_bounds.x() + corner_radius * scale,
      aligned_bounds.y() + aligned_bounds.height() - separator_height,
      kSeparatorThickness * scale, separator_height);
  gfx::RectF trailing_separator_bounds = leading_separator_bounds;
  trailing_separator_bounds.set_x(
      aligned_bounds.right() - (corner_radius + kSeparatorThickness) * scale);

  gfx::PointF origin(bounds().origin());
  origin.Scale(scale);
  leading_separator_bounds.Offset(-origin.x(), -origin.y());
  trailing_separator_bounds.Offset(-origin.x(), -origin.y());

  const SkColor separator_base_color = controller_->GetTabSeparatorColor();
  const auto separator_color = [separator_base_color](float opacity) {
    return SkColorSetA(separator_base_color,
                       gfx::Tween::IntValueBetween(opacity, SK_AlphaTRANSPARENT,
                                                   SK_AlphaOPAQUE));
  };
  const int separator_radius = kSeparatorThickness / 2;
  const SkScalar scaled_separator_radius = SkIntToScalar(
                                                      separator_radius * scale);
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(separator_color(separator_opacities.left));
  canvas->DrawRoundRect(
                      leading_separator_bounds, scaled_separator_radius, flags);
  flags.setColor(separator_color(separator_opacities.right));
  canvas->DrawRoundRect(
                     trailing_separator_bounds, scaled_separator_radius, flags);
}
