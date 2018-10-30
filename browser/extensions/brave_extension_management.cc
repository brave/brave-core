/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_management.h"

#include "base/command_line.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_switches.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/browser/extensions/brave_extension_provider.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "chrome/browser/extensions/external_policy_loader.h"
#include "extensions/common/extension_urls.h"

namespace extensions {

BraveExtensionManagement::BraveExtensionManagement(Profile* profile)
    : ExtensionManagement(profile) {
  providers_.push_back(
      std::make_unique<BraveExtensionProvider>());
  RegisterBraveExtensions();
}

BraveExtensionManagement::~BraveExtensionManagement() {
}

void BraveExtensionManagement::RegisterBraveExtensions() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switches::kDisableTorClientUpdaterExtension))
    g_brave_browser_process->tor_client_updater()->Register();
}

}  // namespace extensions
