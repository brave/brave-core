/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"

#include <optional>

#include "base/run_loop.h"
#include "base/test/scoped_path_override.h"
#include "base/version.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/components/widevine/constants.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/widevine/cdm/buildflags.h"

// We don't bundle. Only use widevine as a component.
TEST(WidevineBuildFlag, FlagTest) {
  EXPECT_TRUE(
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
      false);
#else
      true);
#endif

  EXPECT_TRUE(
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
      true);
#else
      false);
#endif
}

namespace component_updater {

class WidevineCdmComponentInstallerTest : public testing::Test {
 public:
  void SetUp() override {
    // RegisterWidevineCdmComponent() is a no-op until the user has opted in.
    SetWidevineEnabled(true);
  }

  void TearDown() override { SetWidevineEnabled(false); }

  // Registers the Widevine component against a mock service and returns what
  // the installer policy produced.
  std::optional<ComponentRegistration> RegisterAndCaptureComponent() {
    testing::NiceMock<MockComponentUpdateService> cus;
    // ComponentInstaller compares these against the installed version, and
    // base::Version::CompareTo() DCHECKs on invalid operands. CrxUpdateService
    // returns kNullVersion when it has nothing recorded; the mock's default
    // return is an invalid Version, so mirror the real service here.
    ON_CALL(cus, GetRegisteredVersion(testing::_))
        .WillByDefault(testing::Return(base::Version(kNullVersion)));
    ON_CALL(cus, GetMaxPreviousProductVersion(testing::_))
        .WillByDefault(testing::Return(base::Version(kNullVersion)));

    std::optional<ComponentRegistration> registration;
    ON_CALL(cus, RegisterComponent(testing::_))
        .WillByDefault([&registration](const ComponentRegistration& component) {
          registration = component;
          return true;
        });

    base::RunLoop run_loop;
    RegisterWidevineCdmComponent(&cus, run_loop.QuitClosure());
    run_loop.Run();
    return registration;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  // ComponentInstaller reads and creates the component's install directory
  // while registering; keep that out of the real user data directory.
  base::ScopedPathOverride component_dir_override_{DIR_COMPONENT_USER};
};

// The Widevine CRX is fetched from Google's servers. Brave overrides upstream's
// RequiresNetworkEncryption() so that fetch can't fall back to plaintext HTTP,
// which would otherwise announce on the wire that this user is installing
// Widevine. Assert on the registration itself rather than on the policy class,
// so that a wrapper around the policy, or an upstream refactor of how the
// installer is registered, can't silently drop the override.
TEST_F(WidevineCdmComponentInstallerTest, RequiresNetworkEncryption) {
  const std::optional<ComponentRegistration> registration =
      RegisterAndCaptureComponent();

  ASSERT_TRUE(registration.has_value());
  ASSERT_EQ(kWidevineComponentId, registration->app_id);
  EXPECT_TRUE(registration->requires_network_encryption);
}

}  // namespace component_updater
