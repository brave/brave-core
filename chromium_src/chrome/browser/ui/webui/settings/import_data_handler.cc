/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include "base/values.h"
#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "brave/components/constants/pref_names.h"
#include "components/user_data_importer/common/importer_data_types.h"

namespace {

// Reads the Brave-specific import dialog selections.
//
// NOTE: When we add a new import feature, also consider adding it for
// importing in the welcome page. See IMPORT_BROWSER_DATA_REQUESTED action in
// welcome_reducer.ts.
void AddBraveSelectedItems(const base::DictValue& type_dict,
                           uint16_t& selected_items) {
  if (type_dict.FindBool(kImportDialogExtensions).value_or(false)) {
    selected_items |= user_data_importer::EXTENSIONS;
  }
  if (type_dict.FindBool(kImportDialogPayments).value_or(false)) {
    selected_items |= user_data_importer::PAYMENTS;
  }
}

// Reports the Brave-supported import services to the WebUI.
void SetBraveBrowserProfileServices(base::DictValue& browser_profile,
                                    uint16_t browser_services) {
  browser_profile.Set("extensions",
                      (browser_services & user_data_importer::EXTENSIONS) != 0);
  browser_profile.Set("payments",
                      (browser_services & user_data_importer::PAYMENTS) != 0);
}

}  // namespace

#include <chrome/browser/ui/webui/settings/import_data_handler.cc>
