/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_client.h"
#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/browser/importer/brave_in_process_importer_bridge.h"

#define InProcessImporterBridge BraveInProcessImporterBridge
#define ExternalProcessImporterClient BraveExternalProcessImporterClient

#define BRAVE_LAUNCH_IMPORT_IF_READY \
  RecordImporterP3A(source_profile_.importer_type); \

#define BRAVE_START_IMPORT_SETTINGS \
  if (!static_cast<BraveExternalProcessImporterHost*>(this)-> \
          CheckForChromeOrBraveLock()) { \
    Cancel(); \
    return; \
  }

#include "../../../../../chrome/browser/importer/external_process_importer_host.cc"
#undef BRAVE_LAUNCH_IMPORT_IF_READY
#undef BRAVE_START_IMPORT_SETTINGS
#undef InProcessImporterBridge
#undef ExternalProcessImporterClient
