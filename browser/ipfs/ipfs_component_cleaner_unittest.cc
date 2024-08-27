// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ipfs/ipfs_component_cleaner.h"

#include <fstream>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
#if BUILDFLAG(IS_WIN)
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("lnbclahgobmjphilkalbhebakmblnbij");
#elif BUILDFLAG(IS_MAC)
#if defined(ARCH_CPU_ARM64)
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("lejaflgbgglfaomemffoaappaihfligf");
#else
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("nljcddpbnaianmglkpkneakjaapinabi");
#endif
#elif BUILDFLAG(IS_LINUX)
#if defined(ARCH_CPU_ARM64)
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("fmmldihckdnognaabhligdpckkeancng");
#else
static const base::FilePath::StringPieceType kIpfsClientComponentId =
    FILE_PATH_LITERAL("oecghfpdmkjlhnfpmmjegjacfimiafjp");
#endif
#endif

// Simple function to dump some text into a new file.
void CreateTextFile(const base::FilePath& filename,
                    const std::wstring& contents) {
  std::wofstream file;
#if BUILDFLAG(IS_WIN)
  file.open(filename.value().c_str());
#elif BUILDFLAG(IS_POSIX) || BUILDFLAG(IS_FUCHSIA)
  file.open(filename.value());
#endif  // BUILDFLAG(IS_WIN)
  ASSERT_TRUE(file.is_open());
  file << contents;
  file.close();
}
}  // namespace

class IpfsComponentCleanerUnitTest : public testing::Test {
 public:
  IpfsComponentCleanerUnitTest() = default;
  ~IpfsComponentCleanerUnitTest() override = default;

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath user_data_dir_tmp =
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("user_data"));
    ASSERT_TRUE(
        base::PathService::Override(chrome::DIR_USER_DATA, user_data_dir_tmp));
    user_data_path_ = base::PathService::CheckedGet(chrome::DIR_USER_DATA);
  }

  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  base::FilePath user_data_path_;
};

TEST_F(IpfsComponentCleanerUnitTest, CleanIpfsComponent) {
  EXPECT_FALSE(user_data_path_.empty());
  base::FilePath cache_folder =
      user_data_path_.Append(FILE_PATH_LITERAL("brave_ipfs"));
  base::CreateDirectory(cache_folder);
  EXPECT_TRUE(base::PathExists(cache_folder));
  base::FilePath cache_folder_subdir =
      cache_folder.Append(FILE_PATH_LITERAL("subdir1"));
  base::CreateDirectory(cache_folder_subdir);
  EXPECT_TRUE(base::PathExists(cache_folder_subdir));
  base::FilePath cache_folder_subdir_file_01 =
      cache_folder_subdir.Append(FILE_PATH_LITERAL("The file 01.txt"));
  CreateTextFile(cache_folder_subdir_file_01, L"12345678901234567890");

  base::FilePath component_id_folder =
      user_data_path_.Append(base::FilePath(kIpfsClientComponentId));
  base::CreateDirectory(component_id_folder);
  EXPECT_TRUE(base::PathExists(component_id_folder));
  base::FilePath component_id_folde_subdir =
      component_id_folder.Append(FILE_PATH_LITERAL("subdir1"));
  base::CreateDirectory(component_id_folde_subdir);
  EXPECT_TRUE(base::PathExists(component_id_folde_subdir));
  base::FilePath component_id_folde_subdir_file_01 =
      component_id_folde_subdir.Append(FILE_PATH_LITERAL("The file 01.txt"));
  CreateTextFile(component_id_folde_subdir_file_01, L"12345678901234567890");

  ipfs::CleanupIpfsComponent();
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(base::PathExists(cache_folder));
  EXPECT_FALSE(base::PathExists(component_id_folder));
}
