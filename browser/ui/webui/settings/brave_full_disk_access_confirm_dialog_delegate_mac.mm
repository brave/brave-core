/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_full_disk_access_confirm_dialog_delegate.h"

#import <AppKit/AppKit.h>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/constants/url_constants.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog_delegate.h"
#include "chrome/grit/generated_resources.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_types.h"
#include "url/gurl.h"

FullDiskAccessConfirmDialogDelegate::FullDiskAccessConfirmDialogDelegate(
    content::WebContents* web_contents,
    Browser* browser)
    : TabModalConfirmDialogDelegate(web_contents), browser_(browser) {}

FullDiskAccessConfirmDialogDelegate::~FullDiskAccessConfirmDialogDelegate() =
    default;

std::u16string FullDiskAccessConfirmDialogDelegate::GetTitle() {
  return l10n_util::GetStringUTF16(IDS_FULL_DISK_ACCESS_CONFIRM_DIALOG_TITLE);
}

std::u16string FullDiskAccessConfirmDialogDelegate::GetDialogMessage() {
  return l10n_util::GetStringUTF16(IDS_FULL_DISK_ACCESS_CONFIRM_DIALOG_MESSAGE);
}

std::u16string FullDiskAccessConfirmDialogDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(
      IDS_FULL_DISK_ACCESS_CONFIRM_DIALOG_LINK_TEXT);
}

std::u16string FullDiskAccessConfirmDialogDelegate::GetAcceptButtonTitle() {
  return l10n_util::GetStringUTF16(
      IDS_FULL_DISK_ACCESS_CONFIRM_DIALOG_OPEN_PREFS_BUTTON_TEXT);
}

void FullDiskAccessConfirmDialogDelegate::OnAccepted() {
  [[NSWorkspace sharedWorkspace]
      openURL:[NSURL URLWithString:
                         @"x-apple.systempreferences:com.apple.preference."
                         @"security?Privacy_AllFiles"]];  // NOLINT
}

void FullDiskAccessConfirmDialogDelegate::OnLinkClicked(
    WindowOpenDisposition disposition) {
  const int target_index = browser_->tab_strip_model()->active_index() + 1;
  // Add import help tab right after current settings tab.
  chrome::AddTabAt(browser_, GURL(kImportDataHelpURL), target_index,
                   true /* foreground */);
}
