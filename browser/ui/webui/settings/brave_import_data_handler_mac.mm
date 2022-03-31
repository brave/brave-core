/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"

#import <AppKit/AppKit.h>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/common/url_constants.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog_delegate.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_types.h"
#include "url/gurl.h"

namespace {

using content::BrowserThread;

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

  Browser* browser_;
};

FullDiskAccessConfirmDialogDelegate::FullDiskAccessConfirmDialogDelegate(
    content::WebContents* web_contents,
    Browser* browser)
    : TabModalConfirmDialogDelegate(web_contents),
      browser_(browser) {}

FullDiskAccessConfirmDialogDelegate::
    ~FullDiskAccessConfirmDialogDelegate() = default;

std::u16string FullDiskAccessConfirmDialogDelegate::GetTitle() {
  return l10n_util::GetStringUTF16(IDS_FULL_DISK_ACCESS_CONFIRM_DIALOG_TITLE);
}

std::u16string FullDiskAccessConfirmDialogDelegate::GetDialogMessage() {
  return l10n_util::GetStringUTF16(IDS_FULL_DISK_ACCESS_CONFIRM_DIALOG_MESSAGE);
}

std::u16string FullDiskAccessConfirmDialogDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_FULL_DISK_ACCESS_CONFIRM_DIALOG_LINK_TEXT);
}

std::u16string FullDiskAccessConfirmDialogDelegate::GetAcceptButtonTitle() {
  return l10n_util::GetStringUTF16(
      IDS_FULL_DISK_ACCESS_CONFIRM_DIALOG_OPEN_PREFS_BUTTON_TEXT);
}

void FullDiskAccessConfirmDialogDelegate::OnAccepted() {
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:
      @"x-apple.systempreferences:com.apple.preference.security?Privacy_AllFiles"]];  // NOLINT
}

void FullDiskAccessConfirmDialogDelegate::OnLinkClicked(
    WindowOpenDisposition disposition) {
  const int target_index =
      browser_->tab_strip_model()->active_index() + 1;
  // Add import help tab right after current settings tab.
  chrome::AddTabAt(browser_, GURL(kImportDataHelpURL),
                   target_index, true /* foreground */);
}

bool HasProperDiskAccessPermission(uint16_t imported_items) {
  DCHECK(imported_items);

  const base::FilePath& library_dir = base::mac::GetUserLibraryPath();
  const base::FilePath safari_dir = library_dir.Append("Safari");

  if (imported_items & importer::FAVORITES) {
    const base::FilePath bookmarks_path = safari_dir.Append("Bookmarks.plist");
    if(!PathIsWritable(bookmarks_path)) {
      LOG(ERROR) << __func__ << " " << bookmarks_path << " is not accessible."
                 << " Please check full disk access permission.";
      return false;
    }
  }

  if (imported_items & importer::HISTORY) {
    const base::FilePath history_path = safari_dir.Append("History.plist");
    if(!PathIsWritable(history_path)) {
      LOG(ERROR) << __func__ << " " << history_path << " is not accessible."
                 << " Please check full disk access permission.";
      return false;
    }
  }

  return true;
}

}  // namespace

namespace settings {

BraveImportDataHandler::BraveImportDataHandler() : weak_factory_(this) {}
BraveImportDataHandler::~BraveImportDataHandler() = default;

void BraveImportDataHandler::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  guide_dialog_is_requested_ = false;

  if (!imported_items)
    return;

  if (source_profile.importer_type == importer::TYPE_SAFARI) {
    // Start import if Brave has full disk access permission.
    // If not, show dialog that has infos about that permission.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&HasProperDiskAccessPermission, imported_items),
        base::BindOnce(&BraveImportDataHandler::OnGetDiskAccessPermission,
                       weak_factory_.GetWeakPtr(), source_profile,
                       imported_items));
    return;
  }

  ImportDataHandler::StartImport(source_profile, imported_items);
}

void BraveImportDataHandler::OnGetDiskAccessPermission(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items,
    bool allowed) {
  if (!allowed) {
    // Notify to webui to finish import process and launch tab modal dialog
    // to guide full disk access information to users.
    // Guide dialog will be opened after import dialog is closed.
    FireWebUIListener("import-data-status-changed", base::Value("failed"));

    // Observing web_contents is started here to know the closing timing of
    // import dialog.
    Observe(web_ui()->GetWebContents());

    guide_dialog_is_requested_ = true;
    return;
  }

  return ImportDataHandler::StartImport(source_profile, imported_items);
}

void BraveImportDataHandler::DidStopLoading() {
  Observe(nullptr);

  if (!guide_dialog_is_requested_)
    return;

  guide_dialog_is_requested_ = false;

  auto* web_contents = web_ui()->GetWebContents();
  TabModalConfirmDialog::Create(
      std::make_unique<FullDiskAccessConfirmDialogDelegate>(
          web_contents,
          chrome::FindBrowserWithWebContents(web_contents)),
      web_contents);
}

}  // namespace settings
