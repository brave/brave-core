/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_

#include <optional>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "extensions/buildflags/buildflags.h"

class BraveExternalProcessImporterHost : public ExternalProcessImporterHost {
 public:
  BraveExternalProcessImporterHost();
  BraveExternalProcessImporterHost(const BraveExternalProcessImporterHost&) =
      delete;
  BraveExternalProcessImporterHost& operator=(
      const BraveExternalProcessImporterHost&) = delete;
  using MockedInstallCallback =
      base::RepeatingCallback<void(const std::string&)>;

 private:
  friend class ExternalProcessImporterHost;
  friend class BraveExternalProcessImporterHostUnitTest;

  FRIEND_TEST_ALL_PREFIXES(BraveImporterObserverUnitTest, ImportEvents);
  FRIEND_TEST_ALL_PREFIXES(BraveImporterObserverUnitTest, DestroyObserverEarly);

  ~BraveExternalProcessImporterHost() override;
  void OnExtensionInstalled(const std::string& extension_id,
                            bool success,
                            const std::string& error,
                            extensions::webstore_install::Result result);
  void ImportExtensions(std::vector<std::string> extensions_list);
  void InstallExtension(const std::string& id);
  void OnExtensionSettingsRemoved(const std::string& extension_id);

  void DoNotLaunchImportForTesting();
  void SetInstallExtensionCallbackForTesting(MockedInstallCallback callback);
  void SetSettingsRemovedCallbackForTesting(base::RepeatingClosure callback);
  void NotifyImportEndedForTesting();
  importer::ImporterProgressObserver* GetObserverForTesting();

  // ExternalProcessImporterHost overrides:
  void NotifyImportEnded() override;
  void LaunchImportIfReady() override;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  void LaunchExtensionsImport();
  void OnGetChromeExtensionsList(
      std::optional<std::vector<std::string>> extensions_list);
#endif
  bool do_not_launch_import_for_testing_ = false;
  MockedInstallCallback install_extension_callback_for_testing_;
  base::RepeatingClosure settings_removed_callback_for_testing_;
  // Vends weak pointers for the importer to call us back.
  base::WeakPtrFactory<BraveExternalProcessImporterHost> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
