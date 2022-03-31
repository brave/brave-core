/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/common/importer/chrome_importer_utils.h"
#include "brave/common/importer/importer_constants.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/common/extensions/webstore_install_result.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_EXTENSIONS)
absl::optional<base::Value> GetChromeExtensionsList(
    const base::FilePath& secured_preference_path) {
  if (!base::PathExists(secured_preference_path))
    return absl::nullopt;

  std::string secured_preference_content;
  base::ReadFileToString(secured_preference_path, &secured_preference_content);
  absl::optional<base::Value> secured_preference =
      base::JSONReader::Read(secured_preference_content);
  if (auto* extensions = secured_preference->FindPath(
          kChromeExtensionsListPath)) {
    return extensions->Clone();
  }
  return absl::nullopt;
}

// Silent installer via websotre w/o any prompt or bubble.
class WebstoreInstallerForImporting
    : public extensions::WebstoreInstallWithPrompt {
 public:
  using WebstoreInstallWithPrompt::WebstoreInstallWithPrompt;

 private:
  ~WebstoreInstallerForImporting() override {}

  std::unique_ptr<ExtensionInstallPrompt::Prompt>
      CreateInstallPrompt() const override {
    return nullptr;
  }
  bool ShouldShowAppInstalledBubble() const override { return false; }
  bool ShouldShowPostInstallUI() const override { return false; }
};
#endif

}  // namespace

BraveExternalProcessImporterHost::BraveExternalProcessImporterHost()
    : ExternalProcessImporterHost(),
      weak_ptr_factory_(this) {}
BraveExternalProcessImporterHost::~BraveExternalProcessImporterHost() = default;

#if BUILDFLAG(ENABLE_EXTENSIONS)
void BraveExternalProcessImporterHost::LaunchExtensionsImport() {
  DCHECK_EQ(importer::TYPE_CHROME, source_profile_.importer_type);

  const base::FilePath pref_file = source_profile_.source_path.AppendASCII(
      kChromeExtensionsPreferencesFile);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&GetChromeExtensionsList, pref_file),
      base::BindOnce(
          &BraveExternalProcessImporterHost::OnGetChromeExtensionsList,
          weak_ptr_factory_.GetWeakPtr()));
}

void BraveExternalProcessImporterHost::OnGetChromeExtensionsList(
    absl::optional<base::Value> extensions_list) {
  if (!extensions_list || !extensions_list->is_dict()) {
    ExternalProcessImporterHost::NotifyImportEnded();
    return;
  }

  const auto ids =
      GetImportableListFromChromeExtensionsList(extensions_list.value());
  for (const auto& id : ids) {
    scoped_refptr<WebstoreInstallerForImporting> installer =
        // Ignore installation result.
        // TODO(simonhong): Should we check install failure and retry?
        new WebstoreInstallerForImporting(
            id, profile_,
            base::BindOnce([] (bool, const std::string&,
                               extensions::webstore_install::Result) {}));
    installer->BeginInstall();
  }

  if (!ids.empty() && observer_)
    observer_->ImportItemEnded(importer::EXTENSIONS);

  ExternalProcessImporterHost::NotifyImportEnded();
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
#endif
