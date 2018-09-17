/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"

#include "base/command_line.h"
#include "brave/common/brave_switches.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/grit/brave_rewards_resources.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "components/grit/brave_components_resources.h"

namespace extensions {

BraveComponentLoader::BraveComponentLoader(
    ExtensionServiceInterface* extension_service,
    PrefService* profile_prefs,
    PrefService* local_state,
    Profile* profile)
    : ComponentLoader(extension_service, profile_prefs, local_state, profile) {
}

BraveComponentLoader::~BraveComponentLoader() {
}

void BraveComponentLoader::AddDefaultComponentExtensions(
    bool skip_session_components) {
  ComponentLoader::AddDefaultComponentExtensions(skip_session_components);

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switches::kDisableBraveExtension)) {
    base::FilePath brave_extension_path(FILE_PATH_LITERAL(""));
    brave_extension_path =
        brave_extension_path.Append(FILE_PATH_LITERAL("brave_extension"));
    Add(IDR_BRAVE_EXTENSON, brave_extension_path);
  }

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  if (!command_line.HasSwitch(switches::kDisableBraveRewardsExtension)) {
    base::FilePath brave_rewards_path(FILE_PATH_LITERAL(""));
    brave_rewards_path =
        brave_rewards_path.Append(FILE_PATH_LITERAL("brave_rewards"));
    Add(IDR_BRAVE_REWARDS, brave_rewards_path);
  }
#endif

  base::FilePath brave_webtorrent_path(FILE_PATH_LITERAL(""));
  brave_webtorrent_path =
    brave_webtorrent_path.Append(FILE_PATH_LITERAL("brave_webtorrent"));
  Add(IDR_BRAVE_WEBTORRENT, brave_webtorrent_path);
}

}  // namespace extensions
