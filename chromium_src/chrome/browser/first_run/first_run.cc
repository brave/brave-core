/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/importer/importer_list.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/importer/importer_data_types.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#include "../../../../../chrome/browser/first_run/first_run.cc"

namespace brave {

void AutoImportMuon() {
  base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switches::kUpgradeFromMuon))
    return;

  PrefService* local_state = g_browser_process->local_state();
  if (local_state->GetBoolean(kMigratedMuonProfile)) {
    LOG(WARNING) << "Muon profile already migrated, ignoring --upgrade-from-muon";
    return;
  }

  LOG(INFO) << "Auto-importing Muon profile";

  base::RunLoop run_loop;
  auto importer_list = std::make_unique<ImporterList>();
  importer_list->DetectSourceProfiles(
      g_browser_process->GetApplicationLocale(),
      false,  // include_interactive_profiles
      run_loop.QuitClosure());
  run_loop.Run();

  bool brave_profile_found = false;
  size_t brave_profile_index = 0;
  for (size_t i = 0; i < importer_list->count(); i++) {
    const auto& source_profile = importer_list->GetSourceProfileAt(i);
    if (source_profile.importer_type == importer::TYPE_BRAVE) {
      brave_profile_found = true;
      brave_profile_index = i;
      break;
    }
  }
  if (!brave_profile_found) {
    LOG(INFO) << "Muon profile not found";
    return;
  }

  const importer::SourceProfile& source_profile =
      importer_list->GetSourceProfileAt(brave_profile_index);

  // Import every possible type of data from the Muon profile.
  uint16_t items_to_import = 0;
  items_to_import |= source_profile.services_supported;

  ProfileManager* profile_manager = g_browser_process->profile_manager();
  Profile* target_profile = profile_manager->GetLastUsedProfile();

  ImportFromSourceProfile(source_profile, target_profile, items_to_import);

  // Mark Muon profile as migrated so we don't attempt to import it again.
  local_state->SetBoolean(kMigratedMuonProfile, true);
}

void RegisterPrefsForMuonMigration(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kMigratedMuonProfile, false);
}

}  // namespace brave
