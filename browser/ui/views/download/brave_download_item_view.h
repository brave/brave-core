/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_DOWNLOAD_BRAVE_DOWNLOAD_ITEM_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_DOWNLOAD_BRAVE_DOWNLOAD_ITEM_VIEW_H_

#include "brave/browser/download/brave_download_item_model.h"
#include "chrome/browser/ui/download/download_item_mode.h"
#include "chrome/browser/ui/views/download/download_item_view.h"

// The purpose of this subclass is to add URL origin and lock icon to the
// download item view (shown in the download shelf).
class BraveDownloadItemView : public DownloadItemView {
 public:
  BraveDownloadItemView(DownloadUIModel::DownloadUIModelPtr download,
      DownloadShelfView* parent, views::View* accessible_alert);
  BraveDownloadItemView(const BraveDownloadItemView&) = delete;
  BraveDownloadItemView& operator=(const BraveDownloadItemView&) = delete;
  ~BraveDownloadItemView() override;

  // views::View:
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;

  // download::DownloadItem::Observer:
  void OnDownloadUpdated() override;

 protected:
  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  // Padding between URL lock icon and URL text.
  static constexpr int kOriginURLIconRightPadding = 2;

  // Vertical padding between text lines.
  static constexpr int kBraveVerticalTextPadding = 2;

  // These functions calculate the vertical coordinates for each text line.
  int GetYForFilenameText() const;
  int GetYForOriginURLText() const;
  int GetYForStatusText() const;

  // Painting of various download item bits.
  void DrawOriginURL(gfx::Canvas* canvas);
  void DrawLockIcon(gfx::Canvas* canvas);

  // Returns a square lock icon of |height|.
  gfx::ImageSkia GetLockIcon(int height);

  // Overrides the accessible name construction to reflect the origin URL.
  void SetMode(download::DownloadItemMode mode) override;
  void UpdateLabels() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnViewFocused(View* observed_view) override;
  void OnViewBlurred(View* observed_view) override;

  void SetOriginUrlVisible(bool visible);

  // Brave download item model.
  BraveDownloadItemModel brave_model_;

  // The font list used to draw the origin URL text.
  gfx::FontList origin_url_font_list_;

  // Origin url text.
  std::u16string origin_url_text_;
  bool is_origin_url_secure_;
  bool is_origin_url_visible_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_DOWNLOAD_BRAVE_DOWNLOAD_ITEM_VIEW_H_
