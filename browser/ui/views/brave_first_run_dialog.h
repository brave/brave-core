/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_FIRST_RUN_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_FIRST_RUN_DIALOG_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/shell_integrations/buildflags/buildflags.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class Checkbox;
}  // namespace views

class BraveFirstRunDialog : public views::DialogDelegateView {
  METADATA_HEADER(BraveFirstRunDialog, views::DialogDelegateView)
 public:

  BraveFirstRunDialog(const BraveFirstRunDialog&) = delete;
  BraveFirstRunDialog& operator=(const BraveFirstRunDialog&) = delete;

  static void Show(base::RepeatingClosure quit_runloop);

 private:
  explicit BraveFirstRunDialog(base::RepeatingClosure quit_runloop);
  ~BraveFirstRunDialog() override;

  // This terminates the nested message-loop.
  void Done();

  // views::DialogDelegate overrides:
  bool Accept() override;

  // views::WidgetDelegate overrides:
  void WindowClosing() override;

  base::RepeatingClosure quit_runloop_;

#if BUILDFLAG(ENABLE_PIN_SHORTCUT)
  raw_ptr<views::Checkbox> pin_shortcut_checkbox_ = nullptr;
#endif
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_FIRST_RUN_DIALOG_H_
