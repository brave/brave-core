/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OBSOLETE_SYSTEM_CONFIRM_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_OBSOLETE_SYSTEM_CONFIRM_DIALOG_VIEW_H_

#include "base/functional/callback_forward.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

class ObsoleteSystemConfirmDialogView : public views::DialogDelegateView {
  METADATA_HEADER(ObsoleteSystemConfirmDialogView, views::DialogDelegateView)
 public:

  explicit ObsoleteSystemConfirmDialogView(
      base::OnceCallback<void(bool)> closing_callback);
  ObsoleteSystemConfirmDialogView(const ObsoleteSystemConfirmDialogView&) =
      delete;
  ObsoleteSystemConfirmDialogView& operator=(
      const ObsoleteSystemConfirmDialogView&) = delete;
  ~ObsoleteSystemConfirmDialogView() override;

 private:
  void OnButtonPressed(bool accept);

  base::OnceCallback<void(bool)> closing_callback_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OBSOLETE_SYSTEM_CONFIRM_DIALOG_VIEW_H_
