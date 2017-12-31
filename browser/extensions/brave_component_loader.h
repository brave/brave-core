// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_LOADER_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_LOADER_H_

#include "chrome/browser/extensions/component_loader.h"

namespace extensions {

// For registering, loading, and unloading component extensions.
class BraveComponentLoader : public ComponentLoader {
 public:
  BraveComponentLoader(ExtensionServiceInterface* extension_service,
                  PrefService* prefs,
                  PrefService* local_state,
                  Profile* browser_context);
  ~BraveComponentLoader() override;

  // Adds the default component extensions. If |skip_session_components|
  // the loader will skip loading component extensions that weren't supposed to
  // be loaded unless we are in signed user session (ChromeOS). For all other
  // platforms this |skip_session_components| is expected to be unset.
  void AddDefaultComponentExtensions(bool skip_session_components) override;

  DISALLOW_COPY_AND_ASSIGN(BraveComponentLoader);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_LOADER_H_
