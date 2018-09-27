#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "ui/gfx/canvas.h"

void BraveTabStrip::OnPaint(gfx::Canvas* canvas) {
  // BraveTabStrip extends to top: 0 of window, TabStrip does not,
  // so bump all TabStrip painting down a bit whilst still reserving
  // the space for tab shadows.
  canvas->Translate(gfx::Vector2d(0, BrowserNonClientFrameView::kMinimumDragHeight));
  TabStrip::OnPaint(canvas);
}

void BraveTabStrip::GenerateIdealBounds() {
  TabStrip::GenerateIdealBounds();
  new_tab_button_bounds_.set_y(BrowserNonClientFrameView::kMinimumDragHeight);
}

bool BraveTabStrip::IsPositionInWindowCaption(const gfx::Point& point) {
  // ignore shadow area
  return point.y() < BrowserNonClientFrameView::kMinimumDragHeight
      ? true
      : TabStrip::IsPositionInWindowCaption(point);
}