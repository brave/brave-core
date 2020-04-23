/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/values.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "extensions/buildflags/buildflags.h"

class BrowserProfileLock;

class BraveExternalProcessImporterHost : public ExternalProcessImporterHost {
 public:
  BraveExternalProcessImporterHost();

 private:
  friend class ExternalProcessImporterHost;

  ~BraveExternalProcessImporterHost() override;

  // ExternalProcessImporterHost overrides:
  void NotifyImportEnded() override;
  bool CheckForFirefoxLock(
      const importer::SourceProfile& source_profile) override;
  // Make sure that Chrome isn't running, if import browser is Chrome
  // or Brave. Show to the user a dialog that notifies that is necessary to
  // close Chrome  prior to continuing the import. Returns false iff
  // import should be aborted.
  bool CheckForChromeLock(
      const importer::SourceProfile& source_profile);

  // ShowWarningDialog() asks user to close the application that is owning the
  // lock. They can continue or skip the importing process.
  // This method should not be called if the importer is in headless mode.
  void ShowWarningDialog();

  // This is called when when user ends the warning dialog by clicking on either
  // the "Skip" or "Continue" buttons. |is_continue| is true when user clicked
  // the "Continue" button.
  void OnImportLockDialogEnd(bool is_continue);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  void LaunchExtensionsImport();
  void OnGetChromeExtensionsList(base::Optional<base::Value> extensions_list);
#endif

  // Chrome profile lock.
  std::unique_ptr<BrowserProfileLock> browser_lock_;

  // Vends weak pointers for the importer to call us back.
  base::WeakPtrFactory<BraveExternalProcessImporterHost> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BraveExternalProcessImporterHost);
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
