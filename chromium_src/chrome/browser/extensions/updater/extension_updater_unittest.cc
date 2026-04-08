// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "brave/browser/extensions/updater/features.h"

#include <chrome/browser/extensions/updater/extension_updater_unittest.cc>

// `kBraveAutoUpdateExtensions` is enabled by default but can be disabled via
// CLI using --disable-features=BraveAutoUpdateExtensions or by toggling:
// brave://flags/#brave-user-extension-auto-update
//
// These tests checking the two methods that we patch to allow disabling.
// Manually initiated updates (ex: CheckNow) are unaffected.

namespace extensions {

// When the feature is disabled, Start() must not schedule any update checks.
// Specifically, CheckSoon() must not be called, and no update should start.
TEST_F(ExtensionUpdaterTest, BraveExtensionAutoUpdateDisabledStart) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kBraveAutoUpdateExtensions);

  ExtensionDownloaderTestHelper helper;
  TestDownloaderFactory factory(helper.url_loader_factory());
  TestCrxInstallerFactory crx_installer_factory;
  ExtensionUpdater updater(profile());
  updater.InitAndEnable(extension_prefs(), pref_service(), kUpdateFrequency,
                        nullptr, factory.GetDownloaderFactory());
  updater.set_crx_installer_factory_for_test(&crx_installer_factory);

  NiceMock<MockUpdateService> update_service;
  OverrideUpdateService(&updater, &update_service);
  EXPECT_CALL(update_service, StartUpdateCheck(_, _, _)).Times(0);

  updater.Start();

  EXPECT_FALSE(updater.WillCheckSoon());
  RunUntilIdle();
}

// When the feature is disabled, NextCheck() must not trigger CheckNow() or
// reschedule itself. SimulateTimerFired() calls NextCheck() directly, as if
// the scheduler fired, so Start() need only set alive_ = true.
TEST_F(ExtensionUpdaterTest, BraveExtensionAutoUpdateDisabledNextCheck) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kBraveAutoUpdateExtensions);

  ExtensionDownloaderTestHelper helper;
  TestDownloaderFactory factory(helper.url_loader_factory());
  TestCrxInstallerFactory crx_installer_factory;
  ExtensionUpdater updater(profile());
  updater.InitAndEnable(extension_prefs(), pref_service(), kUpdateFrequency,
                        nullptr, factory.GetDownloaderFactory());
  updater.set_crx_installer_factory_for_test(&crx_installer_factory);

  NiceMock<MockUpdateService> update_service;
  OverrideUpdateService(&updater, &update_service);
  EXPECT_CALL(update_service, StartUpdateCheck(_, _, _)).Times(0);

  // Start() sets alive_ = true before the feature guard fires, so NextCheck()
  // will get past the alive_ check and hit the feature guard.
  updater.Start();

  // Simulate the scheduler calling NextCheck() directly.
  SimulateTimerFired(&updater);
  RunUntilIdle();
}

}  // namespace extensions
