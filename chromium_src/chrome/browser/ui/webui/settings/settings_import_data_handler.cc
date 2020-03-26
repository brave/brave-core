/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "brave/common/pref_names.h"

#define ExternalProcessImporterHost BraveExternalProcessImporterHost

#define BRAVE_IMPORT_DATA \
  if (prefs->GetBoolean(kImportDialogExtensions)) \
    selected_items |= importer::EXTENSIONS;

#define BRAVE_SEND_BROWSER_PROFILE_DATA \
  browser_profile->SetBoolean("extensions", \
      (browser_services & importer::EXTENSIONS) != 0);

#include "../../../../../../../chrome/browser/ui/webui/settings/settings_import_data_handler.cc"  // NOLINT
#undef ExternalProcessImporterHost
#undef BRAVE_IMPORT_DATA
#undef BRAVE_SEND_BROWSER_PROFILE_DATA
