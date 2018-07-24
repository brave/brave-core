/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"

#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_sign_in_delegate.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bubble_view.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "chrome/browser/ui/views/toolbar/home_button.h"
#include "chrome/browser/ui/views/toolbar/reload_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/prefs/pref_service.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "ui/base/material_design/material_design_controller.h"

namespace {

int GetToolbarHorizontalPadding() {
  // In the touch-optimized UI, we don't use any horizontal paddings; the back
  // button starts from the beginning of the view, and the app menu button ends
  // at the end of the view.
  return ui::MaterialDesignController::IsTouchOptimizedUiEnabled() ? 0 : 8;
}

}  // namespace

BraveToolbarView::BraveToolbarView(Browser* browser, BrowserView* browser_view)
    : ToolbarView(browser, browser_view) {
}

BraveToolbarView::~BraveToolbarView() {}

void BraveToolbarView::Init() {
  ToolbarView::Init();
  // track changes in bookmarks enabled setting
  edit_bookmarks_enabled_.Init(
      bookmarks::prefs::kEditBookmarksEnabled, browser()->profile()->GetPrefs(),
      base::Bind(&BraveToolbarView::OnEditBookmarksEnabledChanged,
                 base::Unretained(this)));

  // Only location bar in non-normal mode
  if (!is_display_mode_normal()) {
    return;
  }

  bookmark_ = new BookmarkButton(this);
  bookmark_->set_triggerable_event_flags(ui::EF_LEFT_MOUSE_BUTTON | ui::EF_MIDDLE_MOUSE_BUTTON);
  bookmark_->Init();
  AddChildView(bookmark_);
}

void BraveToolbarView::OnEditBookmarksEnabledChanged() {
  Update(nullptr);
}

void BraveToolbarView::Update(content::WebContents* tab) {
  ToolbarView::Update(tab);
  // Decide whether to show the bookmark button
  if (bookmark_) {
    bookmark_->SetVisible(browser_defaults::bookmarks_enabled &&
      edit_bookmarks_enabled_.GetValue());
  }
}

void BraveToolbarView::ShowBookmarkBubble(
    const GURL& url,
    bool already_bookmarked,
    bookmarks::BookmarkBubbleObserver* observer) {
  // Show BookmarkBubble attached to Brave's bookmark button
  // or the location bar if there is no bookmark button
  // (i.e. in non-normal display mode).
  views::View* anchor_view = location_bar();
  BookmarkButton* const star_view = bookmark_button();
  if (star_view && star_view->visible())
    anchor_view = star_view;

  std::unique_ptr<BubbleSyncPromoDelegate> delegate;
  delegate.reset(new BookmarkBubbleSignInDelegate(browser()));
  views::Widget* bubble_widget = BookmarkBubbleView::ShowBubble(
      anchor_view, gfx::Rect(), nullptr, observer, std::move(delegate),
      browser()->profile(), url, already_bookmarked);
  if (bubble_widget && star_view)
    star_view->OnBubbleWidgetCreated(bubble_widget);
}

void BraveToolbarView::Layout() {
  // If we have not been initialized yet just do nothing.
  if (!initialized_)
    return;

  if (!is_display_mode_normal()) {
    location_bar_->SetBounds(0, 0, width(),
                             location_bar_->GetPreferredSize().height());
    return;
  }

  // We assume all toolbar buttons except for the browser actions are the same
  // height. Set toolbar_button_y such that buttons appear vertically centered.
  const int toolbar_button_height =
      std::min(back_->GetPreferredSize().height(), height());
  const int toolbar_button_y = (height() - toolbar_button_height) / 2;

  // If the window is maximized, we extend the back button to the left so that
  // clicking on the left-most pixel will activate the back button.
  // TODO(abarth):  If the window becomes maximized but is not resized,
  //                then Layout() might not be called and the back button
  //                will be slightly the wrong size.  We should force a
  //                Layout() in this case.
  //                http://crbug.com/5540
  const bool maximized =
      browser_->window() && browser_->window()->IsMaximized();
  // The padding at either end of the toolbar.
  const int end_padding = GetToolbarHorizontalPadding();
  back_->SetLeadingMargin(maximized ? end_padding : 0);
  back_->SetBounds(maximized ? 0 : end_padding, toolbar_button_y,
                   back_->GetPreferredSize().width(), toolbar_button_height);
  const int element_padding = GetLayoutConstant(TOOLBAR_ELEMENT_PADDING);
  int next_element_x = back_->bounds().right() + element_padding;

  forward_->SetBounds(next_element_x, toolbar_button_y,
                      forward_->GetPreferredSize().width(),
                      toolbar_button_height);
  next_element_x = forward_->bounds().right() + element_padding;

  reload_->SetBounds(next_element_x, toolbar_button_y,
                     reload_->GetPreferredSize().width(),
                     toolbar_button_height);
  next_element_x = reload_->bounds().right();

  home_->SetSize(
      gfx::Size(home_->GetPreferredSize().width(), toolbar_button_height));
  if (show_home_button_.GetValue() ||
      (browser_->is_app() && extensions::util::IsNewBookmarkAppsEnabled())) {
    home_->SetVisible(true);
    next_element_x += element_padding;
    home_->SetPosition(gfx::Point(next_element_x, toolbar_button_y));
    next_element_x += home_->width();
  } else {
    home_->SetVisible(false);
  }

  // position Brave's BookmarkButton
  if (bookmark_ && bookmark_->visible()) {
    next_element_x += element_padding;
    bookmark_->SetBounds(next_element_x, toolbar_button_y, bookmark_->GetPreferredSize().width(), toolbar_button_height);
    next_element_x = bookmark_->bounds().right();
  }
  next_element_x += GetLayoutConstant(TOOLBAR_STANDARD_SPACING);

  int app_menu_width = app_menu_button_->GetPreferredSize().width();
  const int right_padding = GetLayoutConstant(TOOLBAR_STANDARD_SPACING);

  // Note that the browser actions container has its own internal left and right
  // padding to visually separate it from the location bar and app menu button.
  // However if the container is empty we must account for the |right_padding|
  // value used to visually separate the location bar and app menu button.
  int available_width = std::max(
      0,
      width() - end_padding - app_menu_width -
      (browser_actions_->GetPreferredSize().IsEmpty() ? right_padding : 0) -
      next_element_x);
  if (avatar_) {
    available_width -= avatar_->GetPreferredSize().width();
    available_width -= element_padding;
  }
  // Don't allow the omnibox to shrink to the point of non-existence, so
  // subtract its minimum width from the available width to reserve it.
  const int browser_actions_width = browser_actions_->GetWidthForMaxWidth(
      available_width - location_bar_->GetMinimumSize().width());
  available_width -= browser_actions_width;
  const int location_bar_width = available_width;

  const int location_height = location_bar_->GetPreferredSize().height();
  const int location_y = (height() - location_height) / 2;
  location_bar_->SetBounds(next_element_x, location_y,
                           location_bar_width, location_height);

  next_element_x = location_bar_->bounds().right();

  // Note height() may be zero in fullscreen.
  const int browser_actions_height =
      std::min(browser_actions_->GetPreferredSize().height(), height());
  const int browser_actions_y = (height() - browser_actions_height) / 2;
  browser_actions_->SetBounds(next_element_x, browser_actions_y,
                              browser_actions_width, browser_actions_height);
  next_element_x = browser_actions_->bounds().right();
  if (!browser_actions_width)
    next_element_x += right_padding;

  // The browser actions need to do a layout explicitly, because when an
  // extension is loaded/unloaded/changed, BrowserActionContainer removes and
  // re-adds everything, regardless of whether it has a page action. For a
  // page action, browser action bounds do not change, as a result of which
  // SetBounds does not do a layout at all.
  // TODO(sidchat): Rework the above behavior so that explicit layout is not
  //                required.
  browser_actions_->Layout();

  if (avatar_) {
    avatar_->SetBounds(next_element_x, toolbar_button_y,
                       avatar_->GetPreferredSize().width(),
                       toolbar_button_height);
    next_element_x = avatar_->bounds().right() + element_padding;
  }

  // Extend the app menu to the screen's right edge in maximized mode just like
  // we extend the back button to the left edge.
  if (maximized)
    app_menu_width += end_padding;

  // Set trailing margin before updating the bounds so OnBoundsChange can use
  // the trailing margin.
  app_menu_button_->SetTrailingMargin(maximized ? end_padding : 0);
  app_menu_button_->SetBounds(next_element_x, toolbar_button_y, app_menu_width,
                              toolbar_button_height);
}

gfx::Size BraveToolbarView::GetSizeInternal(
    gfx::Size (View::*get_size)() const) const {
  // Increase the base class width via our added Views
  gfx::Size size = ToolbarView::GetSizeInternal(get_size);
  if (is_display_mode_normal() && bookmark_ && bookmark_->visible()) {
    const int extra_width = GetLayoutConstant(TOOLBAR_ELEMENT_PADDING) +
                            (bookmark_->*get_size)().width();
    size.Enlarge(extra_width, 0);
  }
  return size;
}