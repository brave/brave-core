/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/greaselion/greaselion_service_factory.h"

#include <memory>
#include <string>

#include "base/check_is_test.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "brave/components/greaselion/browser/greaselion_service_impl.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"

namespace greaselion {

namespace {

class GreaselionServiceDelegateImpl
    : public greaselion::GreaselionService::Delegate {
 public:
  explicit GreaselionServiceDelegateImpl(
      content::BrowserContext* browser_context)
      : browser_context_(browser_context) {
    DCHECK(browser_context_);
  }

  bool IsEnabled() const override { return false; }

  void AddExtension(extensions::Extension* extension) override {
    if (auto* extension_service =
            extensions::ExtensionSystem::Get(browser_context_)
                ->extension_service()) {
      extension_service->AddExtension(extension);
    }
  }

  void UnloadExtension(const std::string& extension_id) override {
    if (auto* extension_service =
            extensions::ExtensionSystem::Get(browser_context_)
                ->extension_service()) {
      extension_service->UnloadExtension(
          extension_id, extensions::UnloadedExtensionReason::UPDATE);
    }
  }

 private:
  void UpdateGreaselionExtensions() {
    auto* service =
        GreaselionServiceFactory::GetForBrowserContext(browser_context_);
    if (service) {
      service->UpdateInstalledExtensions();
    }
  }

  raw_ptr<content::BrowserContext> browser_context_;  // Not owned
};

}  // namespace

// static
GreaselionServiceFactory* GreaselionServiceFactory::GetInstance() {
  static base::NoDestructor<GreaselionServiceFactory> instance;
  return instance.get();
}

GreaselionService* GreaselionServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<GreaselionService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

base::FilePath GreaselionServiceFactory::GetInstallDirectory() {
  base::FilePath install_directory;
  base::PathService::Get(chrome::DIR_USER_DATA, &install_directory);
  return install_directory.AppendASCII("Greaselion");
}

GreaselionServiceFactory::GreaselionServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "GreaselionService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(
      extensions::ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
}

GreaselionServiceFactory::~GreaselionServiceFactory() = default;

std::unique_ptr<KeyedService>
GreaselionServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  extensions::ExtensionSystem* extension_system =
      extensions::ExtensionSystem::Get(context);
  extensions::ExtensionRegistry* extension_registry =
      extensions::ExtensionRegistry::Get(context);
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      extensions::GetExtensionFileTaskRunner();
  auto* download_service =
      g_brave_browser_process->greaselion_download_service();
  // download service can be null in tests
  if (!download_service) {
    CHECK_IS_TEST();
    return nullptr;
  }

  return std::make_unique<GreaselionServiceImpl>(
      download_service, GetInstallDirectory(), extension_system,
      extension_registry, task_runner,
      std::make_unique<GreaselionServiceDelegateImpl>(context));
}

bool GreaselionServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace greaselion
