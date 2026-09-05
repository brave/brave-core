/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_external_process_importer_host.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/importer/extensions_import_helpers.h"
#include "brave/common/importer/importer_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "chrome/browser/password_manager/factories/profile_password_store_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/password_store/login_database.h"
#include "components/password_manager/core/browser/password_store/test_password_store.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "components/user_data_importer/common/importer_type.h"
#include "components/value_store/test_value_store_factory.h"
#include "components/value_store/value_store.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/load_error_reporter.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

constexpr const char* kExtensions[] = {
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
    "cccccccccccccccccccccccccccccccc", "dddddddddddddddddddddddddddddddd"};

class ImportEndedObserver : public importer::ImporterProgressObserver {
 public:
  ImportEndedObserver() = default;
  explicit ImportEndedObserver(base::OnceClosure callback)
      : callback_(std::move(callback)) {}
  ~ImportEndedObserver() override = default;

  void set_quit_closure(base::OnceClosure callback) {
    callback_ = std::move(callback);
  }

  // Invoked when the import begins.
  void ImportStarted() override {}

  // Invoked when data for the specified item is about to be collected.
  void ImportItemStarted(user_data_importer::ImportItem item) override {
    ++item_started_count_[item];
  }

  // Invoked when data for the specified item has been collected from the
  // source profile and is now ready for further processing.
  void ImportItemEnded(user_data_importer::ImportItem item) override {
    ++item_ended_count_[item];
  }

  // Invoked when the source profile has been imported.
  void ImportEnded() override {
    ++import_ended_count_;
    if (callback_) {
      std::move(callback_).Run();
    }
  }

  int import_ended_count() const { return import_ended_count_; }
  int item_started_count(user_data_importer::ImportItem item) {
    return item_started_count_[item];
  }
  int item_ended_count(user_data_importer::ImportItem item) {
    return item_ended_count_[item];
  }

 protected:
  base::OnceClosure callback_;
  int import_ended_count_ = 0;
  std::map<user_data_importer::ImportItem, int> item_started_count_;
  std::map<user_data_importer::ImportItem, int> item_ended_count_;
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

  void AddExtension(const std::string& id) {
    auto extension = extensions::ExtensionBuilder()
                         .SetManifest(base::DictValue()
                                          .Set("name", "ext")
                                          .Set("version", "0.1")
                                          .Set("manifest_version", 2))
                         .SetID(id)
                         .Build();
    ASSERT_TRUE(extension);
    extensions::ExtensionRegistrar::Get(GetProfile())
        ->AddExtension(extension.get());
  }

  Profile* GetProfile() { return profile_.get(); }

  void LaunchExtensionsImportAndWait(
      user_data_importer::SourceProfile source_profile) {
    base::RunLoop loop;
    ImportEndedObserver observer(loop.QuitClosure());

    // BraveExternalProcessImporterHost uses `delete this`.
    auto* external_process_host = new BraveExternalProcessImporterHost();

    external_process_host->DoNotLaunchImportForTesting();
    external_process_host->set_observer(&observer);
    external_process_host->StartImportSettings(
        source_profile, GetProfile(), user_data_importer::EXTENSIONS, nullptr);
    loop.Run();
  }

  std::string ReadTargetStore(const std::string& id) {
    return ReadStore(GetExtensionLocalSettingsPath("Brave", id), id);
  }

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  // Installs a TestPasswordStore on the target profile and writes a source
  // `Login Data` file (encrypted with the same OSCryptAsync the importer will
  // decrypt with) into the given source profile directory.
  void SetUpPasswordImport(const base::FilePath& source_path) {
    ProfilePasswordStoreFactory::GetInstance()->SetTestingFactoryAndUse(
        GetProfile(),
        base::BindRepeating(
            &password_manager::BuildPasswordStore<
                content::BrowserContext, password_manager::TestPasswordStore>));

    base::test::TestFuture<scoped_refptr<os_crypt_async::Encryptor>> future;
    TestingBrowserProcess::GetGlobal()->os_crypt_async()->GetInstance(
        future.GetCallback());
    scoped_refptr<os_crypt_async::Encryptor> encryptor = future.Take();

    ASSERT_TRUE(base::CreateDirectory(source_path));
    password_manager::LoginDatabase database(
        source_path.Append(FILE_PATH_LITERAL("Login Data")),
        password_manager::IsAccountStore(false));
    ASSERT_TRUE(database.Init(base::NullCallback(), std::move(encryptor)));
    password_manager::StoredCredential cred;
    cred.url = GURL("https://example.com/");
    cred.signon_realm = "https://example.com/";
    cred.username_value = u"alice";
    cred.password_value = u"secret";
    cred.in_store = password_manager::PasswordForm::Store::kProfileStore;
    ASSERT_FALSE(database.AddLogin(std::move(cred)).empty());
  }

  // Drives a password import to completion, populating `observer`.
  void LaunchPasswordImportAndWait(
      user_data_importer::SourceProfile source_profile,
      ImportEndedObserver* observer) {
    base::RunLoop loop;
    observer->set_quit_closure(loop.QuitClosure());

    // BraveExternalProcessImporterHost uses `delete this`.
    auto* external_process_host = new BraveExternalProcessImporterHost();
    external_process_host->DoNotLaunchImportForTesting();
    external_process_host->set_observer(observer);
    external_process_host->StartImportSettings(
        source_profile, GetProfile(), user_data_importer::PASSWORDS, nullptr);
    loop.Run();
  }
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

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
};

TEST_F(BraveExternalProcessImporterHostUnitTest, ImportEtensionsSettings) {
  user_data_importer::SourceProfile source_profile;
  source_profile.source_path = GetProductProfilePath("Chrome");
  source_profile.importer_type = user_data_importer::TYPE_CHROME;
  source_profile.services_supported = user_data_importer::EXTENSIONS;

  auto extension_installer = base::BindLambdaForTesting(
      [this](
          const std::string& id) -> extensions_import::ExtensionImportStatus {
        if (id == kExtensions[0]) {
          return extensions_import::ExtensionImportStatus::kFailedToInstall;
        }
        AddExtension(id);
        return extensions_import::ExtensionImportStatus::kOk;
      });

  auto installer_override = extensions_import::ExtensionsImporter::
      OverrideExtensionInstallerForTesting(&extension_installer);

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
}

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
// Exercises the browser-process password import lifecycle: `ImportEnded` must
// fire exactly once and the item start/end notifications must each fire once
// for PASSWORDS, and P3A must be recorded once (the guard against the
// NotifyImportEnded() re-entry).
TEST_F(BraveExternalProcessImporterHostUnitTest, ImportPasswordsLifecycle) {
  base::HistogramTester histogram_tester;

  user_data_importer::SourceProfile source_profile;
  source_profile.source_path = GetProductProfilePath("Brave");
  source_profile.importer_type = user_data_importer::TYPE_BRAVE;
  source_profile.services_supported = user_data_importer::PASSWORDS;
  ASSERT_NO_FATAL_FAILURE(SetUpPasswordImport(source_profile.source_path));

  ImportEndedObserver observer;
  LaunchPasswordImportAndWait(source_profile, &observer);

  EXPECT_EQ(1, observer.import_ended_count());
  EXPECT_EQ(1, observer.item_started_count(user_data_importer::PASSWORDS));
  EXPECT_EQ(1, observer.item_ended_count(user_data_importer::PASSWORDS));
  histogram_tester.ExpectTotalCount("Brave.Importer.ImporterSource.2", 1);
}
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
