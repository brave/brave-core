/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/task/thread_pool.h"
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/common/importer/chrome_importer_utils.h"
#include "brave/common/importer/importer_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/importer/extensions_import_helpers.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "extensions/browser/extension_file_task_runner.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_EXTENSIONS)

// Silent installer via websotre w/o any prompt or bubble.
class WebstoreInstallerForImporting
    : public extensions::WebstoreInstallWithPrompt {
 public:
  using WebstoreInstallWithPrompt::WebstoreInstallWithPrompt;

 private:
  ~WebstoreInstallerForImporting() override = default;

  std::unique_ptr<ExtensionInstallPrompt::Prompt>
      CreateInstallPrompt() const override {
    return nullptr;
  }
  bool ShouldShowPostInstallUI() const override { return false; }
};
#endif

}  // namespace

BraveExternalProcessImporterHost::BraveExternalProcessImporterHost()
    : weak_ptr_factory_(this) {}
BraveExternalProcessImporterHost::~BraveExternalProcessImporterHost() = default;

#if BUILDFLAG(ENABLE_EXTENSIONS)
void BraveExternalProcessImporterHost::LaunchExtensionsImport() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&GetImportableChromeExtensionsList,
                     source_profile_.source_path),
      base::BindOnce(
          &BraveExternalProcessImporterHost::OnGetChromeExtensionsList,
          weak_ptr_factory_.GetWeakPtr()));
}

void BraveExternalProcessImporterHost::OnExtensionInstalled(
    const std::string& extension_id,
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  if (success) {
    return;
  }
  VLOG(1) << "Extension " << extension_id << " import failed";
  extensions::GetExtensionFileTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&brave::RemoveExtensionsSettings, profile_->GetPath(),
                     extension_id),
      base::BindOnce(
          &BraveExternalProcessImporterHost::OnExtensionSettingsRemoved,
          weak_ptr_factory_.GetWeakPtr(), extension_id));
}

void BraveExternalProcessImporterHost::OnExtensionSettingsRemoved(
    const std::string& extension_id) {
  if (settings_removed_callback_for_testing_)
    settings_removed_callback_for_testing_.Run();
}

void BraveExternalProcessImporterHost::SetSettingsRemovedCallbackForTesting(
    base::RepeatingClosure callback) {
  settings_removed_callback_for_testing_ = std::move(callback);
}

void BraveExternalProcessImporterHost::InstallExtension(const std::string& id) {
  if (install_extension_callback_for_testing_) {
    install_extension_callback_for_testing_.Run(id);
    return;
  }

  scoped_refptr<WebstoreInstallerForImporting> installer =
      new WebstoreInstallerForImporting(
          id, profile_, /*parent_window=*/gfx::NativeWindow(),
          base::BindOnce(
              &BraveExternalProcessImporterHost::OnExtensionInstalled,
              weak_ptr_factory_.GetWeakPtr(), id));
  installer->BeginInstall();
}

void BraveExternalProcessImporterHost::ImportExtensions(
    std::vector<std::string> ids) {
  for (const auto& id : ids) {
    InstallExtension(id);
  }

  if (!ids.empty() && observer_)
    observer_->ImportItemEnded(importer::EXTENSIONS);

  ExternalProcessImporterHost::NotifyImportEnded();
}

void BraveExternalProcessImporterHost::OnGetChromeExtensionsList(
    std::optional<std::vector<std::string>> extensions_list) {
  if (!extensions_list.has_value()) {
    ExternalProcessImporterHost::NotifyImportEnded();
    return;
  }
  const auto ids = extensions_list.value();
  if (ids.empty()) {
    ExternalProcessImporterHost::NotifyImportEnded();
    return;
  }
  extensions::GetExtensionFileTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&brave::ImportStorages, source_profile_.source_path,
                     profile_->GetPath(), ids),
      base::BindOnce(&BraveExternalProcessImporterHost::ImportExtensions,
                     weak_ptr_factory_.GetWeakPtr(), ids));
}

void BraveExternalProcessImporterHost::NotifyImportEnded() {
  if (!cancelled_)
    RecordImporterP3A(source_profile_.importer_type);

  // If user chooses extension importing, start importing extensions.
  // and NotifyImportEnded() will be called from OnGetChromeExtensionsList().
  // Handling extensions importing after finishing all other properties makes
  // logic simpler.
  // Don't import if cancelled.
  if (!cancelled_ && (items_ & importer::EXTENSIONS)) {
    LaunchExtensionsImport();
    return;
  }

  // Otherwise, notifying here and importing is finished.
  ExternalProcessImporterHost::NotifyImportEnded();
}

void BraveExternalProcessImporterHost::LaunchImportIfReady() {
  if (do_not_launch_import_for_testing_)
    return;
  ExternalProcessImporterHost::LaunchImportIfReady();
}

void BraveExternalProcessImporterHost::DoNotLaunchImportForTesting() {
  do_not_launch_import_for_testing_ = true;
}

void BraveExternalProcessImporterHost::SetInstallExtensionCallbackForTesting(
    MockedInstallCallback callback) {
  install_extension_callback_for_testing_ = std::move(callback);
}

void BraveExternalProcessImporterHost::NotifyImportEndedForTesting() {
  ExternalProcessImporterHost::NotifyImportEnded();
}

importer::ImporterProgressObserver*
BraveExternalProcessImporterHost::GetObserverForTesting() {
  return observer_;
}

#endif
