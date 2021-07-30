/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_functional_test.h"

#include "base/path_service.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/common/mojom/manifest.mojom.h"

#include "brave/common/brave_paths.h"
#include "chrome/test/base/ui_test_utils.h"

namespace extensions {

scoped_refptr<const Extension>
ExtensionFunctionalTest::InstallExtensionSilently(
    ExtensionService* service,
    const base::FilePath& path) {
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  size_t num_before = registry->enabled_extensions().size();

  TestExtensionRegistryObserver registry_observer(registry);
  scoped_refptr<CrxInstaller> installer(CrxInstaller::CreateSilent(service));
  installer->set_is_gallery_install(false);
  installer->set_allow_silent_install(true);
  installer->set_install_source(extensions::mojom::ManifestLocation::kInternal);
  installer->set_off_store_install_allow_reason(
      CrxInstaller::OffStoreInstallAllowedInTest);

  observer_->Watch(NOTIFICATION_CRX_INSTALLER_DONE,
                   content::Source<CrxInstaller>(installer.get()));
  installer->InstallCrx(path);
  observer_->Wait();
  observer_->WaitForExtensionViewsToLoad();

  size_t num_after = registry->enabled_extensions().size();
  EXPECT_EQ(num_before + 1, num_after);

  scoped_refptr<const Extension> extension =
      registry_observer.WaitForExtensionReady();
  EXPECT_TRUE(extension);
  return extension;
}

void ExtensionFunctionalTest::SetUp() {
  InitEmbeddedTestServer();
  ExtensionBrowserTest::SetUp();
}

void ExtensionFunctionalTest::InitEmbeddedTestServer() {
  brave::RegisterPathProvider();
  base::FilePath test_data_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
  ASSERT_TRUE(embedded_test_server()->Start());
}

void ExtensionFunctionalTest::GetTestDataDir(base::FilePath* test_data_dir) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
}

}  // namespace extensions
