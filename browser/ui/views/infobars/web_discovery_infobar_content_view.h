/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_WEB_DISCOVERY_INFOBAR_CONTENT_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_WEB_DISCOVERY_INFOBAR_CONTENT_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view.h"

class WebDiscoveryInfoBarDelegate;

// Occupy whole infobar area.
class WebDiscoveryInfoBarContentView : public views::View {
 public:
  METADATA_HEADER(WebDiscoveryInfoBarContentView);

  explicit WebDiscoveryInfoBarContentView(
      WebDiscoveryInfoBarDelegate* delegate);
  WebDiscoveryInfoBarContentView(const WebDiscoveryInfoBarContentView&) =
      delete;
  WebDiscoveryInfoBarContentView& operator=(
      const WebDiscoveryInfoBarContentView&) = delete;
  ~WebDiscoveryInfoBarContentView() override;

 private:
  // views::View overrides:
  void OnPaint(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;
  void AddedToWidget() override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  void SwitchChildLayout();
  void InitChildren();
  void InitChildrenForWideLayout();
  void InitChildrenForNarrowLayout();
  std::unique_ptr<views::View> GetMessage(int order, int line_height);
  std::unique_ptr<views::View> GetIcon(int order);
  std::unique_ptr<views::View> GetNoThanksButton(int order);
  std::unique_ptr<views::View> GetOkButton(const gfx::Size& size, int order);
  std::unique_ptr<views::View> GetCloseButton();
  std::unique_ptr<views::View> GetSpacer(
      const gfx::Size& size,
      int order = 1,
      views::MinimumFlexSizeRule min_rule =
          views::MinimumFlexSizeRule::kPreferred,
      views::MaximumFlexSizeRule max_rule =
          views::MaximumFlexSizeRule::kPreferred);

  void EnableWebDiscovery();
  void Dismiss();
  void CloseInfoBar();

  raw_ptr<views::View> wide_layout_container_ = nullptr;
  raw_ptr<views::View> narrow_layout_container_ = nullptr;
  int wide_layout_min_width_ = 0;
  int narrow_layout_preferred_width_ = 0;
  raw_ptr<WebDiscoveryInfoBarDelegate> delegate_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_WEB_DISCOVERY_INFOBAR_CONTENT_VIEW_H_
