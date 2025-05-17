/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_FUNCTIONAL_TEST_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_FUNCTIONAL_TEST_H_

#include "base/files/file_path.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"

namespace extensions {

class ExtensionFunctionalTest : public ExtensionBrowserTest {
 public:
  scoped_refptr<const Extension> InstallExtensionSilently(
      const char* filename,
      const char* extension_id);
  scoped_refptr<const Extension> InstallUnpackedExtensionSilently(
      const char* path,
      const char* public_key,
      const char* extension_id);
  base::FilePath GetTestDataDir() const { return test_data_dir_; }

  // content::BrowserTestBase:
  void SetUp() override;
  void SetUpOnMainThread() override;

 private:
  void InitEmbeddedTestServer();
  void OverrideTestDataDir(base::FilePath* test_data_dir);
  scoped_refptr<const Extension> InstallExtensionSilentlyInternal(
      const char* filename,
      const char* extension_id,
      const char* public_key);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_FUNCTIONAL_TEST_H_
