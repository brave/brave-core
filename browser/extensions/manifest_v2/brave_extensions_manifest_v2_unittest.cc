// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_installer.h"
#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_migrator.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/extension_management.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/install_signer.h"
#include "chrome/browser/extensions/install_verifier.h"
#include "chrome/browser/extensions/mv2_deprecation_impact_checker.h"
#include "chrome/browser/extensions/mv2_experiment_stage.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

struct BraveExtensionsManifestV2Test
    : public extensions::ExtensionServiceTestBase {
 public:
  void SetUp() override {
    extensions::ExtensionServiceTestBase::SetUp();

    InitializeExtensionService(ExtensionServiceInitParams());
  }

  void InitVerifier() {
    extensions::InstallSignature signature = {};
    auto dict = signature.ToDict();
    extensions::ExtensionPrefs::Get(profile())->SetInstallSignature(&dict);
    extensions::InstallVerifier::Get(profile())->Init();
  }

 private:
  extensions::ScopedInstallVerifierBypassForTest force_install_verification_{
      extensions::ScopedInstallVerifierBypassForTest::kForceOn};
};

TEST_F(BraveExtensionsManifestV2Test, CheckInstallVerifier) {
  struct {
    const char* extension_id;
    bool expected_must_remain_disabled;
    extensions::disable_reason::DisableReason expected_reason;
  } test_cases[] = {{extensions_mv2::kNoScriptId, false,
                     extensions::disable_reason::DISABLE_NONE},
                    {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", true,
                     extensions::disable_reason::DISABLE_NOT_VERIFIED}};

  InitVerifier();

  for (const auto& test : test_cases) {
    SCOPED_TRACE(::testing::Message() << test.extension_id);

    auto extension =
        extensions::ExtensionBuilder("test")
            .SetID(test.extension_id)
            .AddFlags(extensions::Extension::FROM_WEBSTORE)
            .SetLocation(extensions::mojom::ManifestLocation::kExternalPolicy)
            .Build();

    auto* install_verifier = extensions::InstallVerifier::Get(profile());

    auto disable_reason = extensions::disable_reason::DISABLE_NONE;
    EXPECT_EQ(
        test.expected_must_remain_disabled,
        install_verifier->MustRemainDisabled(extension.get(), &disable_reason));
    EXPECT_EQ(test.expected_reason, disable_reason);
  }
}

class BraveExtensionsManifestV2DeprecationTest
    : public BraveExtensionsManifestV2Test,
      public ::testing::WithParamInterface<extensions::MV2ExperimentStage> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveExtensionsManifestV2DeprecationTest,
    ::testing::Values(extensions::MV2ExperimentStage::kNone,
                      extensions::MV2ExperimentStage::kDisableWithReEnable,
                      extensions::MV2ExperimentStage::kUnsupported,
                      extensions::MV2ExperimentStage::kWarning));

TEST_P(BraveExtensionsManifestV2DeprecationTest,
       KnownMV2ExtensionsNotDeprecated) {
  extensions::MV2DeprecationImpactChecker checker(
      GetParam(),
      extensions::ExtensionManagementFactory::GetForBrowserContext(profile()));

  for (const auto& known_mv2 :
       extensions_mv2::kPreconfiguredManifestV2Extensions) {
    auto extension =
        extensions::ExtensionBuilder("test")
            .SetID(std::string(known_mv2))
            .AddFlags(extensions::Extension::FROM_WEBSTORE)
            .SetLocation(extensions::mojom::ManifestLocation::kExternalPolicy)
            .Build();
    EXPECT_FALSE(checker.IsExtensionAffected(*extension.get()));
  }
}

class BraveExtensionsManifestV2SettingsBackupTest
    : public extensions::ExtensionServiceTestBase {
 public:
  BraveExtensionsManifestV2SettingsBackupTest() = default;
  ~BraveExtensionsManifestV2SettingsBackupTest() override = default;

  void SetUp() override {
    extensions::ExtensionServiceTestBase::SetUp();
    InitializeExtensionService(ExtensionServiceInitParams());
  }

  void CopyTestIndexedDB(const extensions::ExtensionId& cws_id) {
    const auto test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    base::CopyDirectory(test_data_dir.AppendASCII("extensions")
                            .AppendASCII("mv2")
                            .AppendASCII(cws_id)
                            .AppendASCII("IndexedDB"),
                        profile()->GetPath(), true);
  }

  void CopyTestLocalSettings(const extensions::ExtensionId& cws_id) {
    const auto test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    base::CopyDirectory(
        test_data_dir.AppendASCII("extensions")
            .AppendASCII("mv2")
            .AppendASCII(cws_id)
            .Append(extensions::kLocalExtensionSettingsDirectoryName),
        profile()->GetPath(), true);
  }

  base::FilePath GetRelative(const base::FilePath& prefix,
                             const base::FilePath& absolute) {
    auto prefix_components = prefix.GetComponents();
    auto absolute_components = absolute.GetComponents();
    DCHECK_LE(prefix_components.size(), absolute_components.size());

    base::FilePath result;
    for (size_t i = 0; i < absolute_components.size(); ++i) {
      if (i < prefix_components.size() &&
          absolute_components[i] == prefix_components[i]) {
        continue;
      }
      result = result.Append(absolute_components[i]);
    }
    return result;
  }

  bool AreDirectoriesEqual(const base::FilePath& left,
                           const base::FilePath& right) {
    auto check = [this](const base::FilePath& left,
                        const base::FilePath& right) {
      base::FileEnumerator left_e(left, true, base::FileEnumerator::FILES);
      bool equals = true;
      left_e.ForEach([&equals, &left, &right,
                      this](const base::FilePath& file) {
        if (!equals ||
            !base::ContentsEqual(file, right.Append(GetRelative(left, file)))) {
          equals = false;
        }
      });
      return equals;
    };
    return check(left, right) && check(right, left);
  }

  void WaitForExtensionsFileOperations() {
    base::RunLoop loop;
    extensions::GetExtensionFileTaskRunner()->PostTaskAndReply(
        FROM_HERE, base::DoNothing(), loop.QuitClosure());
    loop.Run();
  }
};

TEST_F(BraveExtensionsManifestV2SettingsBackupTest, BackupSettings) {
  base::ScopedAllowBlockingForTesting allow_io;
  {
    // Install uBlock from CWS.
    auto extension =
        extensions::ExtensionBuilder("test")
            .SetID(extensions_mv2::kCwsUBlockId)
            .AddFlags(extensions::Extension::FROM_WEBSTORE)
            .SetLocation(extensions::mojom::ManifestLocation::kExternalPolicy)
            .Build();
    registrar()->AddExtension(extension);
    CopyTestIndexedDB(extensions_mv2::kCwsUBlockId);
    CopyTestLocalSettings(extensions_mv2::kCwsUBlockId);

    // Disable it with unsupported manifest reason. It triggers the migrator to
    // save settings.
    registrar()->DisableExtension(
        extension->id(),
        {extensions::disable_reason::DISABLE_UNSUPPORTED_MANIFEST_VERSION});
    WaitForExtensionsFileOperations();

    // Check indexed settings are saved in the backup dir.
    EXPECT_TRUE(
        AreDirectoriesEqual(profile()->GetPath().AppendASCII("IndexedDB"),
                            profile()
                                ->GetPath()
                                .AppendASCII("ExtensionsMV2Backup")
                                .AppendASCII("IndexedDB")));
    EXPECT_TRUE(AreDirectoriesEqual(
        profile()->GetPath().Append(
            extensions::kLocalExtensionSettingsDirectoryName),
        profile()
            ->GetPath()
            .AppendASCII("ExtensionsMV2Backup")
            .Append(extensions::kLocalExtensionSettingsDirectoryName)));
  }

  {
    // Install uBlock from Brave host.
    auto extension =
        extensions::ExtensionBuilder("test")
            .SetID(extensions_mv2::kUBlockId)
            .AddFlags(extensions::Extension::FROM_WEBSTORE)
            .SetLocation(extensions::mojom::ManifestLocation::kExternalPolicy)
            .Build();
    registrar()->AddExtension(extension);
    registry()->TriggerOnInstalled(extension.get(), false);

    // It triggers the migrator to import settings from backup.
    WaitForExtensionsFileOperations();

    // Settings are moved from backup to the indexeddb dir with the brave-hosted
    // id.
    EXPECT_TRUE(base::IsDirectoryEmpty(profile()
                                           ->GetPath()
                                           .AppendASCII("ExtensionsMV2Backup")
                                           .AppendASCII("IndexedDB")));
    EXPECT_TRUE(base::IsDirectoryEmpty(
        profile()
            ->GetPath()
            .AppendASCII("ExtensionsMV2Backup")
            .Append(extensions::kLocalExtensionSettingsDirectoryName)));

    auto check_settings_dir = [this](const base::FilePath& path) {
      base::FileEnumerator cws_settings(
          profile()->GetPath().Append(path), false,
          base::FileEnumerator::DIRECTORIES,
          base::FilePath::FromASCII(
              base::StrCat({"*", extensions_mv2::kUBlockId, "*"}))
              .value());
      bool checked = false;
      cws_settings.ForEach([&checked, this](const base::FilePath& path) {
        checked = true;
        auto brave_hosted_name = path.BaseName().value();
        base::ReplaceFirstSubstringAfterOffset(
            &brave_hosted_name, 0,
            base::FilePath::FromASCII(extensions_mv2::kCwsUBlockId).value(),
            base::FilePath::FromASCII(extensions_mv2::kUBlockId).value());

        EXPECT_TRUE(AreDirectoriesEqual(
            path, path.DirName().Append(brave_hosted_name)));
      });
      EXPECT_TRUE(checked);
    };

    check_settings_dir(base::FilePath::FromASCII("IndexedDB"));
    check_settings_dir(
        base::FilePath(extensions::kLocalExtensionSettingsDirectoryName));
  }
}
