/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"

#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/browser/importer/extensions_import_helpers.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/importer/importer_lock_dialog.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "components/user_data_importer/common/importer_type.h"

BraveExternalProcessImporterHost::BraveExternalProcessImporterHost()
    : weak_ptr_factory_(this) {}
BraveExternalProcessImporterHost::~BraveExternalProcessImporterHost() = default;

void BraveExternalProcessImporterHost::NotifyImportEnded() {
  // Guard against re-recording: this method is re-entered to continue the
  // import lifecycle after browser-process steps (password and extension
  // import), and P3A must be recorded only once per import.
  if (!cancelled_ && !p3a_recorded_) {
    p3a_recorded_ = true;
    RecordImporterP3A(source_profile_.importer_type);
  }

  // Passwords are imported in the browser process (not the utility process) so
  // OSCryptAsync can decrypt the source's `Login Data`. `password_importer_` is
  // created lazily here and reset in OnPasswordsImported(), which re-enters
  // this method to continue the lifecycle.
  if (NeedToImportPasswords() && !password_import_started_) {
    password_import_started_ = true;
    password_importer_ = std::make_unique<BravePasswordImporter>();
    NotifyImportItemStarted(user_data_importer::PASSWORDS);
    password_importer_->StartImport(
        source_profile_.source_path, profile_,
        base::BindOnce(&BraveExternalProcessImporterHost::OnPasswordsImported,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // If user chooses extension importing, start importing extensions.
  // and NotifyImportEnded() will be called from OnGetChromeExtensionsList().
  // Handling extensions importing after finishing all other properties makes
  // logic simpler.
  // Don't import if cancelled.
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (NeedToImportExtensions() && extensions_importer_) {
    NotifyImportItemStarted(user_data_importer::EXTENSIONS);
    if (extensions_importer_->Import(base::BindRepeating(
            &BraveExternalProcessImporterHost::OnExtensionImported,
            weak_ptr_factory_.GetWeakPtr()))) {
      return;
    }
  }
#endif
  // Force tests to fail if |this| is deleted.
  DCHECK(weak_ptr_factory_.GetWeakPtr());

  // Otherwise, notifying here and importing is finished.
  ExternalProcessImporterHost::NotifyImportEnded();
}

bool BraveExternalProcessImporterHost::NeedToImportPasswords() const {
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  // Only Brave-to-Brave imports share the OS keychain entry needed to decrypt
  // the source's `Login Data`.
  return !cancelled_ &&
         (items_ & user_data_importer::PASSWORDS) ==
             user_data_importer::PASSWORDS &&
         source_profile_.importer_type == user_data_importer::TYPE_BRAVE;
#else
  return false;
#endif
}

void BraveExternalProcessImporterHost::OnPasswordsImported(
    BravePasswordImporter::Result result,
    size_t /*submitted*/) {
  password_importer_.reset();

  if (result == BravePasswordImporter::Result::kSuccess) {
    // Only report the item as ended on success; reporting it on failure would
    // mark the overall import as succeeded. A successful import of zero
    // credentials just means the source had no saved passwords.
    NotifyImportItemEnded(user_data_importer::PASSWORDS);
  } else {
    // Failures are logged so they are not lost, since the import UI has no
    // per-item failure state.
    LOG(ERROR) << "Brave password import failed, result="
               << static_cast<int>(result);
  }

  // Continue the remainder of the import lifecycle (extensions, if any, then
  // the final "ended" notification) by re-entering NotifyImportEnded() now
  // that `password_importer_` has been cleared.
  NotifyImportEnded();
}

void BraveExternalProcessImporterHost::LaunchImportIfReady() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (NeedToImportExtensions()) {
    if (!extensions_importer_) {
      extensions_importer_ =
          std::make_unique<extensions_import::ExtensionsImporter>(
              source_profile_.source_path, profile_);
      extensions_importer_->Prepare(base::BindOnce(
          &BraveExternalProcessImporterHost::OnExtensionsImportReady,
          weak_ptr_factory_.GetWeakPtr()));
      return;
    }
    if (!extensions_import_ready_) {
      return;
    }
  }
#endif

  if (!do_not_launch_import_for_testing_) {
    CHECK(!client_);
    ExternalProcessImporterHost::LaunchImportIfReady();
  } else {
    NotifyImportEnded();
  }
}

void BraveExternalProcessImporterHost::DoNotLaunchImportForTesting() {
  do_not_launch_import_for_testing_ = true;
}

void BraveExternalProcessImporterHost::NotifyImportEndedForTesting() {
  ExternalProcessImporterHost::NotifyImportEnded();
}

importer::ImporterProgressObserver*
BraveExternalProcessImporterHost::GetObserverForTesting() {
  return observer_;
}

#if BUILDFLAG(ENABLE_EXTENSIONS)

bool BraveExternalProcessImporterHost::NeedToImportExtensions() const {
  return !cancelled_ && (items_ & user_data_importer::EXTENSIONS) ==
                            user_data_importer::EXTENSIONS;
}

void BraveExternalProcessImporterHost::OnExtensionsImportReady(bool ready) {
  if (cancelled_) {
    return;
  }
  if (!ready) {
    extensions_importer_.reset();
    importer::ShowImportLockDialog(
        parent_view_, parent_window_,
        base::BindOnce(
            &BraveExternalProcessImporterHost::OnExtensionsImportLockDialogEnd,
            weak_ptr_factory_.GetWeakPtr()),
        IDS_EXTENSIONS_IMPORTER_LOCK_TITLE, IDS_EXTENSIONS_IMPORTER_LOCK_TEXT);
  } else {
    extensions_import_ready_ = true;
    LaunchImportIfReady();
  }
}

void BraveExternalProcessImporterHost::OnExtensionsImportLockDialogEnd(
    bool is_continue) {
  DCHECK(!extensions_importer_);
  if (is_continue) {
    LaunchImportIfReady();
  } else {
    NotifyImportEnded();
  }
}

void BraveExternalProcessImporterHost::OnExtensionImported(
    const std::string& extension_id,
    extensions_import::ExtensionImportStatus status) {
  if (!extensions_importer_ || !extensions_importer_->IsImportInProgress()) {
    extensions_importer_.reset();
    if (observer_) {
      NotifyImportItemEnded(user_data_importer::EXTENSIONS);
    }
    ExternalProcessImporterHost::NotifyImportEnded();
  }
}

#endif
