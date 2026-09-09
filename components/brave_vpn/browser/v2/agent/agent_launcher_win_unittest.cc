/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_path_override.h"
#include "brave/components/brave_vpn/app/v2/agent/branding_buildflags.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher_internal.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2::internal {

// Overriding DIR_EXE keeps this hermetic: the result must not depend on
// whether the build directory happens to hold a built agent.
class AgentLauncherWinTest : public testing::Test {
 public:
  AgentLauncherWinTest() {
    CHECK(exe_dir_.CreateUniqueTempDir());
    exe_override_.emplace(base::DIR_EXE, exe_dir_.GetPath());
  }

  base::FilePath ExpectedAgentPath() const {
    return exe_dir_.GetPath().AppendASCII(BUILDFLAG(VPN_AGENT_EXE_NAME));
  }

 protected:
  base::ScopedTempDir exe_dir_;
  std::optional<base::ScopedPathOverride> exe_override_;
};

// Locks the install layout the browser relies on. If the installer stops
// placing the agent next to the executable, or renames it independently of
// VPN_AGENT_EXE_NAME, this is what should fail.
TEST_F(AgentLauncherWinTest, FindsAgentNextToModule) {
  const base::FilePath agent_path = ExpectedAgentPath();
  ASSERT_TRUE(base::WriteFile(agent_path, ""));

  const base::FilePath found_path = GetValidAgentPath();
  ASSERT_FALSE(found_path.empty());
  EXPECT_EQ(found_path, agent_path);

  // Independent of the constants above.
  EXPECT_EQ(found_path.Extension(), L".exe");
  EXPECT_EQ(found_path.DirName(), exe_dir_.GetPath());
}

// An empty result is the only signal the shared layer has for reporting
// kAppNotFound, so a missing executable must not come back as a usable path.
TEST_F(AgentLauncherWinTest, EmptyWhenAgentMissing) {
  EXPECT_TRUE(GetValidAgentPath().empty());
}

}  // namespace brave_vpn::v2::internal
