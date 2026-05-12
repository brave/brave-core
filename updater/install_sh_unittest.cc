// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// install_sh_test.py contains behavioral tests for Brave's customizations to
// chrome/updater/mac/.install.sh. Run it as a subprocess and fail if any of
// its Python tests fail.
TEST(InstallShTest, RunTestScript) {
  base::FilePath script =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendUTF8("brave")
          .AppendUTF8("updater")
          .AppendUTF8("install_sh_test.py");
  base::CommandLine cmd(base::FilePath("python3"));
  cmd.AppendArgPath(script);
  std::string output;
  ASSERT_TRUE(base::GetAppOutputAndError(cmd, &output)) << output;
}

}  // namespace
