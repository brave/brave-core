/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_EXTENSIONS_IMPORT_HELPERS_H_
#define BRAVE_BROWSER_IMPORTER_EXTENSIONS_IMPORT_HELPERS_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "chrome/common/extensions/webstore_install_result.h"

class Profile;

namespace extensions_import {

struct ImportingExtension {
  ImportingExtension();
  ImportingExtension(ImportingExtension&&);
  ImportingExtension& operator=(ImportingExtension&&);
  ~ImportingExtension();

  std::string id;
  bool is_installed = false;
  bool has_local_settings = false;
  scoped_refptr<class WebstoreInstallerForImporting> installer;
};

enum class ExtensionImportStatus : int32_t {
  kOk = 0,
  kFailedToInstall,
  kFailedToImportSettings,
};

class ExtensionsImporter {
 public:
  using OnReady = base::OnceCallback<void(bool ready)>;
  using OnExtensionImported =
      base::RepeatingCallback<void(const std::string& id,
                                   ExtensionImportStatus status)>;

  ExtensionsImporter(const base::FilePath& source_profile,
                     Profile* target_profile);
  ~ExtensionsImporter();

  void Prepare(OnReady on_ready);
  bool Import(OnExtensionImported on_extension);

  const ImportingExtension* GetExtension(const std::string& id) const;
  bool IsImportInProgress() const;

  static base::RepeatingCallback<
      ExtensionImportStatus(const std::string& extension_id)>&
  GetExtensionInstallerForTesting();

 private:
  using ExtensionsListResult =
      base::expected<std::vector<ImportingExtension>, bool>;
  using OnOneExtensionImported =
      base::OnceCallback<void(const std::string& id,
                              ExtensionImportStatus status)>;

  void OnGetExtensionsForImport(OnReady on_ready, ExtensionsListResult result);
  void OnExtensionInstalled(ImportingExtension* extension,
                            OnOneExtensionImported on_extension,
                            bool success,
                            const std::string& error,
                            extensions::webstore_install::Result result);

  void ImportExtensionSettings(ImportingExtension* extension,
                               OnOneExtensionImported on_extension);
  void OnExtensionSettingsImported(ImportingExtension* extension,
                                   OnOneExtensionImported on_extension,
                                   bool success);

  base::FilePath source_profile_;
  raw_ptr<Profile> target_profile_ = nullptr;

  std::vector<ImportingExtension> extensions_;
  size_t in_progress_count_ = 0;

  base::WeakPtrFactory<ExtensionsImporter> weak_factory_{this};
};

}  // namespace extensions_import

#endif  // BRAVE_BROWSER_IMPORTER_EXTENSIONS_IMPORT_HELPERS_H_
