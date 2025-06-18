/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_importer_observer.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/test/bind.h"
#include "base/test/values_test_util.h"
#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveImporterObserverUnitTest : public testing::Test {
 public:
  BraveImporterObserverUnitTest() {}

  void SetExpectedInfo(base::Value::Dict value) {
    expected_info_ = std::move(value);
  }
  void SetExpectedCalls(int value) { expected_calls_ = value; }
  int GetExpectedCalls() { return expected_calls_; }
  void NotifyImportProgress(
      const user_data_importer::SourceProfile& source_profile,
      const base::Value::Dict& info) {
    EXPECT_EQ(expected_info_, info);
    expected_calls_++;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::Value::Dict expected_info_;
  int expected_calls_ = 0;
  raw_ptr<BraveExternalProcessImporterHost> _ = nullptr;
};

TEST_F(BraveImporterObserverUnitTest, ImportEvents) {
  auto* importer_host = new BraveExternalProcessImporterHost();
  user_data_importer::SourceProfile source_profile;
  source_profile.importer_name = u"importer_name";
  source_profile.importer_type = user_data_importer::TYPE_CHROME;
  source_profile.source_path = base::FilePath(FILE_PATH_LITERAL("test"));
  auto imported_items =
      user_data_importer::AUTOFILL_FORM_DATA | user_data_importer::PASSWORDS;
  std::unique_ptr<BraveImporterObserver> observer =
      std::make_unique<BraveImporterObserver>(
          importer_host, source_profile, imported_items,
          base::BindRepeating(
              &BraveImporterObserverUnitTest::NotifyImportProgress,
              base::Unretained(this)));
  EXPECT_EQ(importer_host->GetObserverForTesting(), observer.get());
  EXPECT_EQ(GetExpectedCalls(), 0);

  // Multiple calls for same profile.
  SetExpectedInfo(base::test::ParseJsonDict(R"({
    "event": "ImportStarted",
    "importer_name": "importer_name",
    "importer_type": 1,
    "items_to_import": 72
  })"));
  observer->ImportStarted();
  observer->ImportStarted();
  observer->ImportStarted();
  EXPECT_EQ(GetExpectedCalls(), 1);

  // ImportItemStarted event.
  SetExpectedCalls(0);
  EXPECT_EQ(GetExpectedCalls(), 0);
  SetExpectedInfo(base::test::ParseJsonDict(R"({
    "event": "ImportItemStarted",
    "importer_name": "importer_name",
    "importer_type": 1,
    "item": 8,
    "items_to_import": 72
  })"));

  observer->ImportItemStarted(user_data_importer::PASSWORDS);
  EXPECT_EQ(GetExpectedCalls(), 1);

  // ImportItemEnded event.
  SetExpectedCalls(0);
  EXPECT_EQ(GetExpectedCalls(), 0);
  SetExpectedInfo(base::test::ParseJsonDict(R"({
    "event": "ImportItemEnded",
    "importer_name": "importer_name",
    "importer_type": 1,
    "item": 8,
    "items_to_import": 72
  })"));

  observer->ImportItemEnded(user_data_importer::PASSWORDS);
  EXPECT_EQ(GetExpectedCalls(), 1);

  // ImportEnded event.
  SetExpectedCalls(0);
  EXPECT_EQ(GetExpectedCalls(), 0);
  SetExpectedInfo(base::test::ParseJsonDict(R"({
    "event": "ImportEnded",
    "importer_name": "importer_name",
    "importer_type": 1,
    "items_to_import": 72
  })"));

  observer->ImportEnded();
  EXPECT_EQ(GetExpectedCalls(), 1);
  EXPECT_EQ(observer->GetImporterHostForTesting(), nullptr);
  // The observer should be removed on ImportEnded event.
  EXPECT_EQ(importer_host->GetObserverForTesting(), nullptr);

  // ImportEnded event should not be called anymore.
  SetExpectedCalls(0);
  EXPECT_EQ(GetExpectedCalls(), 0);
  // Destroy host.
  importer_host->NotifyImportEndedForTesting();
  EXPECT_EQ(GetExpectedCalls(), 0);
}

TEST_F(BraveImporterObserverUnitTest, DestroyObserverEarly) {
  auto* importer_host = new BraveExternalProcessImporterHost();
  user_data_importer::SourceProfile source_profile;
  source_profile.importer_name = u"importer_name";
  source_profile.importer_type = user_data_importer::TYPE_CHROME;
  source_profile.source_path = base::FilePath(FILE_PATH_LITERAL("test"));
  auto imported_items =
      user_data_importer::AUTOFILL_FORM_DATA | user_data_importer::PASSWORDS;
  std::unique_ptr<BraveImporterObserver> observer =
      std::make_unique<BraveImporterObserver>(
          importer_host, source_profile, imported_items,
          base::BindRepeating(
              &BraveImporterObserverUnitTest::NotifyImportProgress,
              base::Unretained(this)));
  EXPECT_EQ(importer_host->GetObserverForTesting(), observer.get());
  EXPECT_EQ(GetExpectedCalls(), 0);
  observer.reset();
  EXPECT_EQ(importer_host->GetObserverForTesting(), nullptr);
  // ImportEnded event should not be called anymore.
  SetExpectedCalls(0);
  EXPECT_EQ(GetExpectedCalls(), 0);
  // Destroy host.
  importer_host->NotifyImportEndedForTesting();
  EXPECT_EQ(GetExpectedCalls(), 0);
}
