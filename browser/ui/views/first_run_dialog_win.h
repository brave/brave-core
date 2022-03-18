/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_WIN_H_
#define BRAVE_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_WIN_H_

#include "base/callback.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

class FirstRunDialogWin : public views::DialogDelegateView {
 public:
  METADATA_HEADER(FirstRunDialogWin);

  FirstRunDialogWin(const FirstRunDialogWin&) = delete;
  FirstRunDialogWin& operator=(const FirstRunDialogWin&) = delete;

  static void Show(base::RepeatingClosure quit_runloop);

 private:
  explicit FirstRunDialogWin(base::RepeatingClosure quit_runloop);
  ~FirstRunDialogWin() override;

  // This terminates the nested message-loop.
  void Done();

  // views::DialogDelegate overrides:
  bool Accept() override;

  // views::WidgetDelegate overrides:
  void WindowClosing() override;

  base::RepeatingClosure quit_runloop_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FIRST_RUN_DIALOG_WIN_H_
