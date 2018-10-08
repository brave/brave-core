/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"

#include "base/command_line.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/component_updater/brave_component_installer.h"
#include "brave/browser/extensions/brave_component_extension.h"
#include "brave/common/brave_switches.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/extension/grit/brave_rewards_resources.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "brave/components/brave_sync/grit/brave_sync_resources.h"
#include "components/grit/brave_components_resources.h"
#include "extensions/browser/extension_prefs.h"

namespace extensions {

BraveComponentLoader::BraveComponentLoader(
    ExtensionServiceInterface* extension_service,
    PrefService* profile_prefs,
    PrefService* local_state,
    Profile* profile)
    : ComponentLoader(extension_service, profile_prefs, local_state, profile),
      profile_(profile) {
}

BraveComponentLoader::~BraveComponentLoader() {
}

void BraveComponentLoader::OnComponentRegistered(std::string extension_id) {
  ComponentsUI demand_updater;
  // This weird looking call is ok, it is just like this to not need
  // to patch for friend access.
  demand_updater.OnDemandUpdate(g_browser_process->component_updater(),
      extension_id);
}

void BraveComponentLoader::OnComponentReady(std::string extension_id,
    bool allow_file_access,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  Add(manifest, install_dir);
  if (allow_file_access) {
    ExtensionPrefs::Get((content::BrowserContext *)profile_)->
        SetAllowFileAccess(extension_id, true);
  }
}

void BraveComponentLoader::AddExtension(const std::string& extension_id,
    const std::string& name, const std::string& public_key) {
  brave::RegisterComponent(g_browser_process->component_updater(),
    name,
    public_key,
    base::Bind(&BraveComponentLoader::OnComponentRegistered,
        base::Unretained(this), extension_id),
    base::Bind(&BraveComponentLoader::OnComponentReady,
        base::Unretained(this), extension_id, true));
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

  if (!command_line.HasSwitch(switches::kDisablePDFJSExtension)) {
    AddExtension(pdfjs_extension_id, pdfjs_extension_name, pdfjs_extension_public_key);
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
