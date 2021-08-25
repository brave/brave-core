/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_DEFAULT_BROWSER_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_DEFAULT_BROWSER_DIALOG_VIEW_H_

#include <memory>

#include "ui/views/window/dialog_delegate.h"

namespace views {
class Checkbox;
class Label;
}  // namespace views

class BraveDefaultBrowserDialogView : public views::DialogDelegateView {
 public:
  BraveDefaultBrowserDialogView();
  ~BraveDefaultBrowserDialogView() override;

  BraveDefaultBrowserDialogView(const BraveDefaultBrowserDialogView&) = delete;
  BraveDefaultBrowserDialogView& operator=(
      const BraveDefaultBrowserDialogView&) = delete;

  // views::DialogDelegateView overrides:
  ui::ModalType GetModalType() const override;
  bool ShouldShowCloseButton() const override;
  std::unique_ptr<views::NonClientFrameView> CreateNonClientFrameView(
      views::Widget* widget) override;
  void OnWidgetInitialized() override;

 private:
  void OnCancelButtonClicked();
  void OnAcceptButtonClicked();
  void CreateChildViews();

  views::Label* header_label_ = nullptr;
  views::Label* contents_label_ = nullptr;
  views::Checkbox* dont_ask_again_checkbox_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_DEFAULT_BROWSER_DIALOG_VIEW_H_
