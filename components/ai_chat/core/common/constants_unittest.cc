/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/constants.h"

#include <string_view>
#include <tuple>

#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {
using HostExpectation = std::tuple<std::string_view, bool, bool>;
}  // namespace

TEST(AIChatCommonConstantsUnitTest, WorkspaceHostShapes) {
  // The first bool is IsAIChatLeoWorkspaceHost(), the second
  // IsAIChatLeoWorkspaceViewHost(). The label is an opaque id, so only the
  // shape matters, never what it reads as.
  const HostExpectation expectations[] = {
      // A workspace's own host: exactly one label before the suffix.
      {"abc.leo-workspace", true, false},
      // The registered host itself is not a workspace.
      {"leo-workspace", false, false},
      // Nor an empty label.
      {".leo-workspace", false, false},
      // The suffix must be at the end.
      {"abc.leo-workspace.evil", false, false},
      // A viewer is not the workspace it belongs to.
      {"view.abc.leo-workspace", false, true},
      // The host that looks like a viewer of the workspace host is just a
      // workspace whose id happens to be "view".
      {"view.leo-workspace", true, false},
      // A viewer belongs to exactly one workspace, at a fixed depth.
      {"view.a.b.leo-workspace", false, false},
      {"view.view.abc.leo-workspace", false, false},
  };
  for (const auto& [host, is_workspace, is_view] : expectations) {
    EXPECT_EQ(is_workspace, IsAIChatLeoWorkspaceHost(host)) << host;
    EXPECT_EQ(is_view, IsAIChatLeoWorkspaceViewHost(host)) << host;
  }
}

}  // namespace ai_chat
