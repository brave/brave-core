// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_MIGRATOR_H_
#define BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_MIGRATOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/scoped_observation.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "extensions/browser/extension_prefs_observer.h"
#include "extensions/browser/extension_registry_observer.h"

class Profile;

namespace extensions_mv2 {

class ExtensionManifestV2Installer;

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

  void BackupExtensionSettings(const extensions::ExtensionId& cws_extension_id);
  void OnBackupSettingsCompleted(
      const extensions::ExtensionId& cws_extension_id);

  void OnSilentInstall(const extensions::ExtensionId& extension_id,
                       bool success,
                       const std::string& error,
                       extensions::webstore_install::Result result);

  const raw_ptr<Profile> profile_ = nullptr;
  base::ScopedObservation<extensions::ExtensionPrefs,
                          extensions::ExtensionPrefsObserver>
      prefs_observation_{this};
  base::ScopedObservation<extensions::ExtensionRegistry,
                          extensions::ExtensionRegistryObserver>
      registry_observation_{this};

  std::vector<std::unique_ptr<ExtensionManifestV2Installer>> silent_installers_;
  base::WeakPtrFactory<ExtensionsManifectV2Migrator> weak_factory_{this};
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
