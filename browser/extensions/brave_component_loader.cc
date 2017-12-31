// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
  Add(IDR_BRAVE_EXTENSON, base::FilePath(FILE_PATH_LITERAL("brave-extension")));
}

}  // namespace extensions
