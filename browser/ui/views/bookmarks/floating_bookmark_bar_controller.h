// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_FLOATING_BOOKMARK_BAR_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_FLOATING_BOOKMARK_BAR_CONTROLLER_H_

#include <memory>
#include <optional>

#include "base/callback_list.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/views/frame/edge_reveal/edge_reveal_watcher.h"
#include "brave/browser/ui/views/view_shadow.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/events/event_observer.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget_observer.h"

class BraveBookmarkBarView;
class BraveBrowserView;

namespace views {
class EventMonitor;
}

// Controls the existing native bookmark view while it is a BrowserView child.
// BrowserView retains ownership and restores its ordinary parent on
// deactivation.
class FloatingBookmarkBarController : public ui::EventObserver,
                                      public content::WebContentsObserver,
                                      public gfx::AnimationDelegate,
                                      public views::FocusChangeListener,
                                      public views::ViewObserver,
                                      public views::WidgetObserver {
 public:
  FloatingBookmarkBarController(BraveBrowserView& browser_view,
                                BraveBookmarkBarView& bookmark_bar);
  FloatingBookmarkBarController(const FloatingBookmarkBarController&) = delete;
  FloatingBookmarkBarController& operator=(
      const FloatingBookmarkBarController&) = delete;
  ~FloatingBookmarkBarController() override;

  void UpdateLayout();
  void UpdateActiveContents(content::WebContents* contents);
  void RevealForKeyboard();
  bool IsRevealed() const;

  static gfx::Rect CalculateBounds(
      const gfx::Rect& contents_bounds,
      const gfx::Rect& toolbar_bounds,
      std::optional<gfx::Rect> vertical_tabs_bounds,
      int margin,
      int preferred_height);

 private:
  void OnEvent(const ui::Event& event) override;
  void DidFinishNavigation(content::NavigationHandle* navigation) override;
  void OnDidChangeFocus(views::View* before, views::View* now) override;
  void OnViewBoundsChanged(views::View* view) override;
  void OnViewThemeChanged(views::View* view) override;
  void OnViewIsDeleting(views::View* view) override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetWindowModalVisibilityChanged(views::Widget* widget,
                                            bool visible) override;
  void OnWidgetDestroying(views::Widget* widget) override;
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;
  void OnRevealReasonsChanged(EdgeRevealWatcher::RevealReasons reasons);
  bool CanReveal(bool for_keyboard = false) const;
  bool IsNewTabPage() const;
  bool IsHoverPoint(const gfx::Point& screen_point) const;
  void UpdateRevealState();
  void RevealAfterHover();
  void Hide();
  void SetRevealed(bool revealed);
  void UpdateAnimation();

  const raw_ref<BraveBrowserView> browser_view_;
  const raw_ref<BraveBookmarkBarView> bookmark_bar_;
  gfx::Rect bounds_in_screen_;
  gfx::Rect trigger_in_screen_;
  gfx::Point last_mouse_location_;
  bool hovering_ = false;
  bool dragging_ = false;
  bool modal_visible_ = false;
  bool created_layer_ = false;
  bool new_tab_dismissed_ = false;
  bool hover_dismissed_ = false;
  EdgeRevealWatcher::RevealReasons reveal_reasons_;
  gfx::SlideAnimation animation_{this};
  base::OneShotTimer show_timer_;
  base::OneShotTimer hide_timer_;
  base::CallbackListSubscription popup_state_subscription_;
  std::unique_ptr<ViewShadow> shadow_;
  std::unique_ptr<views::EventMonitor> event_monitor_;
  std::unique_ptr<EdgeRevealWatcher> reveal_watcher_;
  base::ScopedObservation<views::View, views::ViewObserver>
      bookmark_bar_observation_{this};
  base::ScopedObservation<views::View, views::ViewObserver>
      vertical_tabs_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};
  base::ScopedObservation<views::FocusManager, views::FocusChangeListener>
      focus_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BOOKMARKS_FLOATING_BOOKMARK_BAR_CONTROLLER_H_
