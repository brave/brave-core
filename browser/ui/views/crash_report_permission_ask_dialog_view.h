/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_CRASH_REPORT_PERMISSION_ASK_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_CRASH_REPORT_PERMISSION_ASK_DIALOG_VIEW_H_

#include <memory>

#include "ui/views/window/dialog_delegate.h"

class Browser;

class CrashReportPermissionAskDialogView : public views::DialogDelegateView {
 public:
  static void Show(Browser* browser);

 private:
  explicit CrashReportPermissionAskDialogView(Browser* browser);
  ~CrashReportPermissionAskDialogView() override;

  CrashReportPermissionAskDialogView(
      const CrashReportPermissionAskDialogView&) = delete;
  CrashReportPermissionAskDialogView& operator=(
      const CrashReportPermissionAskDialogView&) = delete;

  // views::DialogDelegateView overrides:
  ui::ModalType GetModalType() const override;
  bool ShouldShowCloseButton() const override;
  bool ShouldShowWindowTitle() const override;
  void OnWidgetInitialized() override;

  void OnAcceptButtonClicked();
  void OnWindowClosing();
  void CreateChildViews(views::Widget* parent);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_CRASH_REPORT_PERMISSION_ASK_DIALOG_VIEW_H_
