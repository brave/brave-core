/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_client.h"
#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "brave/browser/importer/brave_in_process_importer_bridge.h"

BraveExternalProcessImporterHost::BraveExternalProcessImporterHost()
  : ExternalProcessImporterHost() {}

void BraveExternalProcessImporterHost::LaunchImportIfReady() {
  if (waiting_for_bookmarkbar_model_ || template_service_subscription_.get() ||
      !is_source_readable_ || cancelled_)
    return;

  // This is the in-process half of the bridge, which catches data from the IPC
  // pipe and feeds it to the ProfileWriter. The external process half of the
  // bridge lives in the external process (see ProfileImportThread).
  // The ExternalProcessImporterClient created in the next line owns the bridge,
  // and will delete it.
  BraveInProcessImporterBridge* bridge =
      new BraveInProcessImporterBridge(writer_.get(),
                                       weak_ptr_factory_.GetWeakPtr());
  client_ = new BraveExternalProcessImporterClient(
      weak_ptr_factory_.GetWeakPtr(), source_profile_, items_, bridge);
  client_->Start();
}

BraveExternalProcessImporterHost::~BraveExternalProcessImporterHost() {}
