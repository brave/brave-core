/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/extension_system.h"

void BraveDefaultExtensionsHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  web_ui()->RegisterMessageCallback(
      "setWebTorrentEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetWebTorrentEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setHangoutsEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetHangoutsEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setIPFSCompanionEnabled",
      base::BindRepeating(
        &BraveDefaultExtensionsHandler::SetIPFSCompanionEnabled,
        base::Unretained(this)));
}

void BraveDefaultExtensionsHandler::SetWebTorrentEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool value;
  args->GetBoolean(0, &value);

  extensions::ExtensionService* service =
    extensions::ExtensionSystem::Get(profile_)->extension_service();
  extensions::ComponentLoader* loader = service->component_loader();

  if (value) {
    if (!loader->Exists(brave_webtorrent_extension_id)) {
      base::FilePath brave_webtorrent_path(FILE_PATH_LITERAL(""));
      brave_webtorrent_path =
        brave_webtorrent_path.Append(FILE_PATH_LITERAL("brave_webtorrent"));
      loader->Add(IDR_BRAVE_WEBTORRENT, brave_webtorrent_path);
    }
    service->EnableExtension(brave_webtorrent_extension_id);
  } else {
    service->DisableExtension(brave_webtorrent_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_BLOCKED_BY_POLICY);
  }
}

void BraveDefaultExtensionsHandler::SetHangoutsEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool value;
  args->GetBoolean(0, &value);

  extensions::ExtensionService* service =
    extensions::ExtensionSystem::Get(profile_)->extension_service();

  if (value) {
    extensions::ComponentLoader* loader = service->component_loader();
    if (!loader->Exists(hangouts_extension_id)) {
      static_cast<extensions::BraveComponentLoader*>(loader)->
          ForceAddHangoutServicesExtension();
    }
    service->EnableExtension(hangouts_extension_id);
  } else {
    service->DisableExtension(hangouts_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_BLOCKED_BY_POLICY);
  }
}

void BraveDefaultExtensionsHandler::SetIPFSCompanionEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool value;
  args->GetBoolean(0, &value);

  extensions::ExtensionService* service =
    extensions::ExtensionSystem::Get(profile_)->extension_service();

  if (value) {
    extensions::ComponentLoader* loader = service->component_loader();
    if (!loader->Exists(ipfs_companion_extension_id)) {
      static_cast<extensions::BraveComponentLoader*>(loader)->
          AddExtension(ipfs_companion_extension_id,
                       ipfs_companion_extension_name,
                       ipfs_companion_extension_public_key);
    }
    service->EnableExtension(ipfs_companion_extension_id);
  } else {
    service->DisableExtension(ipfs_companion_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_BLOCKED_BY_POLICY);
  }
}
