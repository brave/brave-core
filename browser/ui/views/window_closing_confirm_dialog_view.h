/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WINDOW_CLOSING_CONFIRM_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WINDOW_CLOSING_CONFIRM_DIALOG_VIEW_H_

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

class Browser;
class PrefService;
class DontAskAgainCheckbox;

class WindowClosingConfirmDialogView : public views::DialogDelegateView {
  METADATA_HEADER(WindowClosingConfirmDialogView, views::DialogDelegateView)
 public:

  static void Show(Browser* browser,
                   base::OnceCallback<void(bool)> response_callback);

  WindowClosingConfirmDialogView(const WindowClosingConfirmDialogView&) =
      delete;
  WindowClosingConfirmDialogView& operator=(
      const WindowClosingConfirmDialogView&) = delete;

 private:
  friend class WindowClosingConfirmBrowserTest;

  static void SetCreationCallbackForTesting(
      base::RepeatingCallback<void(views::DialogDelegateView*)>
          creation_callback);

  explicit WindowClosingConfirmDialogView(
      Browser* browser,
      base::OnceCallback<void(bool)> response_callback);
  ~WindowClosingConfirmDialogView() override;

  void OnAccept();
  void OnCancel();
  void OnClosing();

  // views::DialogDelegate overrides:
  ui::mojom::ModalType GetModalType() const override;
  bool ShouldShowCloseButton() const override;
  bool ShouldShowWindowTitle() const override;

  bool close_window_ = true;
  raw_ptr<Browser> browser_ = nullptr;
  base::OnceCallback<void(bool)> response_callback_;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<DontAskAgainCheckbox> dont_ask_again_checkbox_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WINDOW_CLOSING_CONFIRM_DIALOG_VIEW_H_
