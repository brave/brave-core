#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"

void BraveBrowserViewLayout::Layout(views::View* host) {
  BrowserViewLayout::Layout(host);
  // Extend the tab strip to start at the very top
  // so that it can draw shadows in the area which is pretending to
  // be non-client (i.e. draggable)
  if (tab_strip_->visible()) {
    gfx::Rect tabstrip_bounds(tab_strip_->bounds());
    // set initial position to top
    tabstrip_bounds.set_y(tabstrip_bounds.y() -
                          BrowserNonClientFrameView::kMinimumDragHeight);
    // increase height so that bottom is in same position
    tabstrip_bounds.set_height(tabstrip_bounds.height() +
                                BrowserNonClientFrameView::kMinimumDragHeight);
    tab_strip_->SetBoundsRect(tabstrip_bounds);
  }
}