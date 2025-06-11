/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "brave/browser/importer/extensions_import_helpers.h"
#include "brave/common/importer/chrome_importer_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/importer/profile_writer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/value_store/test_value_store_factory.h"
#include "components/value_store/value_store.h"
#include "content/public/test/browser_test.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kExtensionId[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
constexpr char kFailExtensionId[] = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

class TestObserver : public importer::ImporterProgressObserver {
 public:
  ~TestObserver() override = default;

  void ImportStarted() override {}
  void ImportItemStarted(importer::ImportItem item) override {}
  void ImportItemEnded(importer::ImportItem item) override {}
  MOCK_METHOD(void, ImportEnded, (), (override));
};

}  // namespace

class BraveImporterBrowserTest : public InProcessBrowserTest {
 public:
  Profile* CreateProfile() {
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    base::FilePath profile_path =
        profile_manager->GenerateNextProfileDirectoryPath();
    return &profiles::testing::CreateProfileSync(profile_manager, profile_path);
  }

  void NonBlockingDelay(base::TimeDelta delay) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
    run_loop.Run();
  }

  std::string ReadStore(Profile* profile, const extensions::ExtensionId& id) {
    base::ScopedAllowBlockingForTesting allow_io;
    const auto path =
        profile->GetPath()
            .Append(extensions::kLocalExtensionSettingsDirectoryName)
            .AppendASCII(id);

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

  std::string ReadIndexedDB(Profile* profile,
                            const extensions::ExtensionId& id) {
    base::ScopedAllowBlockingForTesting allow_io;

    const auto indexeddb_path =
        profile->GetPath()
            .AppendASCII("IndexedDB")
            .AppendASCII(
                base::StrCat({"chrome-extension_", id, "_indexeddb.test"}))
            .AppendASCII("test");
    std::string contents;
    base::ReadFileToString(indexeddb_path, &contents);
    return contents;
  }

  void AddTestExtension(Profile* profile, const extensions::ExtensionId& id) {
    scoped_refptr<const extensions::Extension> extension(
        extensions::ExtensionBuilder("extension")
            .AddFlags(extensions::Extension::FROM_WEBSTORE)
            .SetID(id)
            .AddJSON(R"("manifest_version": 2, "version": "1.0.0")")
            .SetLocation(extensions::mojom::ManifestLocation::kInternal)
            .Build());
    extensions::ExtensionRegistrar::Get(profile)->AddExtension(extension);
    extensions::ExtensionPrefs::Get(profile)->OnExtensionInstalled(
        extension.get(), {}, {}, extensions::Extension::FROM_WEBSTORE, {},
        base::Value::Dict());

    base::ScopedAllowBlockingForTesting allow_blocking;
    const auto count = GetImportableChromeExtensionsList(profile->GetPath())
                           .value_or(std::vector<std::string>())
                           .size();

    const auto indexeddb_path =
        profile->GetPath()
            .AppendASCII("IndexedDB")
            .AppendASCII(
                base::StrCat({"chrome-extension_", id, "_indexeddb.test"}));
    const auto local_store_path =
        profile->GetPath()
            .Append(extensions::kLocalExtensionSettingsDirectoryName)
            .AppendASCII(id);

    // Simulate pref data.
    auto store_factory =
        base::MakeRefCounted<value_store::TestValueStoreFactory>(
            local_store_path);
    auto source_store = store_factory->CreateValueStore(
        base::FilePath(extensions::kLocalExtensionSettingsDirectoryName), id);
    source_store->Set(value_store::ValueStore::DEFAULTS, "id", base::Value(id));

    base::CreateDirectory(indexeddb_path);
    base::WriteFile(indexeddb_path.AppendASCII("test"), id);

    // Wait for prefs are written on the disk.
    while (GetImportableChromeExtensionsList(profile->GetPath())
               .value_or(std::vector<std::string>())
               .size() != count + 1) {
      NonBlockingDelay(base::Milliseconds(10));
    }
  }
};

IN_PROC_BROWSER_TEST_F(BraveImporterBrowserTest, ImportExtensions) {
  auto* source_profile = CreateProfile();
  AddTestExtension(source_profile, kExtensionId);

  for (int i = 0; i < 3; ++i) {
    Profile* target = CreateProfile();

    for (bool reimport : {false, true}) {
      SCOPED_TRACE(testing::Message() << "Reimport: " << reimport);

      // Deletes itself.
      auto* host = new BraveExternalProcessImporterHost;

      testing::NiceMock<TestObserver> observer;

      base::RunLoop run_loop;
      EXPECT_CALL(observer, ImportEnded())
          .WillOnce(
              ::testing::InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));

      host->set_observer(&observer);

      importer::SourceProfile source;
      source.importer_type = importer::TYPE_CHROME;
      source.source_path = source_profile->GetPath();

      bool extension_imported = false;
      extensions_import::ExtensionsImporter::GetExtensionInstallerForTesting() =
          base::BindLambdaForTesting(
              [&](const std::string& id)
                  -> extensions_import::ExtensionImportStatus {
                EXPECT_EQ(id, kExtensionId);
                extension_imported = true;
                return extensions_import::ExtensionImportStatus::kOk;
              });

      host->StartImportSettings(source, target, importer::EXTENSIONS,
                                new ProfileWriter(target));
      run_loop.Run();
      EXPECT_TRUE(extension_imported);
      EXPECT_EQ(kExtensionId, ReadStore(target, kExtensionId));
      EXPECT_EQ(kExtensionId, ReadIndexedDB(target, kExtensionId));
    }
  }
}

IN_PROC_BROWSER_TEST_F(BraveImporterBrowserTest, ReImportExtensions) {
  auto* source_profile = CreateProfile();
  AddTestExtension(source_profile, kExtensionId);
  AddTestExtension(source_profile, kFailExtensionId);

  Profile* target = CreateProfile();

  auto import = [&]() {
    // Deletes itself.
    auto* host = new BraveExternalProcessImporterHost;

    testing::NiceMock<TestObserver> observer;

    base::RunLoop run_loop;
    EXPECT_CALL(observer, ImportEnded())
        .WillOnce(
            ::testing::InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));

    host->set_observer(&observer);

    importer::SourceProfile source;
    source.importer_type = importer::TYPE_CHROME;
    source.source_path = source_profile->GetPath();

    extensions_import::ExtensionsImporter::GetExtensionInstallerForTesting() =
        base::BindLambdaForTesting(
            [&](const std::string& id)
                -> extensions_import::ExtensionImportStatus {
              if (id == kFailExtensionId) {
                return extensions_import::ExtensionImportStatus::
                    kFailedToInstall;
              }
              AddTestExtension(target, id);
              return extensions_import::ExtensionImportStatus::kOk;
            });

    host->StartImportSettings(source, target, importer::EXTENSIONS,
                              new ProfileWriter(target));
    run_loop.Run();
  };

  // First: on-boarding import.
  import();

  // Second: import from settings. Shouldn't crash.
  import();
}
