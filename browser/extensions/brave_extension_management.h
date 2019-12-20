/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_MANAGEMENT_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_MANAGEMENT_H_

#include "base/scoped_observer.h"
#include "chrome/browser/extensions/extension_management.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"

namespace extensions {

class BraveExtensionManagement : public ExtensionManagement,
                                 public ExtensionRegistryObserver {
 public:
  explicit BraveExtensionManagement(Profile* profile);
  ~BraveExtensionManagement() override;

 private:
  void RegisterBraveExtensions();
  void CleanupBraveExtensions();

  // ExtensionRegistryObserver implementation.
  void OnExtensionLoaded(
      content::BrowserContext* browser_context,
      const Extension* extension) override;
  void OnExtensionUnloaded(
      content::BrowserContext* browser_context,
      const Extension* extension,
      UnloadedExtensionReason reason) override;

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
    extension_registry_observer_;

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BraveExtensionManagement);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_MANAGEMENT_H_
