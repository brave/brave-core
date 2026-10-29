/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_OUTLINE_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_OUTLINE_H_

#include "chrome/browser/ui/views/frame/contents_container_outline.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/rounded_corners_f.h"

// Draws every outline Brave puts around the contents: the subtle outline
// outside of a split, and the active/inactive borders in a split or for the
// web panel. Brave uses per-corner radii and, unlike upstream, lets the stroke
// sit inside the reserved inset so the inactive split border can read as 1px
// while reserving the same 2px as the active one.
class BraveContentsContainerOutline : public ContentsContainerOutline {
  METADATA_HEADER(BraveContentsContainerOutline, ContentsContainerOutline)
 public:
  enum class Style {
    kHidden,
    kMiniToolbarOnly,
    kStandalone,
    kSplitActive,
    kSplitInactive,
  };

  // The inset reserved by both split styles, and the width kSplitActive
  // strokes.
  static constexpr int kSplitBorderThickness = 2;

  static BraveContentsContainerOutline* From(ContentsContainerOutline* view);

  // The inset the container has to reserve for |style|'s stroke.
  static int GetBorderThickness(Style style);

  // How wide |style| actually strokes the ring, which can be thinner than the
  // reserved inset. The mini toolbar sizes its margins to clear this.
  static int GetStrokeThickness(Style style);

  using ContentsContainerOutline::ContentsContainerOutline;
  ~BraveContentsContainerOutline() override;

  void UpdateStyle(Style style,
                   const gfx::RoundedCornersF& contents_corner_radii);

  Style style_for_testing() const { return style_; }

 private:
  // |contents_corner_radii| grown by GetBorderThickness() so the outline stays
  // concentric with the contents.
  static gfx::RoundedCornersF GetOuterCornerRadii(
      Style style,
      const gfx::RoundedCornersF& contents_corner_radii);

  // Per-corner version of ContentsContainerOutline::GetPath(), which only
  // takes a single radius. Brave's contents radii differ per corner (window
  // corners vs. corners meeting the split divider or side panel), so we build
  // the path ourselves rather than patching upstream's.
  //
  // Takes physical radii, and always cuts the notch out of the bottom right
  // corner. Mirroring it for RTL is BuildRingPath()'s job.
  static SkPath GetPath(const gfx::RectF& local_bounds,
                        const gfx::RoundedCornersF& corner_radii,
                        float mini_toolbar_corner_radius,
                        const gfx::SizeF& mini_toolbar_size);

  // Distance from the container's edge to the stroke's outer edge. Except for
  // kMiniToolbarOnly, this plus GetStrokeThickness() equals
  // GetBorderThickness(), i.e. the stroke ends flush with the inner edge of
  // the reserved inset.
  int GetStrokeOuterInset() const;

  ui::ColorId GetStrokeColorId() const;

  // The ring's centre line, in this view's coordinates. OnPaint() strokes it
  // and SetClipPath() clips the mini toolbar to it, so the two cannot disagree.
  SkPath BuildRingPath() const;

  // ContentsContainerOutline:
  void OnPaint(gfx::Canvas* canvas) override;
  void SetClipPath() override;

  Style style_ = Style::kHidden;
  gfx::RoundedCornersF contents_corner_radii_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_CONTENTS_CONTAINER_OUTLINE_H_
