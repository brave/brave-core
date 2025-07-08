// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_MIGRATOR_H_
#define BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_MIGRATOR_H_

#include <memory>

#include "base/memory/singleton.h"
#include "base/scoped_observation.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "extensions/browser/extension_prefs_observer.h"
#include "extensions/browser/extension_registry_observer.h"

class Profile;

namespace extensions_mv2 {

class ExtensionsManifectV2Migrator
    : public KeyedService,
      public extensions::ExtensionPrefsObserver,
      public extensions::ExtensionRegistryObserver {
 public:
  explicit ExtensionsManifectV2Migrator(Profile* profile);

  ExtensionsManifectV2Migrator(const ExtensionsManifectV2Migrator&) = delete;
  ExtensionsManifectV2Migrator& operator=(const ExtensionsManifectV2Migrator&) =
      delete;
  ~ExtensionsManifectV2Migrator() override;

  void Shutdown() override;

 private:
  // ExtensionPrefsObserver:
  void OnExtensionPrefsWillBeDestroyed(
      extensions::ExtensionPrefs* prefs) override;
  void OnExtensionDisableReasonsChanged(
      const extensions::ExtensionId& extension_id,
      extensions::DisableReasonSet disabled_reasons) override;

  // ExtensionRegistryObserver:
  void OnShutdown(extensions::ExtensionRegistry* registry) override;
  void OnExtensionInstalled(content::BrowserContext* browser_context,
                            const extensions::Extension* extension,
                            bool is_updates) override;

  void BackupExtensionSettings(const extensions::ExtensionId& extension_id);

  const raw_ptr<Profile> profile_ = nullptr;
  base::ScopedObservation<extensions::ExtensionPrefs,
                          extensions::ExtensionPrefsObserver>
      prefs_observation_{this};
  base::ScopedObservation<extensions::ExtensionRegistry,
                          extensions::ExtensionRegistryObserver>
      registry_observation_{this};
};

class ExtensionsManifectV2MigratorFactory : public ProfileKeyedServiceFactory {
 public:
  ExtensionsManifectV2MigratorFactory(
      const ExtensionsManifectV2MigratorFactory&) = delete;
  ExtensionsManifectV2MigratorFactory& operator=(
      const ExtensionsManifectV2MigratorFactory&) = delete;

  static ExtensionsManifectV2MigratorFactory* GetInstance();
  static ExtensionsManifectV2Migrator* GetForBrowserContextForTesting(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<
      ExtensionsManifectV2MigratorFactory>;

  ExtensionsManifectV2MigratorFactory();
  ~ExtensionsManifectV2MigratorFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace extensions_mv2

#endif  // BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_MIGRATOR_H_
