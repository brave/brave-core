/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "brave/browser/importer/test_storage_utils.h"
#include "brave/common/importer/importer_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "chrome/test/base/testing_profile.h"
#include "components/value_store/test_value_store_factory.h"
#include "components/value_store/value_store.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/common/constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class ImportEndedObserver : public importer::ImporterProgressObserver {
 public:
  explicit ImportEndedObserver(base::OnceClosure callback)
      : callback_(std::move(callback)) {}
  ~ImportEndedObserver() override = default;

  // Invoked when the import begins.
  void ImportStarted() override {}

  // Invoked when data for the specified item is about to be collected.
  void ImportItemStarted(importer::ImportItem item) override {}

  // Invoked when data for the specified item has been collected from the
  // source profile and is now ready for further processing.
  void ImportItemEnded(importer::ImportItem item) override {
    std::move(callback_).Run();
  }

  // Invoked when the source profile has been imported.
  void ImportEnded() override {}

 protected:
  base::OnceClosure callback_;
};

}  // namespace

class BraveExternalProcessImporterHostUnitTest : public testing::Test {
 public:
  BraveExternalProcessImporterHostUnitTest() {}

  void SetUp() override {
    TestingProfile::Builder profile_builder;
    EXPECT_TRUE(brave_profile_dir_.CreateUniqueTempDir());
    profile_builder.SetPath(GetProductProfilePath("Brave"));
    profile_ = profile_builder.Build();

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    base::CreateDirectory(GetProductProfilePath("Chrome"));
    base::CopyFile(test_data_dir.AppendASCII("extensions")
                       .AppendASCII("import")
                       .AppendASCII("Chrome")
                       .AppendASCII("Secure Preferences"),
                   GetProductProfilePath("Chrome").AppendASCII(
                       kChromeSecurePreferencesFile));
    for (auto i = 0; i < 4; i++) {
      const std::string id = "id" + std::to_string(i);
      brave::CreateTestingStore(GetExtensionLocalSettingsPath("Chrome", id), id,
                                {{"a", "b"}, {"c", "d"}, {"id", id}});
    }

    external_process_host_ = new BraveExternalProcessImporterHost();
    external_process_host()->SetInstallExtensionCallbackForTesting(
        base::BindRepeating(
            &BraveExternalProcessImporterHostUnitTest::InstallExtension,
            base::Unretained(this)));
  }

  base::FilePath GetProductProfilePath(const std::string& product) {
    return brave_profile_dir_.GetPath()
        .AppendASCII("extensions")
        .AppendASCII("import")
        .AppendASCII(product);
  }
  base::FilePath GetExtensionLocalSettingsPath(const std::string& product,
                                               const std::string& id) {
    return GetProductProfilePath(product)
        .Append(extensions::kLocalExtensionSettingsDirectoryName)
        .AppendASCII(id);
  }

  BraveExternalProcessImporterHost* external_process_host() {
    return external_process_host_.get();
  }

  Profile* GetProfile() { return profile_.get(); }

  void InstallExtension(const std::string& id) {
    EXPECT_TRUE(expected_extensions_.count(id));
    bool should_remove_settings = !expected_extensions_[id];
    base::RunLoop loop(base::RunLoop::Type::kNestableTasksAllowed);
    if (should_remove_settings) {
      external_process_host_->SetSettingsRemovedCallbackForTesting(
          loop.QuitClosure());
    }
    external_process_host_->OnExtensionInstalled(
        id, expected_extensions_[id], std::string(),
        extensions::webstore_install::SUCCESS);
    if (should_remove_settings) {
      loop.Run();
    }
  }

  void SetInstallationResults(std::map<std::string, bool> expected_extensions) {
    expected_extensions_ = expected_extensions;
  }

  void LaunchExtensionsImportAndWait(importer::SourceProfile source_profile) {
    external_process_host()->DoNotLaunchImportForTesting();
    external_process_host()->StartImportSettings(source_profile, GetProfile(),
                                                 importer::EXTENSIONS, nullptr);
    base::RunLoop loop;
    ImportEndedObserver observer(base::BindLambdaForTesting([&loop, this] {
      external_process_host_ = nullptr;
      loop.Quit();
    }));
    external_process_host()->set_observer(&observer);
    external_process_host()->NotifyImportEnded();
    loop.Run();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::ScopedTempDir brave_profile_dir_;
  std::map<std::string, bool> expected_extensions_;
  raw_ptr<BraveExternalProcessImporterHost> external_process_host_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveExternalProcessImporterHostUnitTest, ImportExtensionsSettings) {
  importer::SourceProfile source_profile;
  source_profile.source_path = GetProductProfilePath("Chrome");
  source_profile.importer_type = importer::TYPE_CHROME;
  source_profile.services_supported = importer::EXTENSIONS;
  SetInstallationResults(
      {{"id0", false}, {"id1", true}, {"id2", true}, {"id3", true}});
  LaunchExtensionsImportAndWait(source_profile);

  base::FilePath targetLocalExtensionSettingsPath =
      GetProfile()->GetPath().Append(
          extensions::kLocalExtensionSettingsDirectoryName);

  EXPECT_EQ(
      brave::ReadStore(GetExtensionLocalSettingsPath("Brave", "id1"), "id1"),
      base::JSONReader::Read(R"({
    "a": "b",
    "c": "d",
    "id": "id1"
  })"));
  EXPECT_EQ(
      brave::ReadStore(GetExtensionLocalSettingsPath("Brave", "id2"), "id2"),
      base::JSONReader::Read(R"({
    "a": "b",
    "c": "d",
    "id": "id2"
  })"));
  EXPECT_EQ(
      brave::ReadStore(GetExtensionLocalSettingsPath("Brave", "id3"), "id3"),
      base::JSONReader::Read(R"({
    "a": "b",
    "c": "d",
    "id": "id3"
  })"));
  EXPECT_FALSE(
      brave::ReadStore(GetExtensionLocalSettingsPath("Brave", "id0"), "id0"));
}
