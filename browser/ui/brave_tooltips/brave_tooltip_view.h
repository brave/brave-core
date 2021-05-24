/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_VIEW_H_
#define BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_VIEW_H_

#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/view.h"

namespace gfx {
class Canvas;
class Point;
}  // namespace gfx

namespace views {
class ImageView;
class Label;
class LabelButton;
class View;
}  // namespace views

namespace brave_tooltips {

class BraveTooltipView : public views::View {
 public:
  METADATA_HEADER(BraveTooltipView);

  explicit BraveTooltipView(const BraveTooltip& tooltip);
  ~BraveTooltipView() override;

  // views::InkDropHostView:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnThemeChanged() override;

 private:
  BraveTooltip tooltip_;

  gfx::Point initial_mouse_pressed_location_;
  bool is_dragging_ = false;

  bool is_closing_ = false;

  views::Label* title_label_ = nullptr;  // NOT OWNED
  views::Label* body_label_ = nullptr;   // NOT OWNED

  views::LabelButton* ok_button_ = nullptr;      // NOT OWNED
  views::LabelButton* cancel_button_ = nullptr;  // NOT OWNED

  void CreateView();

  void Close();

  views::View* CreateHeaderView();

  views::ImageView* CreateIconView();

  views::Label* CreateTitleLabel();

  views::View* CreateButtonView();

  views::LabelButton* CreateOkButton();
  void OnOkButtonPressed();

  views::LabelButton* CreateCancelButton();
  void OnCancelButtonPressed();

  views::View* CreateBodyView();
  views::Label* CreateBodyLabel();

  std::u16string accessible_name_;

  BraveTooltipView(const BraveTooltipView&) = delete;
  BraveTooltipView& operator=(const BraveTooltipView&) = delete;
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_VIEW_H_
