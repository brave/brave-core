/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_functional_test.h"

#include "base/path_service.h"
#include "base/test/test_future.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/chrome_extension_test_notification_observer.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/test_extension_registry_observer.h"
#include "extensions/browser/user_script_manager.h"
#include "extensions/common/manifest_handlers/content_scripts_handler.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "extensions/common/mojom/manifest.mojom.h"
#include "extensions/test/extension_background_page_waiter.h"
#include "extensions/test/test_content_script_load_waiter.h"

namespace extensions {

scoped_refptr<const Extension>
ExtensionFunctionalTest::InstallExtensionSilently(const char* filename,
                                                  const char* extension_id) {
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  size_t num_before = registry->enabled_extensions().size();

  base::FilePath path = test_data_dir_.AppendASCII(filename);

  TestExtensionRegistryObserver registry_observer(registry);

  scoped_refptr<CrxInstaller> installer(CrxInstaller::CreateSilent(profile()));
  installer->set_is_gallery_install(false);
  installer->set_allow_silent_install(true);
  installer->set_install_source(extensions::mojom::ManifestLocation::kInternal);
  installer->set_off_store_install_allow_reason(
      CrxInstaller::OffStoreInstallAllowedInTest);

  base::test::TestFuture<std::optional<CrxInstallError>> installer_done_future;
  installer->AddInstallerCallback(
      installer_done_future
          .GetCallback<const std::optional<CrxInstallError>&>());
  installer->InstallCrx(path);

  const std::optional<CrxInstallError>& error = installer_done_future.Get();
  EXPECT_FALSE(error);

  scoped_refptr<const Extension> extension =
      registry_observer.WaitForExtensionReady();
  EXPECT_TRUE(extension);
  EXPECT_EQ(extension_id, extension->id());

  size_t num_after = registry->enabled_extensions().size();
  EXPECT_EQ(num_before + 1, num_after);
  EXPECT_TRUE(registry->enabled_extensions().Contains(extension_id));

  UserScriptManager* user_script_manager =
      ExtensionSystem::Get(profile())->user_script_manager();
  if (user_script_manager &&
      !ContentScriptsInfo::GetContentScripts(extension.get()).empty()) {
    ExtensionUserScriptLoader* user_script_loader =
        user_script_manager->GetUserScriptLoaderForExtension(extension_id);
    if (!user_script_loader->HasLoadedScripts()) {
      ContentScriptLoadWaiter waiter(user_script_loader);
      waiter.Wait();
    }
  }

  if (content::RenderProcessHost::GetCurrentRenderProcessCountForTesting() >
      0) {
    content::BrowserContext* context_to_use =
        IncognitoInfo::IsSplitMode(extension.get())
            ? profile()
            : profile()->GetOriginalProfile();

    // If possible, wait for the extension's background context to be loaded.
    std::string reason_unused;
    if (ExtensionBackgroundPageWaiter::CanWaitFor(*extension, reason_unused)) {
      ExtensionBackgroundPageWaiter(context_to_use, *extension)
          .WaitForBackgroundInitialized();
    }
  }

  EXPECT_TRUE(WaitForExtensionViewsToLoad());

  return extension;
}

void ExtensionFunctionalTest::SetUp() {
  InitEmbeddedTestServer();
  ExtensionBrowserTest::SetUp();
}

void ExtensionFunctionalTest::SetUpOnMainThread() {
  ExtensionBrowserTest::SetUpOnMainThread();
  GetTestDataDir(&test_data_dir_);
}

void ExtensionFunctionalTest::InitEmbeddedTestServer() {
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
