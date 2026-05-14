/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/view.h"

namespace gfx {
class Insets;
}  // namespace gfx

namespace ui {
class Event;
}  // namespace ui

namespace views {
class Checkbox;
class ImageView;
class Label;
class Link;
class MdTextButton;
}  // namespace views

// An infobar that shows a message, up to two optional buttons, an optional
// checkbox, and an optional right-aligned link. Commonly used for:
//  "Would you like to do X?  [Yes]  [No]  [check] Remember  [_Learn More_] [x]"
//
// Unlike upstream's ConfirmInfoBar/InfoBarView, this class is self-contained:
// it inherits directly from infobars::InfoBar + views::View instead of from
// InfoBarView, so its layout doesn't change with features::kInfobarRefresh
// and stays insulated from future upstream restructuring of InfoBarView.
class BraveConfirmInfoBar : public infobars::InfoBar, public views::View {
  METADATA_HEADER(BraveConfirmInfoBar, views::View)

 public:
  using Views = std::vector<views::View*>;

  explicit BraveConfirmInfoBar(
      std::unique_ptr<BraveConfirmInfoBarDelegate> delegate);

  BraveConfirmInfoBar(const BraveConfirmInfoBar&) = delete;
  BraveConfirmInfoBar& operator=(const BraveConfirmInfoBar&) = delete;

  ~BraveConfirmInfoBar() override;

  // views::View:
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  BraveConfirmInfoBarDelegate* GetDelegate();
  const BraveConfirmInfoBarDelegate* GetDelegate() const;

  // Testing accessors for layout verification.
  views::ImageView* icon_for_testing() const { return icon_.get(); }
  views::Label* label_for_testing() const { return label_.get(); }
  views::MdTextButton* ok_button_for_testing() const {
    return ok_button_.get();
  }
  views::MdTextButton* cancel_button_for_testing() const {
    return cancel_button_.get();
  }
  views::Link* link_for_testing() const { return link_.get(); }
  views::Checkbox* checkbox_for_testing() const { return checkbox_.get(); }
  views::View* close_button_for_testing() const { return close_button_.get(); }

 protected:
  // Subclasses (e.g. BraveSyncAccountDeletedInfoBar) override Layout() and
  // reposition the views, so expose the children and the positioning
  // helpers here.
  raw_ptr<views::ImageView> icon_ = nullptr;
  raw_ptr<views::Label> label_ = nullptr;
  raw_ptr<views::MdTextButton> ok_button_ = nullptr;
  raw_ptr<views::MdTextButton> cancel_button_ = nullptr;
  raw_ptr<views::Link> link_ = nullptr;
  raw_ptr<views::Checkbox> checkbox_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;

  int GetStartX() const;
  int GetEndX() const;
  int OffsetY(views::View* view) const;
  static void AssignWidths(Views* views, int available_width);
  int NonLabelWidth() const;

 private:
  // Tracks the previously focused external view so we can restore focus on
  // dismissal; defined in the .cc.
  class FocusTracker;

  // infobars::InfoBar:
  void PlatformSpecificShow(bool animate) override;
  void PlatformSpecificHide(bool animate) override;
  void PlatformSpecificOnHeightRecalculated() override;

  // Layout helpers (adapted from InfoBarView).
  std::unique_ptr<views::Label> CreateLabel(const std::u16string& text) const;
  std::unique_ptr<views::Link> CreateLink(const std::u16string& text);
  void SetLabelDetails(views::Label* label) const;
  static void AssignWidthsSorted(Views* views, int available_width);
  gfx::Insets GetCloseButtonSpacing() const;
  int GetElementSpacing() const;
  void LinkClicked(const ui::Event& event);

  // Brave-specific.
  void MaybeLayoutMultiLineLabelAndLink();
  void OkButtonPressed();
  void CancelButtonPressed();
  void CloseButtonPressed();
  void CheckboxPressed();

  std::unique_ptr<FocusTracker> focus_tracker_;

  base::WeakPtrFactory<BraveConfirmInfoBar> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_
