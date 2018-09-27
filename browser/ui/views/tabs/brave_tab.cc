/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/tabs/tab_controller.h"
#include "ui/gfx/animation/animation_container.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skia_paint_util.h"

using namespace chrome_browser_ui_views_tabs_tab_cc;

BraveTab::BraveTab(TabController* controller, gfx::AnimationContainer* container)
          : Tab::Tab(controller, container) {
  // Reserve more space than parent class since
  // Brave tab strip starts at the top, to allow for more shadow
  SetBorder(views::CreateEmptyBorder(GetContentsInsets()));
}

gfx::Insets BraveTab::GetContentsInsets() const {
  gfx::Insets tab_contents_insets(Tab::GetContentsInsets());
  // inset top to reserve space for shadow / drag area
  return tab_contents_insets + gfx::Insets(
                                BrowserNonClientFrameView::kMinimumDragHeight,
                                0,
                                0,
                                0);
}

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

void BraveTab::PaintTabShadows(gfx::Canvas* canvas, const gfx::Path& fill_path) {
  const bool active = IsActive();
  const bool hover = !active && hover_controller_.ShouldDraw();
  if (active || hover) {
    gfx::ScopedCanvas scoped_canvas(canvas);
    const float scale = canvas->UndoDeviceScaleFactor();
    canvas->sk_canvas()->clipPath(fill_path, SkClipOp::kDifference, true);
    std::vector<gfx::ShadowValue> shadows;
    double alpha = 100;
    double blur = 6;
    if (hover) {
      double animation_value = hover_controller_.GetAnimationValue();
      alpha = animation_value * (alpha * 0.66);
      blur = animation_value * blur;
    }
    gfx::ShadowValue shadow(
        gfx::Vector2d(0, 0), blur * scale,
        SkColorSetARGB(alpha, 0x00, 0x00, 0x00));
    shadows.push_back(shadow.Scale(scale));

    cc::PaintFlags shadow_flags;
    shadow_flags.setLooper(gfx::CreateShadowDrawLooper(shadows));
    shadow_flags.setColor(SK_ColorTRANSPARENT);
    canvas->DrawPath(fill_path, shadow_flags);
  }
}

void BraveTab::PaintTabBackgroundFill(gfx::Canvas* canvas,
                                const gfx::Path& fill_path,
                                bool active,
                                bool paint_hover_effect,
                                SkColor active_color,
                                SkColor inactive_color,
                                int fill_id,
                                int y_inset) {
  // Insert shadow painting
  PaintTabShadows(canvas, fill_path);
  Tab::PaintTabBackgroundFill(canvas, fill_path, active, paint_hover_effect,
                              active_color, inactive_color, fill_id, y_inset);
}

void BraveTab::OnPaint(gfx::Canvas* canvas) {
  // apply inset but allow painting outside bounds, for shadow
  canvas->Translate(gfx::Vector2d(0, BrowserNonClientFrameView::kMinimumDragHeight));
  Tab::OnPaint(canvas);
}