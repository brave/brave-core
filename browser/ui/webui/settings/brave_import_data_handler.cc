/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"

#include <memory>
#include <unordered_map>

#include "brave/browser/ui/webui/settings/import_feature.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/importer/profile_writer.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(IS_MAC)
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ui/webui/settings/brave_full_disk_access_confirm_dialog_delegate.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "content/public/browser/browser_thread.h"
#endif  // BUILDFLAG(IS_MAC)

namespace {
#if BUILDFLAG(IS_MAC)
bool HasProperDiskAccessPermission(uint16_t imported_items) {
  DCHECK(imported_items);

  const base::FilePath& library_dir = base::mac::GetUserLibraryPath();
  const base::FilePath safari_dir = library_dir.Append("Safari");

  if (imported_items & importer::FAVORITES) {
    const base::FilePath bookmarks_path = safari_dir.Append("Bookmarks.plist");
    if (!PathIsWritable(bookmarks_path)) {
      LOG(ERROR) << __func__ << " " << bookmarks_path << " is not accessible."
                 << " Please check full disk access permission.";
      return false;
    }
  }

  if (imported_items & importer::HISTORY) {
    const base::FilePath history_path = safari_dir.Append("History.plist");
    if (!PathIsWritable(history_path)) {
      LOG(ERROR) << __func__ << " " << history_path << " is not accessible."
                 << " Please check full disk access permission.";
      return false;
    }
  }

  return true;
}
#endif  // BUILDFLAG(IS_MAC)
}  // namespace

namespace settings {

BraveImportDataHandler::BraveImportDataHandler() = default;
BraveImportDataHandler::~BraveImportDataHandler() = default;

void BraveImportDataHandler::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
#if BUILDFLAG(IS_MAC)
  CheckDiskAccess(source_profile, imported_items);
#else
  StartImportImpl(source_profile, imported_items);
#endif
}

void BraveImportDataHandler::StartImportImpl(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  // Temporary flag to keep old way until
  // https://github.com/brave/brave-core/pull/15637 landed.
  // Should be removed in that PR.
  if (!IsParallelImportEnabled()) {
    ImportDataHandler::StartImport(source_profile, imported_items);
    return;
  }
  if (!imported_items)
    return;
  // If another import is already ongoing, let it finish silently.
  if (import_observers_.count(source_profile.source_path))
    import_observers_.erase(source_profile.source_path);

  // Using weak pointers because it destroys itself when finshed.
  auto* importer_host = new ExternalProcessImporterHost();
  import_observers_[source_profile.source_path] =
      std::make_unique<settings::BraveImporterObserver>(
          importer_host, source_profile, imported_items,
          base::BindRepeating(&BraveImportDataHandler::NotifyImportProgress,
                              weak_factory_.GetWeakPtr()));
  importer_host->set_observer(
      import_observers_[source_profile.source_path].get());

  Profile* profile = Profile::FromWebUI(web_ui());
  importer_host->StartImportSettings(source_profile, profile, imported_items,
                                     new ProfileWriter(profile));
}

void BraveImportDataHandler::NotifyImportProgress(const base::Value& info) {
  FireWebUIListener("brave-import-data-status-changed", info);
}

#if BUILDFLAG(IS_MAC)
void BraveImportDataHandler::CheckDiskAccess(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

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

  StartImportImpl(source_profile, imported_items);
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
    if (IsParallelImportEnabled()) {
      import_observers_[source_profile.source_path]->ImportEnded();
    }
    // Observing web_contents is started here to know the closing timing of
    // import dialog.
    Observe(web_ui()->GetWebContents());

    guide_dialog_is_requested_ = true;
    return;
  }

  StartImportImpl(source_profile, imported_items);
}

void BraveImportDataHandler::DidStopLoading() {
  Observe(nullptr);

  if (!guide_dialog_is_requested_)
    return;

  guide_dialog_is_requested_ = false;

  auto* web_contents = web_ui()->GetWebContents();
  TabModalConfirmDialog::Create(
      std::make_unique<FullDiskAccessConfirmDialogDelegate>(
          web_contents, chrome::FindBrowserWithWebContents(web_contents)),
      web_contents);
}
#endif
}  // namespace settings
