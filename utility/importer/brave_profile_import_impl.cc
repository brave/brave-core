/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_profile_import_impl.h"

#include <utility>

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"

void BraveProfileImportImpl::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t items,
    const base::flat_map<uint32_t, std::string>& localized_strings,
    mojo::PendingRemote<chrome::mojom::ProfileImportObserver> observer) {
  // Signal change to OSCrypt password for importing from Chrome/Chromium
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (base::StartsWith(base::UTF16ToUTF8(source_profile.importer_name),
                       "Chrome", base::CompareCase::SENSITIVE)) {
    command_line->AppendSwitch("import-chrome");
  } else if (base::StartsWith(base::UTF16ToUTF8(source_profile.importer_name),
                              "Chromium", base::CompareCase::SENSITIVE)) {
    command_line->AppendSwitch("import-chromium");
  } else if (base::StartsWith(base::UTF16ToUTF8(source_profile.importer_name),
                              "Brave", base::CompareCase::SENSITIVE)) {
    command_line->AppendSwitch("import-brave");
  }

  ProfileImportImpl::StartImport(
      source_profile, items, localized_strings, std::move(observer));
}
