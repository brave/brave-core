/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <windows.h>

#include <string>

#include "base/test/test_reg_util_win.h"
#include "base/version.h"
#include "chrome/installer/setup/setup_constants.h"

#include <chrome/installer/setup/unpack_archive_unittest.cc>

namespace installer {

// This test used to be upstream and had to be restored in Brave to support
// delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
TEST_P(SetupUnpackArchiveTest, UnpackArchiveSetsUncompressedArchive) {
  base::FilePath chrome_archive = GetParam().test_file;

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::CommandLine cmd_line = base::CommandLine::FromString(L"setup.exe");
  cmd_line.AppendSwitchPath(GetParam().archive_switch, chrome_archive);
  FakeInstallerState installer_state;

  ASSERT_THAT(
      UnpackChromeArchive(
          temp_dir.GetPath(),
          base::FilePath(),  // Unused when archive is provided via cmd_line.
          cmd_line, installer_state),
      base::test::HasValue());

  EXPECT_EQ(installer_state.uncompressed_archive,
            GetParam().uncompressed_output_matches_input_file
                ? chrome_archive
                : temp_dir.GetPath().Append(FILE_PATH_LITERAL("chrome.7z")));
  ASSERT_EQ(installer_state.archive_type, ArchiveType::FULL_ARCHIVE_TYPE);
}

namespace {

base::FilePath GetBraveTestDataPath() {
  base::FilePath test_data_root;
  base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &test_data_root);
  return test_data_root.Append(FILE_PATH_LITERAL("brave"))
      .Append(FILE_PATH_LITERAL("test"))
      .Append(FILE_PATH_LITERAL("data"))
      .Append(FILE_PATH_LITERAL("installer"));
}

// Adds `data` as an LZMA ("B7") resource named `name` to the PE file at
// `pe_file`.
bool AddLZMAResource(const base::FilePath& pe_file,
                     const wchar_t* name,
                     const std::string& data) {
  HANDLE handle = ::BeginUpdateResource(pe_file.value().c_str(),
                                        /*bDeleteExistingResources=*/FALSE);
  if (!handle) {
    return false;
  }
  const bool updated = ::UpdateResource(
      handle, kLZMAResourceType, name,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), const_cast<char*>(data.data()),
      static_cast<DWORD>(data.size()));
  return ::EndUpdateResource(handle, /*fDiscard=*/!updated) && updated;
}

}  // namespace

// Exercises the differential update path that used to be upstream and had to
// be restored in Brave to support delta updates on Windows until we are on
// Omaha 4 (see github.com/brave/brave-core/pull/31937): a compressed archive
// holds a patch file rather than chrome.7z, and the patch is applied to the
// installed version's chrome.7z.
//
// The test data is set up so that patching plays out with upstream's test
// files: brave/test/data/installer/test_patch.packed.7z holds a copy of
// zucchini_archive.diff, which patches archive1.7z into archive2.7z.
class SetupUnpackArchiveDeltaTest : public testing::Test {
 protected:
  void SetUp() override {
    // Contain the registry accesses of InstallationState::Initialize() and
    // InstallerState::SetStage().
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_LOCAL_MACHINE));
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_CURRENT_USER));
    ASSERT_TRUE(unpack_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(target_dir_.CreateUniqueTempDir());

    // Stage the installed version's archive that the patch will be applied to,
    // at <target_path>\<previous_version>\Installer\chrome.7z.
    const base::FilePath installer_dir =
        target_dir_.GetPath()
            .AppendASCII(previous_version_.GetString())
            .Append(kInstallerDir);
    ASSERT_TRUE(base::CreateDirectory(installer_dir));
    ASSERT_TRUE(base::CopyFile(
        GetTestFileRootPath().Append(FILE_PATH_LITERAL("archive1.7z")),
        installer_dir.Append(kChromeArchive)));
  }

  base::CommandLine MakeCmdLine() const {
    base::CommandLine cmd_line = base::CommandLine::FromString(L"setup.exe");
    cmd_line.AppendSwitchASCII(switches::kPreviousVersion,
                               previous_version_.GetString());
    return cmd_line;
  }

  void RunAndExpectPatchedArchive(const base::CommandLine& cmd_line) {
    FakeInstallerState installer_state;
    installer_state.set_target_path_for_testing(target_dir_.GetPath());

    ASSERT_THAT(UnpackChromeArchive(unpack_dir_.GetPath(), base::FilePath(),
                                    cmd_line, installer_state),
                base::test::HasValue());

    EXPECT_EQ(installer_state.archive_type,
              ArchiveType::INCREMENTAL_ARCHIVE_TYPE);
    const base::FilePath patched_archive =
        unpack_dir_.GetPath().Append(kChromeArchive);
    EXPECT_EQ(installer_state.uncompressed_archive, patched_archive);

    // Patching archive1.7z with the extracted patch must yield archive2.7z.
    EXPECT_TRUE(base::ContentsEqual(
        patched_archive,
        GetTestFileRootPath().Append(FILE_PATH_LITERAL("archive2.7z"))));

    // The patched archive was then unpacked; archive2.7z contains b.exe.
    EXPECT_TRUE(base::PathExists(
        unpack_dir_.GetPath().Append(FILE_PATH_LITERAL("b.exe"))));

    // The extracted patch file was deleted after it was applied.
    EXPECT_FALSE(base::PathExists(
        unpack_dir_.GetPath().Append(FILE_PATH_LITERAL("chrome_patch.diff"))));
  }

  const base::Version previous_version_{"1.0.0.0"};
  base::ScopedTempDir unpack_dir_;
  base::ScopedTempDir target_dir_;

 private:
  registry_util::RegistryOverrideManager registry_override_manager_;
};

TEST_F(SetupUnpackArchiveDeltaTest, PatchFromInstallArchive) {
  base::CommandLine cmd_line = MakeCmdLine();
  cmd_line.AppendSwitchPath(
      switches::kInstallArchive,
      GetBraveTestDataPath().Append(FILE_PATH_LITERAL("test_patch.packed.7z")));
  RunAndExpectPatchedArchive(cmd_line);
}

// Deltas served via Omaha arrive as a mini_installer whose compressed archive
// resource holds the patch archive. Verify the patch path also works in the
// resource extraction mode added in Chromium revision 8624d4109b29d024a6da3f15.
TEST_F(SetupUnpackArchiveDeltaTest, PatchFromMiniInstallerResource) {
  std::string packed_patch;
  ASSERT_TRUE(base::ReadFileToString(
      GetBraveTestDataPath().Append(FILE_PATH_LITERAL("test_patch.packed.7z")),
      &packed_patch));
  const base::FilePath mini_installer =
      target_dir_.GetPath().Append(FILE_PATH_LITERAL("mini_installer.exe"));
  ASSERT_TRUE(base::CopyFile(GetTestFileRootPath().Append(
                                 FILE_PATH_LITERAL("mini_installer.exe.test")),
                             mini_installer));
  ASSERT_TRUE(
      AddLZMAResource(mini_installer, L"PATCH.PACKED.7Z", packed_patch));

  base::CommandLine cmd_line = MakeCmdLine();
  cmd_line.AppendSwitchPath(switches::kMiniInstallerPath, mini_installer);
  cmd_line.AppendSwitchNative(switches::kArchiveResourceName,
                              L"PATCH.PACKED.7Z");
  cmd_line.AppendSwitchNative(switches::kArchiveResourceType,
                              kLZMAResourceType);
  RunAndExpectPatchedArchive(cmd_line);
}

}  // namespace installer
