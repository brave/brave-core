/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/app/server/win/updater_legacy_idl.h"
#include "chrome/updater/util/win_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace updater {

// Brave's replacement for LegacyCOMClassesTest.CheckLegacyInterfaceIDs in
// upstream's updater_tests, which hardcodes Chromium's GUIDs. The browser
// reaches the updater through these legacy COM classes and interfaces, so their
// GUIDs must never change. Those also in Omaha 3 must match our fork.
TEST(BraveLegacyCOMClassesTest, CheckLegacyInterfaceIDs) {
  EXPECT_EQ(StringFromGuid(__uuidof(GoogleUpdate3WebUserClass)),
            L"{2F78AECB-0A7F-4474-89D5-C325293DE960}");
  EXPECT_EQ(StringFromGuid(__uuidof(GoogleUpdate3WebSystemClass)),
            L"{00B16F95-319A-4F01-AC81-CE69B8F4E387}");
  EXPECT_EQ(StringFromGuid(__uuidof(GoogleUpdate3WebServiceClass)),
            L"{3A9D7221-2278-41DD-930B-C2356B7D3725}");
  EXPECT_EQ(StringFromGuid(__uuidof(PolicyStatusUserClass)),
            L"{02FA9A9C-3856-48A8-A62B-F898C64E45C5}");
  EXPECT_EQ(StringFromGuid(__uuidof(PolicyStatusSystemClass)),
            L"{598BBE98-5919-4392-B62A-50D7115F10A3}");
  EXPECT_EQ(StringFromGuid(__uuidof(ProcessLauncherClass)),
            L"{4C3BA8F3-1264-4BDB-BB2D-CA44734AD00D}");
  EXPECT_EQ(StringFromGuid(__uuidof(IAppVersionWeb)),
            L"{35A4470F-5EEC-4715-A2DC-6AA9F8E21183}");
  EXPECT_EQ(StringFromGuid(__uuidof(ICurrentState)),
            L"{E6836CFF-5949-44BC-B6BE-9C8C48DD8D97}");
  EXPECT_EQ(StringFromGuid(__uuidof(IGoogleUpdate3Web)),
            L"{C9190589-ECEC-43F8-8AEC-62496BB87B26}");
  EXPECT_EQ(StringFromGuid(__uuidof(IAppBundleWeb)),
            L"{852A0F87-D117-4B7C-ABA9-2F76D91BCB9D}");
  EXPECT_EQ(StringFromGuid(__uuidof(IAppWeb)),
            L"{FB43AAD0-DDBA-4D01-A3E0-FAB100E7926B}");
  EXPECT_EQ(StringFromGuid(__uuidof(IAppCommandWeb)),
            L"{19F4616B-B7DD-4B3F-8084-C81C5C77AAA4}");
  EXPECT_EQ(StringFromGuid(__uuidof(IPolicyStatus)),
            L"{10DB7BD5-BD0B-4886-9705-174203FE0ADA}");
  EXPECT_EQ(StringFromGuid(__uuidof(IPolicyStatus2)),
            L"{EFF9CA12-4CD3-474B-B881-CDE1D92F1996}");
  EXPECT_EQ(StringFromGuid(__uuidof(IPolicyStatus3)),
            L"{C974F2DD-CFB8-4466-8E6D-96ED901DAACA}");
  EXPECT_EQ(StringFromGuid(__uuidof(IPolicyStatusValue)),
            L"{931E73FD-D487-4458-AA08-1FF41413377B}");
  EXPECT_EQ(StringFromGuid(__uuidof(IProcessLauncher)),
            L"{70E5ECF5-2CA7-4019-9B23-916789A13C2C}");
  EXPECT_EQ(StringFromGuid(__uuidof(IProcessLauncher2)),
            L"{D5627FC9-E2F0-484B-89A4-5DACFE7FAAD3}");

  // Not in Omaha 3. Brave's values were generated for Omaha 4.
  EXPECT_EQ(StringFromGuid(__uuidof(IPolicyStatus4)),
            L"{1ECC256F-6FD7-48A0-8E5E-4CAB7247E2C2}");
  EXPECT_EQ(StringFromGuid(__uuidof(IPolicyStatus4User)),
            L"{C5DE1B8A-574F-44A8-800E-C9AA0B949CDF}");
  EXPECT_EQ(StringFromGuid(__uuidof(IPolicyStatus4System)),
            L"{B166D491-0A95-4ECD-9535-A1BFAD79EF04}");
  EXPECT_EQ(StringFromGuid(__uuidof(IProcessLauncherSystem)),
            L"{B21B6D77-FD99-4099-8615-2E45844AC08D}");
  EXPECT_EQ(StringFromGuid(__uuidof(IProcessLauncher2System)),
            L"{653E6424-9F84-4988-B56D-C5554C519692}");
}

}  // namespace updater
