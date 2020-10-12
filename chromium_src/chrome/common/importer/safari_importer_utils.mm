/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define SafariImporterCanImport SafariImporterCanImport_Chromium_Unused

#include "../../../../../chrome/common/importer/safari_importer_utils.mm"

#undef SafariImporterCanImport

// Upstream only shows supported list when browser can access to bookmarks.
// But, we let users know about disk access permission is needed when user
// select any services and we don't have proper access permission about it.
// So, we displays all supported services always.
bool SafariImporterCanImport(const base::FilePath& library_dir,
                             uint16_t* services_supported) {
  base::FilePath safari_dir = library_dir.Append("Safari");

  if (base::PathExists(safari_dir.Append("Bookmarks.plist")))
    *services_supported |= importer::FAVORITES;

  if (base::PathExists(safari_dir.Append("History.db")))
    *services_supported |= importer::HISTORY;

  return *services_supported != importer::NONE;
}
