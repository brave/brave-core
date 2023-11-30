// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/test/bind.h"
#include "base/time/time_override.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::subtle::ScopedTimeClockOverrides;
using testing::_;

namespace ipfs {

namespace {

class MockIpfsService : public IpfsService {
 public:
  MockIpfsService() = default;

  ~MockIpfsService() override = default;

  MOCK_METHOD(bool, IsDaemonLaunched, (), (const, override));
  MOCK_METHOD1(StartDaemonAndLaunch, void(base::OnceCallback<void()>));
  MOCK_METHOD2(GetConnectedPeers,
               void(IpfsService::GetConnectedPeersCallback,
                    std::optional<int>));
};

}  // namespace

class IpfsBasePinServiceTest : public testing::Test {
 public:
  IpfsBasePinServiceTest() = default;

  void SetUp() override {
    task_environment_ = std::make_unique<content::BrowserTaskEnvironment>(
        base::test::TaskEnvironment::TimeSource::MOCK_TIME);
    ipfs_base_pin_service_ =
        std::make_unique<IpfsBasePinService>(GetIpfsService());
  }

  testing::NiceMock<MockIpfsService>* GetIpfsService() {
    return &ipfs_service_;
  }

  IpfsBasePinService* service() { return ipfs_base_pin_service_.get(); }

  content::BrowserTaskEnvironment* env() { return task_environment_.get(); }

 private:
  std::unique_ptr<IpfsBasePinService> ipfs_base_pin_service_;
  testing::NiceMock<MockIpfsService> ipfs_service_;
  std::unique_ptr<content::BrowserTaskEnvironment> task_environment_;
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
  service()->OnGetConnectedPeersResult(1, true, {});
  std::optional<bool> method_called;
  std::unique_ptr<MockJob> first_job = std::make_unique<MockJob>(
      base::BindLambdaForTesting([&method_called]() { method_called = true; }));
  service()->AddJob(std::move(first_job));
  EXPECT_TRUE(method_called.value());

  std::optional<bool> second_method_called;
  std::unique_ptr<MockJob> second_job =
      std::make_unique<MockJob>(base::BindLambdaForTesting(
          [&second_method_called]() { second_method_called = true; }));
  service()->AddJob(std::move(second_job));
  EXPECT_FALSE(second_method_called.has_value());

  service()->OnJobDone(true);
  EXPECT_TRUE(second_method_called.value());
}

TEST_F(IpfsBasePinServiceTest, OnIpfsShutdown) {
  service()->OnGetConnectedPeersResult(1, true, {});
  EXPECT_TRUE(service()->daemon_ready_);

  std::unique_ptr<MockJob> first_job =
      std::make_unique<MockJob>(base::DoNothing());
  std::unique_ptr<MockJob> second_job =
      std::make_unique<MockJob>(base::DoNothing());

  service()->AddJob(std::move(first_job));
  service()->AddJob(std::move(second_job));

  service()->OnIpfsShutdown();

  EXPECT_EQ(1u, service()->jobs_.size());
  EXPECT_FALSE(service()->current_job_);

  service()->OnJobDone(false);

  EXPECT_EQ(1u, service()->jobs_.size());
  EXPECT_FALSE(service()->current_job_);
}

TEST_F(IpfsBasePinServiceTest, OnGetConnectedPeers) {
  service()->OnGetConnectedPeersResult(1, true, {});
  EXPECT_TRUE(service()->daemon_ready_);

  std::unique_ptr<MockJob> first_job =
      std::make_unique<MockJob>(base::DoNothing());
  std::unique_ptr<MockJob> second_job =
      std::make_unique<MockJob>(base::DoNothing());

  service()->AddJob(std::move(first_job));

  service()->OnGetConnectedPeersResult(1, true, {});

  service()->AddJob(std::move(second_job));

  service()->OnGetConnectedPeersResult(1, true, {});

  EXPECT_EQ(1u, service()->jobs_.size());
  EXPECT_TRUE(service()->current_job_);

  service()->OnJobDone(true);

  EXPECT_EQ(0u, service()->jobs_.size());
  EXPECT_TRUE(service()->current_job_);

  service()->OnJobDone(true);

  EXPECT_EQ(0u, service()->jobs_.size());
  EXPECT_FALSE(service()->current_job_);
}

TEST_F(IpfsBasePinServiceTest, OnGetConnectedPeers_Retry) {
  ON_CALL(*GetIpfsService(), StartDaemonAndLaunch(_))
      .WillByDefault(
          ::testing::Invoke([](base::OnceCallback<void(void)> callback) {
            std::move(callback).Run();
          }));
  ON_CALL(*GetIpfsService(), IsDaemonLaunched())
      .WillByDefault(::testing::Return(true));

  std::unique_ptr<MockJob> first_job =
      std::make_unique<MockJob>(base::DoNothing());

  service()->AddJob(std::move(first_job));
  EXPECT_CALL(*GetIpfsService(), GetConnectedPeers(_, _)).Times(1);

  env()->AdvanceClock(base::Minutes(2));
  env()->RunUntilIdle();

  EXPECT_CALL(*GetIpfsService(), GetConnectedPeers(_, _)).Times(1);
  service()->OnGetConnectedPeersResult(1, false, {});

  env()->AdvanceClock(base::Minutes(2));
  env()->RunUntilIdle();
}

}  // namespace ipfs
