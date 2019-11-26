/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_help_tips_handler.h"

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/values.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_wayback_machine/grit/brave_wayback_machine_resources.h"
#include "content/public/browser/web_ui.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_system.h"
#endif

void BraveHelpTipsHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  web_ui()->RegisterMessageCallback(
      "setBraveWaybackMachineEnabled",
      base::BindRepeating(&BraveHelpTipsHandler::SetBraveWaybackMachineEnabled,
                          base::Unretained(this)));
#endif
}

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
void BraveHelpTipsHandler::SetBraveWaybackMachineEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool enabled;
  args->GetBoolean(0, &enabled);

  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  extensions::ComponentLoader* loader = service->component_loader();

  if (enabled) {
    if (!loader->Exists(brave_wayback_machine_extension_id)) {
      base::FilePath brave_wayback_machine_path(FILE_PATH_LITERAL(""));
      brave_wayback_machine_path = brave_wayback_machine_path.Append(
          FILE_PATH_LITERAL("brave_wayback_machine"));
      loader->Add(IDR_BRAVE_WAYBACK_MACHINE, brave_wayback_machine_path);
    }
    service->EnableExtension(brave_wayback_machine_extension_id);
  } else {
    service->DisableExtension(brave_wayback_machine_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_BLOCKED_BY_POLICY);
  }
}
#endif
