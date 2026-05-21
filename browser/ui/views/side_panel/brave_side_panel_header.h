/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_HEADER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_HEADER_H_

#include <memory>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class Label;
}  // namespace views

// Header view shown on top of the side panel border in sidebar v2. The visual
// spec mirrors the v1 inline headers in BraveReadLaterSidePanelView and
// BraveBookmarksSidePanelView; per-entry pieces are produced by a Delegate.
class BraveSidePanelHeader : public views::View {
  METADATA_HEADER(BraveSidePanelHeader, views::View)

 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual std::unique_ptr<views::Label> CreatePanelTitle() = 0;

    // Return nullptr when the entry has no launch action (e.g. reading list).
    virtual std::unique_ptr<views::ImageButton> CreateLaunchButton() = 0;
    virtual std::unique_ptr<views::ImageButton> CreateCloseButton() = 0;
  };

  explicit BraveSidePanelHeader(std::unique_ptr<Delegate> delegate);
  BraveSidePanelHeader(const BraveSidePanelHeader&) = delete;
  BraveSidePanelHeader& operator=(const BraveSidePanelHeader&) = delete;
  ~BraveSidePanelHeader() override;

  // views::View:
  void Layout(PassKey) override;

 private:
  std::unique_ptr<Delegate> delegate_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_HEADER_H_
