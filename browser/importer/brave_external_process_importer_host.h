/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
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
  bool do_not_launch_import_for_testing_ = false;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  bool NeedToImportExtensions() const;

  void OnExtensionsImportReady(bool ready);
  void OnExtensionImported(const std::string& extension_id,
                           extensions_import::ExtensionImportStatus status);

  std::unique_ptr<extensions_import::ExtensionsImporter> extensions_importer_;
#endif
  // Vends weak pointers for the importer to call us back.
  base::WeakPtrFactory<BraveExternalProcessImporterHost> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
