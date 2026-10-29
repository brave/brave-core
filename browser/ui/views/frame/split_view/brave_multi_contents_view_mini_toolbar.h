/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_

#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveMultiContentsViewMiniToolbar : public MultiContentsViewMiniToolbar {
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

  using MultiContentsViewMiniToolbar::MultiContentsViewMiniToolbar;
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

  // MultiContentsViewMiniToolbar:
  void UpdateContents() override;
  void UpdateState(bool is_active, bool is_highlighted) override;

 private:
  bool UsesSplitBorder() const;
  int GetOutlineThickness() const;

  bool is_active_ = false;
  bool always_show_domain_ = false;
  Style style_ = Style::kSplit;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
