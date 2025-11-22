/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/installer/setup/unpack_archive_unittest.cc>

namespace installer {

// This test used to be upstream and had to be restored in Brave to support
// delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
TEST_P(SetupUnpackArchiveTest, UnpackArchiveSetsUncompressedArchive) {
  base::FilePath chrome_archive = GetParam().test_file;

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  InstallationState original_state;  // Unused when not patching.
  base::CommandLine cmd_line = base::CommandLine::FromString(L"setup.exe");
  cmd_line.AppendSwitchPath(GetParam().archive_switch, chrome_archive);
  FakeInstallerState installer_state;

  ASSERT_THAT(
      UnpackChromeArchive(
          temp_dir.GetPath(), original_state,
          base::FilePath(),  // Unused when archive is provided via cmd_line.
          cmd_line, installer_state),
      base::test::HasValue());

  EXPECT_EQ(installer_state.uncompressed_archive,
            GetParam().uncompressed_output_matches_input_file
                ? chrome_archive
                : temp_dir.GetPath().Append(FILE_PATH_LITERAL("chrome.7z")));
  ASSERT_EQ(installer_state.archive_type, ArchiveType::FULL_ARCHIVE_TYPE);
}

}  // namespace installer
