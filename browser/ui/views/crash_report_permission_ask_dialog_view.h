/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_CRASH_REPORT_PERMISSION_ASK_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_CRASH_REPORT_PERMISSION_ASK_DIALOG_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

class BrowserWindowInterface;

namespace views {
class Checkbox;
}  // namespace views

class CrashReportPermissionAskDialogView : public views::DialogDelegateView {
  METADATA_HEADER(CrashReportPermissionAskDialogView, views::DialogDelegateView)

 public:
  static void Show(BrowserWindowInterface* browser);

 private:
  CrashReportPermissionAskDialogView();
  ~CrashReportPermissionAskDialogView() override;

  CrashReportPermissionAskDialogView(
      const CrashReportPermissionAskDialogView&) = delete;
  CrashReportPermissionAskDialogView& operator=(
      const CrashReportPermissionAskDialogView&) = delete;

  // views::DialogDelegateView overrides:
  ui::mojom::ModalType GetModalType() const override;
  bool ShouldShowCloseButton() const override;
  bool ShouldShowWindowTitle() const override;
  void OnWidgetInitialized() override;

  void OnAcceptButtonClicked();
  void OnWindowClosing();
  void CreateChildViews();

  raw_ptr<views::Checkbox> dont_ask_again_checkbox_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_CRASH_REPORT_PERMISSION_ASK_DIALOG_VIEW_H_
