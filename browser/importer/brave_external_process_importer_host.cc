/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "brave/browser/importer/brave_external_process_importer_client.h"
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/browser/importer/brave_in_process_importer_bridge.h"
#include "brave/browser/importer/brave_profile_lock.h"
#include "brave/browser/importer/chrome_profile_lock.h"

#include "brave/browser/importer/brave_importer_lock_dialog.h"

BraveExternalProcessImporterHost::BraveExternalProcessImporterHost()
    : ExternalProcessImporterHost(), weak_ptr_factory_(this) {}

BraveExternalProcessImporterHost::~BraveExternalProcessImporterHost() {}

void BraveExternalProcessImporterHost::StartImportSettings(
    const importer::SourceProfile& source_profile,
    Profile* target_profile,
    uint16_t items,
    ProfileWriter* writer) {
  // We really only support importing from one host at a time.
  DCHECK(!profile_);
  DCHECK(target_profile);

  profile_ = target_profile;
  writer_ = writer;
  source_profile_ = source_profile;
  items_ = items;

  if (!ExternalProcessImporterHost::CheckForFirefoxLock(source_profile)) {
    Cancel();
    return;
  }

  if (!CheckForChromeOrBraveLock()) {
    Cancel();
    return;
  }

  ExternalProcessImporterHost::CheckForLoadedModels(items);

  LaunchImportIfReady();
}

void BraveExternalProcessImporterHost::LaunchImportIfReady() {
  if (waiting_for_bookmarkbar_model_ || template_service_subscription_.get() ||
      !is_source_readable_ || cancelled_)
    return;

  // This is the in-process half of the bridge, which catches data from the IPC
  // pipe and feeds it to the ProfileWriter. The external process half of the
  // bridge lives in the external process (see ProfileImportThread).
  // The ExternalProcessImporterClient created in the next line owns the bridge,
  // and will delete it.
  BraveInProcessImporterBridge* bridge = new BraveInProcessImporterBridge(
      writer_.get(), weak_ptr_factory_.GetWeakPtr());
  client_ = new BraveExternalProcessImporterClient(
      weak_ptr_factory_.GetWeakPtr(), source_profile_, items_, bridge);
  client_->Start();

  RecordImporterP3A(source_profile_.importer_type);
}

void BraveExternalProcessImporterHost::ShowWarningDialog() {
  DCHECK(!headless_);
  brave::importer::ShowImportLockDialog(
      parent_window_, source_profile_,
      base::Bind(&BraveExternalProcessImporterHost::OnImportLockDialogEnd,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BraveExternalProcessImporterHost::OnImportLockDialogEnd(bool is_continue) {
  if (is_continue) {
    // User chose to continue, then we check the lock again to make sure that
    // the other browser has been closed. Try to import the settings if
    // successful. Otherwise, show a warning dialog.
    browser_lock_->Lock();
    if (browser_lock_->HasAcquired()) {
      is_source_readable_ = true;
      LaunchImportIfReady();
    } else {
      ShowWarningDialog();
    }
  } else {
    NotifyImportEnded();
  }
}

bool BraveExternalProcessImporterHost::CheckForChromeOrBraveLock() {
  if (!(source_profile_.importer_type == importer::TYPE_CHROME ||
        source_profile_.importer_type == importer::TYPE_BRAVE))
    return true;

  DCHECK(!browser_lock_.get());

  if (source_profile_.importer_type == importer::TYPE_CHROME) {
    // Extract the user data directory from the path of the profile to be
    // imported, because we can only lock/unlock the entire user directory with
    // ProcessSingleton.
    base::FilePath user_data_dir = source_profile_.source_path.DirName();
    browser_lock_.reset(new ChromeProfileLock(user_data_dir));
  } else {  // source_profile_.importer_type == importer::TYPE_BRAVE
    browser_lock_.reset(new BraveProfileLock(source_profile_.source_path));
  }

  browser_lock_->Lock();
  if (browser_lock_->HasAcquired())
    return true;

  // If fail to acquire the lock, we set the source unreadable and
  // show a warning dialog, unless running without UI (in which case the import
  // must be aborted).
  is_source_readable_ = false;
  if (headless_)
    return false;

  ShowWarningDialog();
  return true;
}
