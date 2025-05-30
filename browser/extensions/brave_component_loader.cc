/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_component_updater/browser/brave_component_installer.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_extension/grit/brave_extension.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/mojom/manifest.mojom.h"
#include "ui/base/resource/resource_bundle.h"

using extensions::mojom::ManifestLocation;

namespace extensions {

BraveComponentLoader::BraveComponentLoader(Profile* profile)
    : ComponentLoader(profile),
      profile_(profile),
      profile_prefs_(profile->GetPrefs()) {
  pref_change_registrar_.Init(profile_prefs_);

  pref_change_registrar_.Add(
      kWebDiscoveryEnabled,
      base::BindRepeating(&BraveComponentLoader::UpdateBraveExtension,
                          base::Unretained(this)));
}

BraveComponentLoader::~BraveComponentLoader() = default;

void BraveComponentLoader::OnComponentRegistered(std::string extension_id) {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()->EnsureInstalled(
      extension_id);
}

void BraveComponentLoader::OnComponentReady(std::string extension_id,
                                            bool allow_file_access,
                                            const base::FilePath& install_dir,
                                            const std::string& manifest) {
  Add(manifest, install_dir);
  if (allow_file_access) {
    ExtensionPrefs::Get(profile_)->SetAllowFileAccess(extension_id, true);
  }
}

void BraveComponentLoader::AddExtension(const std::string& extension_id,
                                        const std::string& name,
                                        const std::string& public_key) {
  brave_component_updater::RegisterComponent(
      g_browser_process->component_updater(), name, public_key,
      base::BindOnce(&BraveComponentLoader::OnComponentRegistered,
                     base::Unretained(this), extension_id),
      base::BindRepeating(&BraveComponentLoader::OnComponentReady,
                          base::Unretained(this), extension_id, true));
}

void BraveComponentLoader::AddDefaultComponentExtensions(
    bool skip_session_components) {
  ComponentLoader::AddDefaultComponentExtensions(skip_session_components);
  UpdateBraveExtension();
}

bool BraveComponentLoader::UseBraveExtensionBackgroundPage() {
  // Keep sync with `pref_change_registrar_` in the ctor.
  return profile_prefs_->GetBoolean(kWebDiscoveryEnabled);
}

void BraveComponentLoader::UpdateBraveExtension() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDisableBraveExtension)) {
    return;
  }

  base::FilePath brave_extension_path(FILE_PATH_LITERAL(""));
  brave_extension_path =
      brave_extension_path.Append(FILE_PATH_LITERAL("brave_extension"));
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  std::optional<base::Value::Dict> manifest = base::JSONReader::ReadDict(
      resource_bundle.LoadDataResourceString(IDR_BRAVE_EXTENSION));
  CHECK(manifest) << "invalid Brave Extension manifest";

  // The background page is a conditional. Replace MAYBE_background in the
  // manifest to "background" or remove it.
  auto background_value = manifest->Extract("MAYBE_background");
  if (UseBraveExtensionBackgroundPage() && background_value) {
    manifest->Set("background", std::move(*background_value));
  }

  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(profile_);
  const Extension* current_extension =
      registry->GetInstalledExtension(brave_extension_id);

  if (current_extension) {
    const auto* current_manifest = current_extension->manifest();
    if (current_manifest && *current_manifest->value() == *manifest) {
      return;  // Skip reload, nothing is actually changed.
    }
    Remove(brave_extension_id);
  }

  const auto id = Add(std::move(*manifest), brave_extension_path);
  CHECK_EQ(id, brave_extension_id);
}

}  // namespace extensions
