/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, BlacklistExtension) {
  brave::RegisterPathProvider();
  base::FilePath test_data_dir;
  PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  const extensions::Extension* extension =
    InstallExtension(test_data_dir.AppendASCII("should-be-blocked-extension"), 0);
  ASSERT_FALSE(extension);
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, WhitelistedExtension) {
  brave::RegisterPathProvider();
  base::FilePath test_data_dir;
  PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  const extensions::Extension* extension =
    InstallExtension(test_data_dir.AppendASCII("adblock-data"), 1);
  ASSERT_TRUE(extension);
}
