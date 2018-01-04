/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"

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
  Add(IDR_BRAVE_EXTENSON, base::FilePath(FILE_PATH_LITERAL("${root_gen_dir}/brave_extension")));
}

}  // namespace extensions
