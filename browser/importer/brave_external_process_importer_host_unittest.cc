/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "brave/browser/importer/extensions_import_helpers.h"
#include "brave/common/importer/importer_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/load_error_reporter.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "chrome/test/base/testing_profile.h"
#include "components/value_store/test_value_store_factory.h"
#include "components/value_store/value_store.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr const char* kExtensions[] = {
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
    "cccccccccccccccccccccccccccccccc", "dddddddddddddddddddddddddddddddd"};

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
  void ImportItemEnded(importer::ImportItem item) override {}

  // Invoked when the source profile has been imported.
  void ImportEnded() override { std::move(callback_).Run(); }

 protected:
  base::OnceClosure callback_;
};

void CreateTestingStore(const base::FilePath& path, const std::string& id) {
  auto store_factory =
      base::MakeRefCounted<value_store::TestValueStoreFactory>(path);
  auto source_store = store_factory->CreateValueStore(
      base::FilePath(extensions::kLocalExtensionSettingsDirectoryName), id);
  source_store->Set(value_store::ValueStore::DEFAULTS, "id", base::Value(id));
}

std::string ReadStore(const base::FilePath& path, const std::string& id) {
  if (!base::DirectoryExists(path)) {
    return {};
  }
  auto store_factory =
      base::MakeRefCounted<value_store::TestValueStoreFactory>(path);
  auto source_store = store_factory->CreateValueStore(
      base::FilePath(extensions::kLocalExtensionSettingsDirectoryName), id);
  auto setting = source_store->Get();
  if (!setting.status().ok()) {
    return {};
  }
  return *setting.PassSettings().FindString("id");
}

}  // namespace

class BraveExternalProcessImporterHostUnitTest : public testing::Test {
 public:
  BraveExternalProcessImporterHostUnitTest() = default;

  void SetUp() override {
    TestingProfile::Builder profile_builder;
    EXPECT_TRUE(brave_profile_dir_.CreateUniqueTempDir());
    profile_builder.SetPath(GetProductProfilePath("Brave"));
    profile_ = profile_builder.Build();
    base::CreateDirectory(profile_->GetPath().AppendASCII("IndexedDB"));

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    base::CopyDirectory(test_data_dir.AppendASCII("extensions")
                            .AppendASCII("import")
                            .AppendASCII("Chrome"),
                        GetProductProfilePath("Chrome"), true);
    for (const char* id : kExtensions) {
      CreateTestingStore(GetExtensionLocalSettingsPath("Chrome", id), id);
    }

    extensions::LoadErrorReporter::Init(false);
    extensions::TestExtensionSystem* extension_system =
        static_cast<extensions::TestExtensionSystem*>(
            extensions::ExtensionSystem::Get(GetProfile()));
    extension_system->CreateExtensionService(
        base::CommandLine::ForCurrentProcess(), base::FilePath(), false);
    extension_service_ =
        extensions::ExtensionSystem::Get(GetProfile())->extension_service();
  }

  void TearDown() override { extension_service_ = nullptr; }

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

  extensions::ExtensionService* extension_service() {
    return extension_service_.get();
  }

  void AddExtension(const std::string& id) {
    auto extension = extensions::ExtensionBuilder()
                         .SetManifest(base::Value::Dict()
                                          .Set("name", "ext")
                                          .Set("version", "0.1")
                                          .Set("manifest_version", 2))
                         .SetID(id)
                         .Build();
    ASSERT_TRUE(extension);
    extension_service()->AddExtension(extension.get());
  }

  Profile* GetProfile() { return profile_.get(); }

  void LaunchExtensionsImportAndWait(importer::SourceProfile source_profile) {
    base::RunLoop loop;
    ImportEndedObserver observer(loop.QuitClosure());

    // BraveExternalProcessImporterHost uses `delete this`.
    auto* external_process_host = new BraveExternalProcessImporterHost();

    external_process_host->DoNotLaunchImportForTesting();
    external_process_host->set_observer(&observer);
    external_process_host->StartImportSettings(source_profile, GetProfile(),
                                               importer::EXTENSIONS, nullptr);
    loop.Run();
  }

  std::string ReadTargetStore(const std::string& id) {
    return ReadStore(GetExtensionLocalSettingsPath("Brave", id), id);
  }

  std::string ReadTargetIndexedDB(const std::string& id,
                                  const std::string& type) {
    base::ScopedAllowBlockingForTesting allow_io;
    const auto base_path = GetProductProfilePath("Brave")
                               .AppendASCII("IndexedDB")
                               .AppendASCII(base::StrCat(
                                   {"chrome-extension_", id, "_0.indexeddb"}));
    std::string contents;
    base::ReadFileToString(
        base_path.AddExtensionASCII(type).AppendASCII("data.txt"), &contents);
    return contents;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::ScopedTempDir brave_profile_dir_;

  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<extensions::ExtensionService> extension_service_ = nullptr;
};

TEST_F(BraveExternalProcessImporterHostUnitTest, ImportEtensionsSettings) {
  importer::SourceProfile source_profile;
  source_profile.source_path = GetProductProfilePath("Chrome");
  source_profile.importer_type = importer::TYPE_CHROME;
  source_profile.services_supported = importer::EXTENSIONS;

  auto extension_installer = base::BindLambdaForTesting(
      [this](
          const std::string& id) -> extensions_import::ExtensionImportStatus {
        if (id == kExtensions[0]) {
          return extensions_import::ExtensionImportStatus::kFailedToInstall;
        }
        AddExtension(id);
        return extensions_import::ExtensionImportStatus::kOk;
      });

  extensions_import::ExtensionsImporter::GetExtensionInstallerForTesting() =
      extension_installer;

  LaunchExtensionsImportAndWait(source_profile);

  EXPECT_TRUE(ReadTargetStore(kExtensions[0]).empty());  // failed to install
  EXPECT_EQ(kExtensions[1], ReadTargetStore(kExtensions[1]));
  EXPECT_EQ(kExtensions[2], ReadTargetStore(kExtensions[2]));
  EXPECT_EQ(kExtensions[3], ReadTargetStore(kExtensions[3]));

  EXPECT_TRUE(ReadTargetIndexedDB(kExtensions[0], "blob").empty());
  EXPECT_TRUE(ReadTargetIndexedDB(kExtensions[0], "leveldb").empty());
  EXPECT_EQ(ReadTargetIndexedDB(kExtensions[1], "blob"), "id1");
  EXPECT_EQ(ReadTargetIndexedDB(kExtensions[1], "leveldb"), "id1");
  // no data in the source profile:
  EXPECT_TRUE(ReadTargetIndexedDB(kExtensions[2], "blob").empty());
  EXPECT_TRUE(ReadTargetIndexedDB(kExtensions[2], "leveldb").empty());
  EXPECT_TRUE(ReadTargetIndexedDB(kExtensions[3], "blob").empty());
  EXPECT_TRUE(ReadTargetIndexedDB(kExtensions[3], "leveldb").empty());

  extensions_import::ExtensionsImporter::GetExtensionInstallerForTesting()
      .Reset();
}
