/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/component_updater/resource_component_registrar.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component_registrar_delegate.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

// Two valid country codes from the component map.
constexpr char kCountryCode[] = "AD";
constexpr char kOtherCountryCode[] = "AE";

constexpr base::FilePath::CharType kInstallDir[] =
    FILE_PATH_LITERAL("/fake/install/dir");
constexpr base::FilePath::CharType kUpdatedInstallDir[] =
    FILE_PATH_LITERAL("/fake/install/dir/v2");

// Fake BraveComponent::Delegate that captures the ready_callback so tests can
// trigger OnComponentReady at will.
class FakeComponentUpdaterDelegate
    : public brave_component_updater::BraveComponent::Delegate {
 public:
  FakeComponentUpdaterDelegate() = default;

  void Register(const std::string& /*component_name*/,
                const std::string& /*component_base64_public_key*/,
                base::OnceClosure registered_callback,
                brave_component_updater::BraveComponent::ReadyCallback
                    ready_callback) override {
    ready_callback_ = std::move(ready_callback);
    if (registered_callback) {
      std::move(registered_callback).Run();
    }
  }

  bool Unregister(const std::string& /*component_id*/) override { return true; }
  void EnsureInstalled(const std::string& /*component_id*/) override {}
  void AddObserver(
      brave_component_updater::BraveComponent::ComponentObserver*) override {}
  void RemoveObserver(
      brave_component_updater::BraveComponent::ComponentObserver*) override {}
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() override {
    return nullptr;
  }
  const std::string& locale() const override { return locale_; }
  PrefService* local_state() override { return nullptr; }

  void SimulateComponentReady(const base::FilePath& install_dir) {
    ASSERT_TRUE(ready_callback_) << "Register() must be called first";
    ready_callback_.Run(install_dir, "{}");
  }

 private:
  brave_component_updater::BraveComponent::ReadyCallback ready_callback_;
  std::string locale_;
};

// Fake delegate that records calls from ResourceComponentRegistrar.
class FakeResourceComponentRegistrarDelegate
    : public ResourceComponentRegistrarDelegate {
 public:
  void OnResourceComponentRegistered(
      const std::string& /*component_id*/,
      const base::FilePath& install_dir) override {
    ++registered_count_;
    last_registered_install_dir_ = install_dir;
  }

  void OnResourceComponentUnregistered(
      const std::string& /*component_id*/) override {
    ++unregistered_count_;
  }

  size_t registered_count() const { return registered_count_; }
  size_t unregistered_count() const { return unregistered_count_; }
  const base::FilePath& last_registered_install_dir() const {
    return last_registered_install_dir_;
  }

 private:
  size_t registered_count_ = 0;
  size_t unregistered_count_ = 0;
  base::FilePath last_registered_install_dir_;
};

}  // namespace

class BraveAdsResourceComponentRegistrarTest : public testing::Test {
 protected:
  void SetUp() override {
    resource_component_registrar_ =
        std::make_unique<ResourceComponentRegistrar>(
            &component_updater_delegate_,
            resource_component_registrar_delegate_);
  }

  FakeComponentUpdaterDelegate component_updater_delegate_;
  FakeResourceComponentRegistrarDelegate resource_component_registrar_delegate_;
  std::unique_ptr<ResourceComponentRegistrar> resource_component_registrar_;
};

TEST_F(BraveAdsResourceComponentRegistrarTest,
       DoesNotNotifyDelegateBeforeComponentIsReady) {
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);

  EXPECT_EQ(0U, resource_component_registrar_delegate_.registered_count());
}

TEST_F(BraveAdsResourceComponentRegistrarTest,
       NotifiesDelegateWhenComponentBecomesReady) {
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);
  component_updater_delegate_.SimulateComponentReady(
      base::FilePath(kInstallDir));

  EXPECT_EQ(1U, resource_component_registrar_delegate_.registered_count());
  EXPECT_EQ(
      base::FilePath(kInstallDir),
      resource_component_registrar_delegate_.last_registered_install_dir());
}

// Regression test: a profile opened after the component was already processed
// must still receive the resource metadata. Previously, re-registration was a
// no-op in the component updater so newly-added observers were never notified.
TEST_F(BraveAdsResourceComponentRegistrarTest,
       ReplaysReadyNotificationOnReRegistrationForSameComponent) {
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);
  component_updater_delegate_.SimulateComponentReady(
      base::FilePath(kInstallDir));
  ASSERT_EQ(1U, resource_component_registrar_delegate_.registered_count());

  // Simulate a second profile registering the same component (e.g. opening the
  // profile after the first one has already processed the update).
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);

  EXPECT_EQ(2U, resource_component_registrar_delegate_.registered_count());
  EXPECT_EQ(
      base::FilePath(kInstallDir),
      resource_component_registrar_delegate_.last_registered_install_dir());
}

TEST_F(BraveAdsResourceComponentRegistrarTest,
       ReplayUsesLatestInstallDirAfterComponentUpdate) {
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);
  component_updater_delegate_.SimulateComponentReady(
      base::FilePath(kInstallDir));

  // Simulate the component updating to a newer version.
  component_updater_delegate_.SimulateComponentReady(
      base::FilePath(kUpdatedInstallDir));
  ASSERT_EQ(2U, resource_component_registrar_delegate_.registered_count());

  // A subsequent re-registration must replay the newest install dir.
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);
  EXPECT_EQ(3U, resource_component_registrar_delegate_.registered_count());
  EXPECT_EQ(
      base::FilePath(kUpdatedInstallDir),
      resource_component_registrar_delegate_.last_registered_install_dir());
}

TEST_F(BraveAdsResourceComponentRegistrarTest,
       ReRegistrationForSameComponentDoesNotUnregister) {
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);
  component_updater_delegate_.SimulateComponentReady(
      base::FilePath(kInstallDir));

  resource_component_registrar_->RegisterResourceComponent(kCountryCode);

  EXPECT_EQ(0U, resource_component_registrar_delegate_.unregistered_count());
}

TEST_F(BraveAdsResourceComponentRegistrarTest,
       DoesNotReplayWhenSwitchingToADifferentComponent) {
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);
  component_updater_delegate_.SimulateComponentReady(
      base::FilePath(kInstallDir));
  ASSERT_EQ(1U, resource_component_registrar_delegate_.registered_count());

  // Switching country codes must unregister the old component and not replay
  // the stale install dir for the new one.
  resource_component_registrar_->RegisterResourceComponent(kOtherCountryCode);

  EXPECT_EQ(1U, resource_component_registrar_delegate_.registered_count());
  EXPECT_EQ(1U, resource_component_registrar_delegate_.unregistered_count());
}

TEST_F(BraveAdsResourceComponentRegistrarTest,
       NotifiesDelegateWhenReplacementComponentBecomesReady) {
  resource_component_registrar_->RegisterResourceComponent(kCountryCode);
  component_updater_delegate_.SimulateComponentReady(
      base::FilePath(kInstallDir));

  // Switch to a different component and bring it ready.
  resource_component_registrar_->RegisterResourceComponent(kOtherCountryCode);
  component_updater_delegate_.SimulateComponentReady(
      base::FilePath(kUpdatedInstallDir));

  EXPECT_EQ(2U, resource_component_registrar_delegate_.registered_count());
  EXPECT_EQ(
      base::FilePath(kUpdatedInstallDir),
      resource_component_registrar_delegate_.last_registered_install_dir());
}

TEST_F(BraveAdsResourceComponentRegistrarTest, IgnoresUnsupportedResourceId) {
  resource_component_registrar_->RegisterResourceComponent("UNSUPPORTED");

  EXPECT_EQ(0U, resource_component_registrar_delegate_.registered_count());
}

}  // namespace brave_ads
