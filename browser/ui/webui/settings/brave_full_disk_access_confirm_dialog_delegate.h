/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_FULL_DISK_ACCESS_CONFIRM_DIALOG_DELEGATE_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_FULL_DISK_ACCESS_CONFIRM_DIALOG_DELEGATE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog_delegate.h"

namespace content {
class WebContents;
}  // namespace content

class Browser;

class FullDiskAccessConfirmDialogDelegate
    : public TabModalConfirmDialogDelegate {
 public:
  FullDiskAccessConfirmDialogDelegate(content::WebContents* web_contents,
                                      Browser* browser);
  ~FullDiskAccessConfirmDialogDelegate() override;

  FullDiskAccessConfirmDialogDelegate(
      const FullDiskAccessConfirmDialogDelegate&) = delete;
  FullDiskAccessConfirmDialogDelegate& operator=(
      const FullDiskAccessConfirmDialogDelegate&) = delete;

 private:
  // TabModalConfirmDialogDelegate overrides:
  std::u16string GetTitle() override;
  std::u16string GetDialogMessage() override;
  std::u16string GetLinkText() const override;
  std::u16string GetAcceptButtonTitle() override;
  void OnAccepted() override;
  void OnLinkClicked(WindowOpenDisposition disposition) override;

  raw_ptr<Browser> browser_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_FULL_DISK_ACCESS_CONFIRM_DIALOG_DELEGATE_H_
