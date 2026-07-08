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

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/psst/core/browser/psst_component_installer.h"
#include "brave/components/psst/core/browser/psst_report_uploader.h"
#include "brave/components/psst/core/common/psst_script_responses.h"
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
               base::ListValue failed_tasks,
               base::OnceCallback<void()> callback),
              (override));
};

class MockPsstReporterServiceDelegate : public PsstReporterService::Delegate {
 public:
  MockPsstReporterServiceDelegate() = default;
  ~MockPsstReporterServiceDelegate() override = default;

  MOCK_METHOD(std::vector<ComponentInfo>,
              GetComponentInfos,
              (),
              (const, override));
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
                       int,
                       base::ListValue,
                       base::OnceCallback<void()> callback) {
  std::move(callback).Run();
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
  auto delegate =
      std::make_unique<testing::NiceMock<MockPsstReporterServiceDelegate>>();
  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();
  PsstReporterService service(std::move(delegate), std::move(uploader));

  EXPECT_CALL(*uploader_ptr, Upload).Times(0);

  base::test::TestFuture<void> future;
  service.SubmitPsstErrorsReport(std::nullopt, /*script_version=*/1,
                                 future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

TEST_F(PsstReporterServiceUnitTest,
       EmptyFailedTasksSkipsUploadAndRunsCallback) {
  auto delegate =
      std::make_unique<testing::NiceMock<MockPsstReporterServiceDelegate>>();
  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();
  PsstReporterService service(std::move(delegate), std::move(uploader));

  EXPECT_CALL(*uploader_ptr, Upload).Times(0);

  base::test::TestFuture<void> future;
  service.SubmitPsstErrorsReport(PolicyTasksSet(), /*script_version=*/1,
                                 future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

TEST_F(PsstReporterServiceUnitTest,
       FailedTasksUploadMatchedComponentVersionAndSortedTasks) {
  auto delegate = std::make_unique<MockPsstReporterServiceDelegate>();
  const std::vector<PsstReporterService::PsstComponentInfo> components{
      {"other-id", "other-name", "0.0.1"},
      {kPsstComponentId, "psst", "9.8.7"},
  };
  EXPECT_CALL(*delegate, GetComponentInfos)
      .WillOnce(testing::Return(components));

  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();
  PsstReporterService service(std::move(delegate), std::move(uploader));

  PolicyTasksSet tasks;
  tasks.insert(MakePolicyTask("uid-b", "https://b.example", "task b"));
  tasks.insert(MakePolicyTask("uid-a", "https://a.example", "task a", "boom"));

  // Only failed tasks must be passed to uploader
  base::ListValue expected_tasks;
  expected_tasks.Append(
      MakePolicyTask("uid-a", "https://a.example", "task a", "boom").ToValue());

  EXPECT_CALL(*uploader_ptr,
              Upload(std::optional<std::string>("9.8.7"), 42, _, _))
      .WillOnce([&](std::optional<std::string>, int,
                    base::ListValue failed_tasks,
                    base::OnceCallback<void()> callback) {
        EXPECT_EQ(failed_tasks, expected_tasks);
        std::move(callback).Run();
      });

  base::test::TestFuture<void> future;
  service.SubmitPsstErrorsReport(std::move(tasks), 42, future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

TEST_F(PsstReporterServiceUnitTest,
       SkipsUploadWhenNoTasksHaveErrorDescription) {
  auto delegate = std::make_unique<MockPsstReporterServiceDelegate>();
  const std::vector<PsstReporterService::PsstComponentInfo> components{
      {"other-id", "other-name", "0.0.1"},
  };
  EXPECT_CALL(*delegate, GetComponentInfos)
      .WillOnce(testing::Return(components));

  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();
  PsstReporterService service(std::move(delegate), std::move(uploader));

  PolicyTasksSet tasks;
  tasks.insert(MakePolicyTask("uid-a", "https://a.example", "task a"));

  EXPECT_CALL(*uploader_ptr, Upload(std::optional<std::string>(), 1, _, _))
      .Times(0);

  base::test::TestFuture<void> future;
  service.SubmitPsstErrorsReport(std::move(tasks), 1, future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

TEST_F(PsstReporterServiceUnitTest,
       FailedTasksUploadWithoutDelegateSkipsComponentVersionLookup) {
  auto uploader = std::make_unique<MockPsstErrorReportUploader>();
  auto* uploader_ptr = uploader.get();
  PsstReporterService service(/*service_delegate=*/nullptr,
                              std::move(uploader));

  PolicyTasksSet tasks;
  tasks.insert(MakePolicyTask("uid-a", "https://a.example", "task a", "boom"));

  EXPECT_CALL(*uploader_ptr, Upload(std::optional<std::string>(), 3, _, _))
      .Times(1)
      .WillOnce(&RunUploadCallback);

  base::test::TestFuture<void> future;
  service.SubmitPsstErrorsReport(std::move(tasks), 3, future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

}  // namespace psst
