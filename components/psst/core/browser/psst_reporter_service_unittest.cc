// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/core/browser/psst_reporter_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/psst/core/browser/psst_component_installer.h"
#include "brave/components/psst/core/browser/psst_report_uploader.h"
#include "brave/components/psst/core/common/psst_script_responses.h"
#include "brave/components/version_info/version_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace psst {

namespace {

class MockPsstErrorReportUploader : public PsstErrorReportUploader {
 public:
  MockPsstErrorReportUploader() : PsstErrorReportUploader(nullptr) {}
  ~MockPsstErrorReportUploader() override = default;

  MOCK_METHOD(void,
              Upload,
              (std::optional<std::string> psst_component_version,
               const int script_version,
               const std::string& brave_version,
               std::optional<std::string> channel,
               base::ListValue failed_tasks,
               base::OnceCallback<void()> callback),
              (override));
};

PolicyTask MakePolicyTask(
    std::string uid,
    std::string url,
    std::string description,
    std::optional<std::string> error_description = std::nullopt) {
  PolicyTask task;
  task.uid = std::move(uid);
  task.url = std::move(url);
  task.description = std::move(description);
  task.error_description = std::move(error_description);
  return task;
}

void RunUploadCallback(std::optional<std::string>,
                       const int,
                       const std::string&,
                       std::optional<std::string>,
                       base::ListValue,
                       base::OnceCallback<void()> callback) {
  std::move(callback).Run();
}

std::unique_ptr<PsstReporterService> CreateService(
    ChannelNameCallback channel_name_callback,
    ComponentVersionCallback component_version_callback,
    std::unique_ptr<MockPsstErrorReportUploader> uploader) {
  return std::make_unique<PsstReporterService>(
      std::move(channel_name_callback), std::move(component_version_callback),
      std::move(uploader));
}

std::unique_ptr<PsstReporterService> CreateServicePredefinedCallbacks(
    std::optional<std::string> component_version,
    const std::string& channel_name,
    std::unique_ptr<MockPsstErrorReportUploader> uploader) {
  auto cv_callback = base::BindLambdaForTesting(
      [&]() -> std::optional<std::string> { return component_version; });
  auto cn_callback =
      base::BindLambdaForTesting([&]() -> std::string { return channel_name; });

  return CreateService(std::move(cn_callback), std::move(cv_callback),
                       std::move(uploader));
}

}  // namespace

class PsstReporterServiceUnitTest : public testing::Test {
 public:
  PsstReporterServiceUnitTest() = default;
  ~PsstReporterServiceUnitTest() override = default;

 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(PsstReporterServiceUnitTest, NoFailedTasksSkipsUploadAndRunsCallback) {
  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();

  auto service =
      CreateServicePredefinedCallbacks("1", "test", std::move(uploader));

  EXPECT_CALL(*uploader_ptr, Upload).Times(0);

  base::test::TestFuture<void> future;
  service->SubmitPsstErrorsReport(std::nullopt, /*script_version=*/1,
                                  future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

TEST_F(PsstReporterServiceUnitTest,
       EmptyFailedTasksSkipsUploadAndRunsCallback) {
  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();

  auto service =
      CreateServicePredefinedCallbacks("1", "test", std::move(uploader));

  EXPECT_CALL(*uploader_ptr, Upload).Times(0);

  base::test::TestFuture<void> future;
  service->SubmitPsstErrorsReport(PolicyTasksSet(), /*script_version=*/1,
                                  future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

TEST_F(PsstReporterServiceUnitTest,
       FailedTasksUploadMatchedComponentVersionAndSortedTasks) {
  const std::string expected_channel_name("channel_name");
  const auto expected_brave_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();

  std::optional<std::string> component_version = "1";
  auto service = CreateServicePredefinedCallbacks(
      component_version, expected_channel_name, std::move(uploader));

  auto failed_task =
      MakePolicyTask("uid-a", "https://a.example", "task a", "boom");
  PolicyTasksSet tasks;
  tasks.insert(MakePolicyTask("uid-b", "https://b.example", "task b"));
  tasks.insert(failed_task.Clone());

  // Only failed tasks must be passed to uploader
  base::ListValue expected_tasks;
  expected_tasks.Append(failed_task.ToValue());

  EXPECT_CALL(*uploader_ptr,
              Upload(component_version, 42, expected_brave_version,
                     std::optional<std::string>(expected_channel_name), _, _))
      .WillOnce([&](std::optional<std::string>, const int, const std::string&,
                    std::optional<std::string>, base::ListValue failed_tasks,
                    base::OnceCallback<void()> callback) {
        EXPECT_EQ(failed_tasks, expected_tasks);
        std::move(callback).Run();
      });

  base::test::TestFuture<void> future;
  service->SubmitPsstErrorsReport(std::move(tasks), 42, future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

TEST_F(PsstReporterServiceUnitTest,
       SkipsUploadWhenNoTasksHaveErrorDescription) {
  const std::string expected_channel_name("channel_name");
  const auto expected_brave_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();

  std::optional<std::string> component_version = "1";
  auto service = CreateServicePredefinedCallbacks(
      component_version, expected_channel_name, std::move(uploader));

  PolicyTasksSet tasks;
  tasks.insert(MakePolicyTask("uid-a", "https://a.example", "task a"));

  EXPECT_CALL(*uploader_ptr,
              Upload(std::optional<std::string>(std::nullopt), _,
                     expected_brave_version,
                     std::optional<std::string>(expected_channel_name), _, _))
      .Times(0);

  base::test::TestFuture<void> future;
  service->SubmitPsstErrorsReport(std::move(tasks), 1, future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

TEST_F(PsstReporterServiceUnitTest, FailedTasksUploadWithNullServiceCallbacks) {
  const auto expected_brave_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();
  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();

  auto service = CreateService(base::NullCallback(), base::NullCallback(),
                               std::move(uploader));
  auto failed_task =
      MakePolicyTask("uid-a", "https://a.example", "task a", "boom");
  PolicyTasksSet tasks;
  tasks.insert(failed_task.Clone());

  base::ListValue expected_tasks;
  expected_tasks.Append(failed_task.ToValue());

  EXPECT_CALL(
      *uploader_ptr,
      Upload(std::optional<std::string>(std::nullopt), 3,
             expected_brave_version, std::optional<std::string>(std::nullopt),
             testing::Truly([&expected_tasks](const base::ListValue& actual) {
               return actual == expected_tasks;
             }),
             _))
      .Times(1)
      .WillOnce(&RunUploadCallback);

  base::test::TestFuture<void> future;
  service->SubmitPsstErrorsReport(std::move(tasks), 3, future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

}  // namespace psst
