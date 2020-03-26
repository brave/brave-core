/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "brave/browser/importer/brave_profile_writer.h"
#include "brave/common/pref_names.h"

#define ProfileWriter BraveProfileWriter
#define ExternalProcessImporterHost BraveExternalProcessImporterHost

#define BRAVE_IMPORT_DATA                               \
  if (*types->FindBoolKey(kImportDialogCookies)) \
    selected_items |= importer::COOKIES;                \
  if (*types->FindBoolKey(kImportDialogStats))   \
    selected_items |= importer::STATS;                  \
  if (*types->FindBoolKey(kImportDialogLedger))  \
    selected_items |= importer::LEDGER;                 \
  if (*types->FindBoolKey(kImportDialogWindows)) \
    selected_items |= importer::WINDOWS; \
  if (*types->FindBoolKey(kImportDialogExtensions))     \
    selected_items |= importer::EXTENSIONS;

#define BRAVE_SEND_BROWSER_PROFILE_DATA                                     \
  browser_profile->SetBoolean("cookies",                                    \
                              (browser_services & importer::COOKIES) != 0); \
  browser_profile->SetBoolean("stats",                                      \
                              (browser_services & importer::STATS) != 0);   \
  browser_profile->SetBoolean("ledger",                                     \
                              (browser_services & importer::LEDGER) != 0);  \
  browser_profile->SetBoolean("windows",                                    \
                              (browser_services & importer::WINDOWS) != 0); \
  browser_profile->SetBoolean("extensions",                                 \
                              (browser_services & importer::EXTENSIONS) != 0);

#include "../../../../../../../chrome/browser/ui/webui/settings/import_data_handler.cc"
#undef ProfileWriter
#undef ExternalProcessImporterHost
#undef BRAVE_IMPORT_DATA
#undef BRAVE_SEND_BROWSER_PROFILE_DATA
