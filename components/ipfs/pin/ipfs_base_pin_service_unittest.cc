/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"

#include <memory>
#include <utility>

#include "base/test/bind.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace ipfs {

class MockIpfsService : public IpfsService {
 public:
  MockIpfsService() = default;

  ~MockIpfsService() override = default;

  MOCK_METHOD1(StartDaemonAndLaunch, void(base::OnceCallback<void()>));
  MOCK_METHOD2(GetConnectedPeers,
               void(IpfsService::GetConnectedPeersCallback, int));
};

class IpfsBasePinServiceTest : public testing::Test {
 public:
  IpfsBasePinServiceTest() = default;

  void SetUp() override {
    auto* registry = pref_service_.registry();
    IpfsService::RegisterProfilePrefs(registry);
    ipfs_base_pin_service_ =
        std::make_unique<IpfsBasePinService>(GetPrefs(), GetIpfsService());
  }

  void TearDown() override {
    ipfs_base_pin_service_->RemovePrefListenersForTests();
  }

  void SetLocalNodeEnabled(bool enabled) {
    GetPrefs()->SetInteger(
        kIPFSResolveMethod,
        static_cast<int>(enabled ? IPFSResolveMethodTypes::IPFS_LOCAL
                                 : IPFSResolveMethodTypes::IPFS_DISABLED));
  }

  PrefService* GetPrefs() { return &pref_service_; }

  testing::NiceMock<MockIpfsService>* GetIpfsService() {
    return &ipfs_service_;
  }

  IpfsBasePinService* service() { return ipfs_base_pin_service_.get(); }

 private:
  std::unique_ptr<IpfsBasePinService> ipfs_base_pin_service_;
  testing::NiceMock<MockIpfsService> ipfs_service_;
  TestingPrefServiceSimple pref_service_;
  content::BrowserTaskEnvironment task_environment_;
};

class MockJob : public IpfsBaseJob {
 public:
  explicit MockJob(base::OnceCallback<void()> callback) {
    callback_ = std::move(callback);
  }

  void Start() override {
    if (callback_) {
      std::move(callback_).Run();
    }
  }

 private:
  base::OnceCallback<void()> callback_;
};

TEST_F(IpfsBasePinServiceTest, TasksExecuted) {
  service()->OnGetConnectedPeers(true, {});
  absl::optional<bool> method_called;
  std::unique_ptr<MockJob> first_job = std::make_unique<MockJob>(
      base::BindLambdaForTesting([&method_called]() { method_called = true; }));
  service()->AddJob(std::move(first_job));
  EXPECT_TRUE(method_called.value());

  absl::optional<bool> second_method_called;
  std::unique_ptr<MockJob> second_job =
      std::make_unique<MockJob>(base::BindLambdaForTesting(
          [&second_method_called]() { second_method_called = true; }));
  service()->AddJob(std::move(second_job));
  EXPECT_FALSE(second_method_called.has_value());

  service()->OnJobDone(true);
  EXPECT_TRUE(second_method_called.value());
}

TEST_F(IpfsBasePinServiceTest, LaunchDaemon_AfterSettingChange) {
  SetLocalNodeEnabled(false);
  EXPECT_CALL(*GetIpfsService(), StartDaemonAndLaunch(_)).Times(1);
  SetLocalNodeEnabled(true);
}

}  // namespace ipfs
