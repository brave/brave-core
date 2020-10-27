/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define SafariImporterCanImport SafariImporterCanImport_ChromiumImpl

#include "../../../../../chrome/common/importer/safari_importer_utils.mm"

#undef SafariImporterCanImport

bool SafariImporterCanImport(const base::FilePath& library_dir,
                             uint16_t* services_supported) {
  SafariImporterCanImport_ChromiumImpl(library_dir, services_supported);

  if (base::PathExists(library_dir.Append("Safari").Append("History.db")))
    *services_supported |= importer::HISTORY;

  return *services_supported != importer::NONE;
}
