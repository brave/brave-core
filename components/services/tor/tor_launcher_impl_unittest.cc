/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/tor/tor_launcher_impl.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/run_until.h"
#include "base/test/scoped_path_override.h"
#include "base/test/task_environment.h"
#include "brave/components/tor/constants.h"
#include "components/component_updater/component_updater_paths.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tor {
namespace {

class TorLauncherImplTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(TorLauncherImplTest, DeletesStateFileOnReceiverDisconnect) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::ScopedPathOverride path_override(component_updater::DIR_COMPONENT_USER,
                                         temp_dir.GetPath());

  const base::FilePath data_path = GetTorDataPath();
  ASSERT_TRUE(base::CreateDirectory(data_path));

  const base::FilePath state_file = data_path.AppendASCII("state");
  ASSERT_TRUE(base::WriteFile(state_file, "state contents"));
  const base::FilePath other_file = data_path.AppendASCII("other");
  ASSERT_TRUE(base::WriteFile(other_file, "other contents"));

  mojo::Remote<mojom::TorLauncher> remote;
  TorLauncherImpl launcher(remote.BindNewPipeAndPassReceiver());
  remote.reset();

  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !base::PathExists(state_file); }));
  ASSERT_TRUE(base::PathExists(other_file));
}

}  // namespace
}  // namespace tor
