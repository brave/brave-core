/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_management.h"

#include <memory>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_switches.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/browser/extensions/brave_extension_provider.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/extensions/common/brave_extension_urls.h"
#include "chrome/browser/extensions/external_policy_loader.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/url_pattern.h"
#include "extensions/common/url_pattern_set.h"

namespace extensions {

BraveExtensionManagement::BraveExtensionManagement(Profile* profile)
    : ExtensionManagement(profile),
      extension_registry_observer_(this) {
  extension_registry_observer_.Add(ExtensionRegistry::Get(
        static_cast<content::BrowserContext*>(profile)));
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

void BraveExtensionManagement::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  if (extension->id() == ipfs_companion_extension_id)
    pref_service_->SetBoolean(kIPFSCompanionEnabled, true);

  const int bogus_tab_id = 0;
  const auto contentScriptWithheldUrls =
      extension_urls::BraveProtectedUrls::ContentScriptWithheldUrls();
  for (auto i = contentScriptWithheldUrls.begin();
       i != contentScriptWithheldUrls.end(); i++) {
    const GURL& url = *i;
    if (extension->permissions_data()->
        GetContentScriptAccess(url, bogus_tab_id, nullptr) ==
        extensions::PermissionsData::PageAccess::kAllowed) {

      LOG(ERROR) << "BEFORE: " << extension->permissions_data()->withheld_permissions().scriptable_hosts() << std::endl;
      std::string pattern = url.spec() + "*";

      URLPattern url_pattern(URLPattern::SCHEME_HTTPS |
                             URLPattern::SCHEME_HTTP);
      if (url_pattern.Parse(pattern) != URLPattern::ParseResult::kSuccess) {
        LOG(ERROR) << "Couldn't add " << url.spec() <<
            " to the list of protected URLs." << std::endl;
      }

      URLPatternSet scriptable_hosts;
      scriptable_hosts.AddPattern(url_pattern);
      PermissionSet permission_set(APIPermissionSet(), ManifestPermissionSet(),
                                   URLPatternSet(), scriptable_hosts.Clone());

      const PermissionSet& active_permissions =
          extension->permissions_data()->active_permissions();
      const PermissionSet& withheld_permissions =
          extension->permissions_data()->withheld_permissions();
      extension->permissions_data()->SetPermissions(active_permissions.Clone(),
          PermissionSet::CreateUnion(withheld_permissions, permission_set));
      LOG(ERROR) << "AFTER: " << extension->permissions_data()->withheld_permissions().scriptable_hosts() << std::endl;
    }
  }
}

void BraveExtensionManagement::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  if (extension->id() == ipfs_companion_extension_id)
    pref_service_->SetBoolean(kIPFSCompanionEnabled, false);
}

}  // namespace extensions
