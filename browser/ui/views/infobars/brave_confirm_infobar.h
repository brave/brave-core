/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/browser/ui/views/infobars/infobar_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace views {
class Checkbox;
class Label;
class MdTextButton;
}  // namespace views

// An infobar that shows a message, up to two optional buttons, and an optional,
// right-aligned link.  This is commonly used to do things like:
// "Would you like to do X?  [Yes]  [No]  [<custom button>]    _Learn More_ [x]"
class BraveConfirmInfoBar : public InfoBarView {
 public:
  METADATA_HEADER(BraveConfirmInfoBar);
  explicit BraveConfirmInfoBar(
      std::unique_ptr<BraveConfirmInfoBarDelegate> delegate);

  BraveConfirmInfoBar(const BraveConfirmInfoBar&) = delete;
  BraveConfirmInfoBar& operator=(const BraveConfirmInfoBar&) = delete;

  ~BraveConfirmInfoBar() override;

  // InfoBarView:
  void Layout() override;

  BraveConfirmInfoBarDelegate* GetDelegate() const;

 private:
  // InfoBarView:
  int GetContentMinimumWidth() const override;

  void OkButtonPressed();
  void CancelButtonPressed();
  void ExtraButtonPressed();
  void CloseButtonPressed() override;
  void CheckboxPressed();

  views::MdTextButton* GetButtonById(int id);

  // Returns the width of all content other than the label and link.  Layout()
  // uses this to determine how much space the label and link can take.
  int NonLabelWidth() const;

  raw_ptr<views::Label> label_ = nullptr;
  raw_ptr<views::MdTextButton> ok_button_ = nullptr;
  raw_ptr<views::MdTextButton> cancel_button_ = nullptr;
  raw_ptr<views::MdTextButton> extra_button_ = nullptr;
  raw_ptr<views::Link> link_ = nullptr;
  raw_ptr<views::Checkbox> checkbox_ = nullptr;

  base::WeakPtrFactory<BraveConfirmInfoBar> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_
