/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/task/sequenced_task_runner.h"
#include "base/version.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "extensions/common/extension_id.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace base {
class SequencedTaskRunner;
}

namespace extensions {
class Extension;
class ExtensionRegistry;
class ExtensionService;
class ExtensionSystem;
}  // namespace extensions

namespace greaselion {

class GreaselionServiceImpl : public GreaselionService,
                              public GreaselionDownloadService::Observer {
 public:
  explicit GreaselionServiceImpl(
      GreaselionDownloadService* download_service,
      const base::FilePath& install_directory,
      extensions::ExtensionSystem* extension_system,
      extensions::ExtensionRegistry* extension_registry,
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  GreaselionServiceImpl(const GreaselionServiceImpl&) = delete;
  GreaselionServiceImpl& operator=(const GreaselionServiceImpl&) = delete;
  ~GreaselionServiceImpl() override;

  // KeyedService overrides
  void Shutdown() override;

  // GreaselionService overrides
  void SetFeatureEnabled(GreaselionFeature feature, bool enabled) override;
  void UpdateInstalledExtensions() override;
  bool IsGreaselionExtension(const std::string& id) override;
  std::vector<extensions::ExtensionId> GetExtensionIdsForTesting() override;
  bool ready() override;
  void AddObserver(GreaselionService::Observer* observer) override;
  void RemoveObserver(GreaselionService::Observer* observer) override;

  // ExtensionRegistryObserver overrides
  void OnExtensionReady(content::BrowserContext* browser_context,
                        const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;

  using GreaselionConvertedExtension =
      std::pair<scoped_refptr<extensions::Extension>, base::FilePath>;

 private:
  void SetBrowserVersionForTesting(const base::Version& version) override;
  void CreateAndInstallExtensions();
  void PostConvert(
      absl::optional<GreaselionConvertedExtension> converted_extension);
  void Install(scoped_refptr<extensions::Extension> extension);
  void MaybeNotifyObservers();

  // GreaselionDownloadService::Observer:
  void OnRulesReady(GreaselionDownloadService* download_service) override;

  raw_ptr<GreaselionDownloadService> download_service_ = nullptr;  // NOT OWNED
  GreaselionFeatures state_;
  const base::FilePath install_directory_;
  raw_ptr<extensions::ExtensionSystem> extension_system_ =
      nullptr;  // NOT OWNED
  raw_ptr<extensions::ExtensionService> extension_service_ =
      nullptr;  // NOT OWNED
  raw_ptr<extensions::ExtensionRegistry> extension_registry_ =
      nullptr;  // NOT OWNED
  bool all_rules_installed_successfully_;
  bool update_in_progress_;
  bool update_pending_;
  int pending_installs_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::ObserverList<GreaselionService::Observer> observers_;
  std::vector<extensions::ExtensionId> greaselion_extensions_;
  std::vector<base::FilePath> extension_dirs_;
  base::Version browser_version_;
  base::WeakPtrFactory<GreaselionServiceImpl> weak_factory_;
};

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_
