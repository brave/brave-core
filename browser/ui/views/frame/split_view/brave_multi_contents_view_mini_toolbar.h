/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_

#include <memory>

#include "base/callback_list.h"
#include "base/functional/callback_forward.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"

class EdgeHoverDetector;

namespace gfx {
class Point;
class Rect;
}  // namespace gfx

class BraveMultiContentsViewMiniToolbar : public MultiContentsViewMiniToolbar,
                                          public gfx::AnimationDelegate {
  METADATA_HEADER(BraveMultiContentsViewMiniToolbar,
                  MultiContentsViewMiniToolbar)
 public:
  // Describes the contents area that hosts this toolbar.
  enum class Style {
    // One side of a split. Meets the split border and offers the split menu.
    kSplit,
    // The web panel. Meets a split-style border, but has no split menu.
    kWebPanel,
    // A contents area that stands alone, surrounded by the thinner contents
    // outline.
    kStandalone,
  };

  BraveMultiContentsViewMiniToolbar(BrowserView* browser_view,
                                    ContentsWebView* web_view);
  ~BraveMultiContentsViewMiniToolbar() override;

  static BraveMultiContentsViewMiniToolbar* From(
      MultiContentsViewMiniToolbar* toolbar);

  // Keeps the favicon and domain visible while this toolbar's contents area is
  // active. By default they are shown only while it is inactive. Takes effect
  // on the next `UpdateState()` call.
  void SetAlwaysShowDomain(bool always_show_domain);

  // Sets how this toolbar is decorated and which controls it offers. Defaults
  // to `Style::kSplit`. Takes effect on the next `UpdateState()` call.
  void SetStyle(Style style);

  // Slides this toolbar out of view and lets pointer events pass through while
  // hovered.
  void SetHideOnHover(bool hide_on_hover);

  // Returns the current reveal fraction in [0, 1], where 1 is fully shown.
  double GetRevealFraction() const;

  // Returns the current downward slide offset in dips.
  int GetSlideOffset() const;

  // Registers `callback` to be invoked whenever the reveal fraction changes.
  base::CallbackListSubscription RegisterRevealFractionChanged(
      base::RepeatingClosure callback);

  // Returns the active hover detector, or null if hover-hiding is inactive.
  EdgeHoverDetector* hover_detector_for_testing() {
    return hover_detector_.get();
  }

  // Simulates a hover state transition for testing.
  void OnHoverStateChangedForTesting(bool hovering);

  // MultiContentsViewMiniToolbar:
  void UpdateContents() override;
  void UpdateState(bool is_active, bool is_highlighted) override;

 private:
  // views::View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void VisibilityChanged(views::View* starting_from, bool is_visible) override;
  void AddedToWidget() override;
  void RemovedFromWidget() override;

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  bool UsesSplitBorder() const;
  void UpdateHoverDetector();
  void ResetHoverHide();
  bool IsHoverPoint(const gfx::Point& screen_point) const;
  void OnHoverStateChanged(bool hovering);
  void UpdateSlideState();

  void CloseWebPanel();

  bool is_active_ = false;
  bool always_show_domain_ = false;
  bool hide_on_hover_ = false;
  Style style_ = Style::kSplit;

  gfx::SlideAnimation slide_animation_{this};
  std::unique_ptr<EdgeHoverDetector> hover_detector_;
  base::RepeatingClosureList reveal_fraction_changed_callbacks_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
