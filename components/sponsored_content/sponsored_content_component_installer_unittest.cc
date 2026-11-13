/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/sponsored_content_component_installer.h"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"
#include "brave/components/sponsored_content/sponsored_content_component_public_keys.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/crx_file/id_util.h"
#include "components/update_client/update_client.h"
#include "crypto/hash.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sponsored_content {

namespace {

constexpr char kComponentName[] = "Sponsored Content (US)";

}  // namespace

class SponsoredContentComponentInstallerPolicyTest : public testing::Test {
 protected:
  std::array<uint8_t, crypto::hash::kSha256Size> component_public_key_sha256_ =
      *GetComponentPublicKeySHA256("US");
};

TEST_F(SponsoredContentComponentInstallerPolicyTest, ReturnsGivenName) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  EXPECT_EQ(kComponentName, policy.GetName());
}

TEST_F(SponsoredContentComponentInstallerPolicyTest, ReturnsGivenHash) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  std::vector<uint8_t> hash;
  policy.GetHash(&hash);

  EXPECT_THAT(hash, testing::ElementsAreArray(component_public_key_sha256_));
}

TEST_F(SponsoredContentComponentInstallerPolicyTest,
       RelativeInstallDirMatchesCrxIdForHash) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  EXPECT_EQ(
      base::FilePath::FromUTF8Unsafe(
          crx_file::id_util::GenerateIdFromHash(component_public_key_sha256_)),
      policy.GetRelativeInstallDir());
}

TEST_F(SponsoredContentComponentInstallerPolicyTest, IsABraveComponent) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  EXPECT_TRUE(policy.IsBraveComponent());
}

TEST_F(SponsoredContentComponentInstallerPolicyTest,
       DoesNotRequireNetworkEncryption) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  EXPECT_FALSE(policy.RequiresNetworkEncryption());
}

TEST_F(SponsoredContentComponentInstallerPolicyTest,
       SupportsGroupPolicyEnabledComponentUpdates) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  EXPECT_TRUE(policy.SupportsGroupPolicyEnabledComponentUpdates());
}

TEST_F(SponsoredContentComponentInstallerPolicyTest, AlwaysVerifies) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  EXPECT_TRUE(policy.VerifyInstallation(base::DictValue(), base::FilePath()));
}

TEST_F(SponsoredContentComponentInstallerPolicyTest, OnCustomInstallSucceeds) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  EXPECT_EQ(update_client::ErrorCategory::kNone,
            policy.OnCustomInstall(base::DictValue(), base::FilePath())
                .result.category);
}

TEST_F(SponsoredContentComponentInstallerPolicyTest, HasNoInstallerAttributes) {
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName, base::DoNothing());

  EXPECT_TRUE(policy.GetInstallerAttributes().empty());
}

TEST_F(SponsoredContentComponentInstallerPolicyTest,
       ComponentReadyRunsCallbackWithInstalledPath) {
  base::test::TestFuture<base::FilePath> test_future;
  SponsoredContentComponentInstallerPolicy policy(
      component_public_key_sha256_, kComponentName,
      test_future.GetRepeatingCallback<const base::FilePath&>());

  const base::FilePath installed_dir =
      base::FilePath::FromUTF8Unsafe("installed_dir");
  policy.ComponentReady(base::Version("1.0.0"), installed_dir,
                        base::DictValue());

  EXPECT_EQ(installed_dir, test_future.Get());
}

class SponsoredContentComponentInstallerTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  component_updater::MockComponentUpdateService component_update_service_;
  brave_component_updater::MockOnDemandUpdater on_demand_updater_;
  std::array<uint8_t, crypto::hash::kSha256Size> component_public_key_sha256_ =
      *GetComponentPublicKeySHA256("US");
};

TEST_F(SponsoredContentComponentInstallerTest,
       DoesNotRegisterWhenComponentUpdateServiceIsNull) {
  // `component_update_service` can be null in test; registering must not
  // crash and must not reach the real installer.
  RegisterSponsoredContentComponentInstaller(
      /*component_update_service=*/nullptr, component_public_key_sha256_,
      kComponentName, base::DoNothing());
}

TEST_F(SponsoredContentComponentInstallerTest,
       RegistersAndChecksForUpdateOnceRegistered) {
  EXPECT_CALL(component_update_service_, RegisterComponent)
      .WillOnce(testing::Return(true));

  base::test::TestFuture<void> test_future;
  EXPECT_CALL(
      on_demand_updater_,
      OnDemandUpdate(testing::An<const std::string&>(), testing::_, testing::_))
      .WillOnce([&test_future](const std::string& /*id*/,
                               component_updater::OnDemandUpdater::Priority
                               /*priority*/,
                               component_updater::Callback callback) {
        std::move(callback).Run(update_client::Error::NONE);
        test_future.SetValue();
      });

  RegisterSponsoredContentComponentInstaller(&component_update_service_,
                                             component_public_key_sha256_,
                                             kComponentName, base::DoNothing());

  ASSERT_TRUE(test_future.Wait());
}

// `ComponentInstaller::FinishRegistration` runs the registered callback even
// when `RegisterComponent` itself returns false.
TEST_F(SponsoredContentComponentInstallerTest,
       StillChecksForUpdateWhenRegisterComponentFails) {
  EXPECT_CALL(component_update_service_, RegisterComponent)
      .WillOnce(testing::Return(false));

  base::test::TestFuture<void> test_future;
  EXPECT_CALL(
      on_demand_updater_,
      OnDemandUpdate(testing::An<const std::string&>(), testing::_, testing::_))
      .WillOnce([&test_future](const std::string& /*id*/,
                               component_updater::OnDemandUpdater::Priority
                               /*priority*/,
                               component_updater::Callback callback) {
        std::move(callback).Run(update_client::Error::NONE);
        test_future.SetValue();
      });

  RegisterSponsoredContentComponentInstaller(&component_update_service_,
                                             component_public_key_sha256_,
                                             kComponentName, base::DoNothing());

  ASSERT_TRUE(test_future.Wait());
}

}  // namespace sponsored_content
