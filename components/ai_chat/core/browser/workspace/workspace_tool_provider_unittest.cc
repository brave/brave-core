// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace/workspace_tool_provider.h"

#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class WorkspaceToolProviderTest : public testing::Test {
 public:
  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  WorkspaceService service_;
  const std::string conv_ = "conv-1";
};

TEST_F(WorkspaceToolProviderTest, NoToolsWhenFeatureDisabled) {
  feature_list_.InitAndDisableFeature(features::kAIChatWorkspaceTools);
  WorkspaceToolProvider provider(service_.GetWeakPtr());
  provider.OnConversationHandlerReady(conv_);
  service_.SetWorkspaceRoot(conv_, temp_dir_.GetPath());
  EXPECT_TRUE(provider.GetTools().empty());
}

TEST_F(WorkspaceToolProviderTest, NoToolsWithoutWorkspace) {
  feature_list_.InitAndEnableFeature(features::kAIChatWorkspaceTools);
  WorkspaceToolProvider provider(service_.GetWeakPtr());
  provider.OnConversationHandlerReady(conv_);
  EXPECT_TRUE(provider.GetTools().empty());
}

TEST_F(WorkspaceToolProviderTest, NoToolsBeforeConversationReady) {
  feature_list_.InitAndEnableFeature(features::kAIChatWorkspaceTools);
  WorkspaceToolProvider provider(service_.GetWeakPtr());
  // OnConversationHandlerReady not called, so no conversation id.
  EXPECT_TRUE(provider.GetTools().empty());
}

TEST_F(WorkspaceToolProviderTest, ToolsWhenEnabledAndWorkspaceSet) {
  feature_list_.InitAndEnableFeature(features::kAIChatWorkspaceTools);
  WorkspaceToolProvider provider(service_.GetWeakPtr());
  provider.OnConversationHandlerReady(conv_);
  service_.SetWorkspaceRoot(conv_, temp_dir_.GetPath());
  EXPECT_FALSE(provider.GetTools().empty());
}

}  // namespace ai_chat
