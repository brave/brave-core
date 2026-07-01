/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "brave/browser/importer/brave_password_importer.h"
#include "chrome/browser/importer/importer_list.h"
#include "chrome/browser/importer/profile_writer.h"
#include "chrome/browser/profiles/profile.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "components/user_data_importer/common/importer_type.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(IS_MAC)
#include "base/apple/foundation_util.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ui/webui/settings/brave_full_disk_access_confirm_dialog_delegate.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#endif  // BUILDFLAG(IS_MAC)

namespace {
#if BUILDFLAG(IS_MAC)
bool HasProperDiskAccessPermission(uint16_t imported_items) {
  DCHECK(imported_items);

  const base::FilePath& library_dir = base::apple::GetUserLibraryPath();
  const base::FilePath safari_dir = library_dir.Append("Safari");

  if (imported_items & user_data_importer::FAVORITES) {
    const base::FilePath bookmarks_path = safari_dir.Append("Bookmarks.plist");
    if (!PathIsWritable(bookmarks_path)) {
      LOG(ERROR) << __func__ << " " << bookmarks_path << " is not accessible."
                 << " Please check full disk access permission.";
      return false;
    }
  }

  if (imported_items & user_data_importer::HISTORY) {
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
constexpr char kImportStatusSucceeded[] = "succeeded";
constexpr char kImportStatusFailed[] = "failed";
}  // namespace

namespace settings {

BraveImportDataHandler::BraveImportDataHandler() = default;
BraveImportDataHandler::~BraveImportDataHandler() = default;

void BraveImportDataHandler::StartImport(
    const user_data_importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  if (!imported_items) {
    return;
  }
  Profile* profile = Profile::FromWebUI(web_ui());
#if BUILDFLAG(IS_MAC)
  CheckDiskAccess(imported_items, source_profile.source_path,
                  source_profile.importer_type,
                  base::BindOnce(&BraveImportDataHandler::StartImportImpl,
                                 weak_factory_.GetWeakPtr(), source_profile,
                                 imported_items, profile));
#else
  StartImportImpl(source_profile, imported_items, profile);
#endif
}

void BraveImportDataHandler::StartImportImpl(
    const user_data_importer::SourceProfile& source_profile,
    uint16_t imported_items,
    Profile* profile) {
  // If another import is already ongoing, let it finish silently.
  if (import_observers_.count(source_profile.source_path)) {
    import_observers_.erase(source_profile.source_path);
  }

  FireWebUIListener("import-data-status-changed", base::Value("inProgress"));

  // Passwords from another Brave installation are imported directly in the
  // browser process so we can use OSCryptAsync to decrypt the source's
  // `Login Data`. The utility-process ChromeImporter does not carry password
  // import code, so we strip PASSWORDS from the items it receives.
  uint16_t utility_items = imported_items;
  if ((imported_items & user_data_importer::PASSWORDS) &&
      source_profile.importer_type == user_data_importer::TYPE_BRAVE) {
    utility_items &= ~user_data_importer::PASSWORDS;
    password_importers_[source_profile.source_path] =
        std::make_unique<BravePasswordImporter>();
    password_importers_[source_profile.source_path]->Start(
        source_profile.source_path, profile,
        base::BindOnce(&BraveImportDataHandler::OnPasswordImportFinished,
                       weak_factory_.GetWeakPtr(), source_profile.source_path));
  }

  // Using weak pointers because it destroys itself when finshed.
  auto* importer_host = new BraveExternalProcessImporterHost();
  import_observers_[source_profile.source_path] =
      std::make_unique<BraveImporterObserver>(
          importer_host, source_profile, utility_items,
          base::BindRepeating(&BraveImportDataHandler::NotifyImportProgress,
                              weak_factory_.GetWeakPtr()));

  importer_host->set_parent_window(
      web_ui()->GetWebContents()->GetTopLevelNativeWindow());
  importer_host->set_parent_view(web_ui()->GetWebContents()->GetNativeView());

  importer_host->StartImportSettings(source_profile, profile, utility_items,
                                     new ProfileWriter(profile));
}

void BraveImportDataHandler::OnPasswordImportFinished(
    base::FilePath source_path,
    size_t added) {
  password_importers_.erase(source_path);
  if (added > 0) {
    import_did_succeed_ = true;
  } else {
    LOG(WARNING) << "Brave password import finished with no credentials added";
  }
}

void BraveImportDataHandler::NotifyImportProgress(
    const user_data_importer::SourceProfile& source_profile,
    const base::DictValue& info) {
  const std::string* event = info.FindString("event");
  if (!event) {
    return;
  }
  if (*event == "ImportItemEnded") {
    import_did_succeed_ = true;
  } else if (*event == "ImportEnded") {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&BraveImportDataHandler::OnImportEnded,
                                  weak_factory_.GetWeakPtr(), source_profile));
  }
}

void BraveImportDataHandler::HandleImportData(const base::ListValue& args) {
  ImportDataHandler::HandleImportData(args);
}

void BraveImportDataHandler::OnImportEnded(
    const user_data_importer::SourceProfile& source_profile) {
  import_observers_.erase(source_profile.source_path);
  FireWebUIListener("import-data-status-changed",
                    base::Value(import_did_succeed_ ? kImportStatusSucceeded
                                                    : kImportStatusFailed));
}

void BraveImportDataHandler::OnJavascriptDisallowed() {
  ImportDataHandler::OnJavascriptDisallowed();

  // When the WebUI is unloading, we ignore all further updates from the host as
  // ImportDataHandler does.
  import_observers_.clear();
}

const user_data_importer::SourceProfile&
BraveImportDataHandler::GetSourceProfileAt(int browser_index) {
  return importer_list_->GetSourceProfileAt(browser_index);
}

#if BUILDFLAG(IS_MAC)
void BraveImportDataHandler::CheckDiskAccess(
    uint16_t imported_items,
    base::FilePath source_path,
    user_data_importer::ImporterType importer_type,
    ContinueImportCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  guide_dialog_is_requested_ = false;

  if (importer_type == user_data_importer::TYPE_SAFARI) {
    // Start import if Brave has full disk access permission.
    // If not, show dialog that has infos about that permission.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&HasProperDiskAccessPermission, imported_items),
        base::BindOnce(&BraveImportDataHandler::OnGetDiskAccessPermission,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       source_path));
    return;
  }
  std::move(callback).Run();
}

void BraveImportDataHandler::OnGetDiskAccessPermission(
    ContinueImportCallback callback,
    base::FilePath source_path,
    bool allowed) {
  if (!allowed) {
    // Notify to webui to finish import process and launch tab modal dialog
    // to guide full disk access information to users.
    // Guide dialog will be opened after import dialog is closed.
    FireWebUIListener("import-data-status-changed", base::Value("failed"));
    if (import_observers_.count(source_path)) {
      import_observers_[source_path]->ImportEnded();
    }
    // Observing web_contents is started here to know the closing timing of
    // import dialog.
    Observe(web_ui()->GetWebContents());

    guide_dialog_is_requested_ = true;
    return;
  }

  std::move(callback).Run();
}

void BraveImportDataHandler::DidStopLoading() {
  Observe(nullptr);

  if (!guide_dialog_is_requested_) {
    return;
  }

  guide_dialog_is_requested_ = false;

  auto* web_contents = web_ui()->GetWebContents();
  TabModalConfirmDialog::Create(
      std::make_unique<FullDiskAccessConfirmDialogDelegate>(
          web_contents, chrome::FindBrowserWithTab(web_contents)),
      web_contents);
}
#endif
}  // namespace settings
