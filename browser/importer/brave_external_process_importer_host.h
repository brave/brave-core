/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_

#include "chrome/browser/importer/external_process_importer_host.h"

class BraveExternalProcessImporterHost : public ExternalProcessImporterHost {
 public:
  BraveExternalProcessImporterHost();

 private:
  ~BraveExternalProcessImporterHost() override;

  // Launches the utility process that starts the import task, unless bookmark
  // or template model are not yet loaded. If load is not detected, this method
  // will be called when the loading observer sees that model loading is
  // complete.
  void LaunchImportIfReady() override;

  DISALLOW_COPY_AND_ASSIGN(BraveExternalProcessImporterHost);
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
