/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_

#include <cstddef>
#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/importer/brave_password_importer.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
namespace extensions_import {
class ExtensionsImporter;
enum class ExtensionImportStatus : int32_t;
}  // namespace extensions_import
#endif

class BraveExternalProcessImporterHost : public ExternalProcessImporterHost {
 public:
  BraveExternalProcessImporterHost();
  BraveExternalProcessImporterHost(const BraveExternalProcessImporterHost&) =
      delete;
  BraveExternalProcessImporterHost& operator=(
      const BraveExternalProcessImporterHost&) = delete;

 private:
  friend class ExternalProcessImporterHost;
  friend class BraveExternalProcessImporterHostUnitTest;

  FRIEND_TEST_ALL_PREFIXES(BraveImporterObserverUnitTest, ImportEvents);
  FRIEND_TEST_ALL_PREFIXES(BraveImporterObserverUnitTest, DestroyObserverEarly);

  ~BraveExternalProcessImporterHost() override;

  void DoNotLaunchImportForTesting();
  void NotifyImportEndedForTesting();
  importer::ImporterProgressObserver* GetObserverForTesting();

  // ExternalProcessImporterHost overrides:
  void NotifyImportEnded() override;
  void LaunchImportIfReady() override;

  // Passwords from another Brave installation are imported in the browser
  // process (not the utility process) so we can use OSCryptAsync to decrypt
  // the source's `Login Data`. Supported on macOS and Linux only; see
  // `BravePasswordImporter`.
  bool NeedToImportPasswords() const;
  void OnPasswordsImported(BravePasswordImporter::Result result,
                           size_t submitted);

  std::unique_ptr<BravePasswordImporter> password_importer_;
  bool password_import_started_ = false;
  bool p3a_recorded_ = false;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  bool NeedToImportExtensions() const;

  void OnExtensionsImportReady(bool ready);
  void OnExtensionsImportLockDialogEnd(bool is_continue);
  void OnExtensionImported(const std::string& extension_id,
                           extensions_import::ExtensionImportStatus status);

  std::unique_ptr<extensions_import::ExtensionsImporter> extensions_importer_;
  bool extensions_import_ready_ = false;
#endif

  bool do_not_launch_import_for_testing_ = false;
  // Vends weak pointers for the importer to call us back.
  base::WeakPtrFactory<BraveExternalProcessImporterHost> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
