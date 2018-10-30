/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FIRST_RUN_FIRST_RUN_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FIRST_RUN_FIRST_RUN_H_

#include "../../../../../chrome/browser/first_run/first_run.h"

class PrefRegistrySimple;

namespace brave {

void AutoImportMuon();

// Registers the preferences used to track the state of migration from Muon
void RegisterPrefsForMuonMigration(PrefRegistrySimple* registry);

}  // namespace brave

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FIRST_RUN_FIRST_RUN_H_
