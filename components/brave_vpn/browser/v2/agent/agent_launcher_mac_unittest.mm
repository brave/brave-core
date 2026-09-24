/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#include "base/apple/bundle_locations.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher_internal.h"
#include "brave/components/brave_vpn/common/v2/branding_buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2::internal {

// Overriding the framework bundle path keeps this hermetic: the result must not
// depend on whether the build directory happens to hold an assembled framework.
class AgentLauncherMacTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(framework_dir_.CreateUniqueTempDir());
    base::apple::SetOverrideFrameworkBundlePath(framework_dir_.GetPath());
  }

  void TearDown() override { base::apple::SetOverrideFrameworkBundle(nil); }

  base::FilePath ExpectedAgentPath() const {
    return framework_dir_.GetPath().AppendASCII("Helpers").AppendASCII(
        BUILDFLAG(VPN_AGENT_APP_NAME));
  }

 protected:
  base::ScopedTempDir framework_dir_;
};

// Locks the packaging layout the browser relies on.
TEST_F(AgentLauncherMacTest, FindsAgentInFrameworkHelpers) {
  const base::FilePath agent_path = ExpectedAgentPath();
  ASSERT_TRUE(base::CreateDirectory(agent_path));

  const base::FilePath found_path = GetValidAgentPath();
  ASSERT_FALSE(found_path.empty());
  EXPECT_EQ(found_path, agent_path);

  // Independent of the constants above: the agent must be a bundle sitting
  // directly inside the framework's Helpers directory.
  EXPECT_EQ(found_path.Extension(), ".app");
  EXPECT_EQ(found_path.DirName().BaseName().value(), "Helpers");
  EXPECT_EQ(found_path.DirName().DirName(), framework_dir_.GetPath());
}

// An empty result is the only signal the shared layer has for reporting
// kAppNotFound, so a missing bundle must not come back as a usable path.
// The Helpers directory existing is not enough: guards against a check that
// stops one level too high.
TEST_F(AgentLauncherMacTest, EmptyWhenAgentBundleMissing) {
  EXPECT_TRUE(GetValidAgentPath().empty());
  ASSERT_TRUE(
      base::CreateDirectory(framework_dir_.GetPath().AppendASCII("Helpers")));
  EXPECT_TRUE(GetValidAgentPath().empty());
}

}  // namespace brave_vpn::v2::internal
