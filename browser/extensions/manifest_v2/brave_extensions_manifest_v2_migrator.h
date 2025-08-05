// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_MIGRATOR_H_
#define BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_MIGRATOR_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/scoped_observation.h"
#include "base/version.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "extensions/browser/extension_prefs_observer.h"
#include "extensions/browser/extension_registry_observer.h"

class Profile;

namespace extensions_mv2 {

class ExtensionsManifestV2Migrator
    : public KeyedService,
      public extensions::ExtensionPrefsObserver,
      public extensions::ExtensionRegistryObserver {
 public:
  explicit ExtensionsManifestV2Migrator(Profile* profile);

  ExtensionsManifestV2Migrator(const ExtensionsManifestV2Migrator&) = delete;
  ExtensionsManifestV2Migrator& operator=(const ExtensionsManifestV2Migrator&) =
      delete;
  ~ExtensionsManifestV2Migrator() override;

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

  void BackupExtensionSettings(
      const extensions::ExtensionId& webstore_extension_id);
  void OnSettingsImported(
      const extensions::ExtensionId& brave_hosted_extension_id);

  const raw_ptr<Profile> profile_ = nullptr;
  base::ScopedObservation<extensions::ExtensionPrefs,
                          extensions::ExtensionPrefsObserver>
      prefs_observation_{this};
  base::ScopedObservation<extensions::ExtensionRegistry,
                          extensions::ExtensionRegistryObserver>
      registry_observation_{this};

  base::WeakPtrFactory<ExtensionsManifestV2Migrator> weak_factory_{this};
};

class ExtensionsManifestV2MigratorFactory : public ProfileKeyedServiceFactory {
 public:
  ExtensionsManifestV2MigratorFactory(
      const ExtensionsManifestV2MigratorFactory&) = delete;
  ExtensionsManifestV2MigratorFactory& operator=(
      const ExtensionsManifestV2MigratorFactory&) = delete;

  static ExtensionsManifestV2MigratorFactory* GetInstance();
  static ExtensionsManifestV2Migrator* GetForBrowserContextForTesting(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<
      ExtensionsManifestV2MigratorFactory>;

  ExtensionsManifestV2MigratorFactory();
  ~ExtensionsManifestV2MigratorFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace extensions_mv2

#endif  // BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_MIGRATOR_H_
