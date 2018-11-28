/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/importer/brave_importer_utils.h"

#include <memory>
#include <string>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "chrome/common/importer/importer_data_types.h"

bool BraveImporterCanImport(const base::FilePath& profile,
                             uint16_t* services_supported) {
  DCHECK(services_supported);
  *services_supported = importer::NONE;

  base::FilePath session_store =
    profile.Append(base::FilePath::StringType(FILE_PATH_LITERAL("session-store-1")));
  base::FilePath passwords =
    profile.Append(base::FilePath::StringType(FILE_PATH_LITERAL("Login Data")));
  base::FilePath cookies =
    profile.Append(base::FilePath::StringType(FILE_PATH_LITERAL("Cookies")));
  base::FilePath ledger_state =
    profile.Append(base::FilePath::StringType(FILE_PATH_LITERAL("ledger-state.json")));

  if (base::PathExists(session_store))
    *services_supported |= importer::HISTORY | importer::FAVORITES | importer::STATS;
  if (base::PathExists(passwords))
    *services_supported |= importer::PASSWORDS;
  if (base::PathExists(cookies))
    *services_supported |= importer::COOKIES;
  if (base::PathExists(ledger_state))
    *services_supported |= importer::LEDGER;

  return *services_supported != importer::NONE;
}
