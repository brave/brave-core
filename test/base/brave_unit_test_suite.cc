/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_unit_test_suite.h"

#include "base/logging.h"
#include "brave/common/resource_bundle_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/install_static/product_install_details.h"
#include "chrome/test/base/chrome_unit_test_suite.h"

BraveUnitTestSuite::BraveUnitTestSuite(int argc, char** argv)
    : ChromeUnitTestSuite(argc, argv) {}

void BraveUnitTestSuite::Initialize() {
#if BUILDFLAG(IS_WIN) && defined(OFFICIAL_BUILD)
  // When ChromeExtensionsBrowserClient is initialized, it needs
  install_static::InitializeProductDetailsForPrimaryModule();
#endif
  // This will also add Brave resources bundle via chromium_src override.
  ChromeUnitTestSuite::Initialize();

}
