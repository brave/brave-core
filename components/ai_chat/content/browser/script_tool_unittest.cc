// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/script_tool.h"

#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "content/public/browser/weak_document_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/content_extraction/script_tools.mojom.h"

namespace ai_chat {

namespace {

blink::mojom::ScriptToolPtr MakeScriptTool(
    const std::string& name,
    const std::string& description,
    std::optional<std::string> input_schema = std::nullopt) {
  auto tool = blink::mojom::ScriptTool::New();
  tool->name = name;
  tool->description = description;
  tool->input_schema = std::move(input_schema);
  return tool;
}

}  // namespace

class ScriptToolTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(ScriptToolTest, NameAndDescription) {
  auto mojo_tool = MakeScriptTool("highlight", "Highlight the page");
  ScriptTool tool(*mojo_tool, content::WeakDocumentPtr());
  EXPECT_EQ(tool.Name(), "highlight");
  EXPECT_EQ(tool.Description(), "Highlight the page");
}

TEST_F(ScriptToolTest, MissingInputSchemaYieldsNoProperties) {
  auto mojo_tool = MakeScriptTool("noop", "");
  ScriptTool tool(*mojo_tool, content::WeakDocumentPtr());
  EXPECT_FALSE(tool.InputProperties().has_value());
  EXPECT_FALSE(tool.RequiredProperties().has_value());
}

TEST_F(ScriptToolTest, ParsesPropertiesAndRequired) {
  // input_schema follows the JSON Schema convention used by WebMCP.
  static constexpr char kSchema[] = R"({
    "type": "object",
    "properties": {
      "query": {"type": "string", "description": "search query"},
      "limit": {"type": "number"}
    },
    "required": ["query"]
  })";
  auto mojo_tool = MakeScriptTool("search", "Search", std::string(kSchema));
  ScriptTool tool(*mojo_tool, content::WeakDocumentPtr());

  auto input_properties = tool.InputProperties();
  ASSERT_TRUE(input_properties.has_value());
  EXPECT_TRUE(input_properties->contains("query"));
  EXPECT_TRUE(input_properties->contains("limit"));

  ASSERT_TRUE(tool.RequiredProperties().has_value());
  EXPECT_THAT(*tool.RequiredProperties(), ::testing::ElementsAre("query"));
}

TEST_F(ScriptToolTest, MalformedSchemaFallsBackToNoProperties) {
  auto mojo_tool = MakeScriptTool("broken", "", std::string("not json"));
  ScriptTool tool(*mojo_tool, content::WeakDocumentPtr());
  EXPECT_FALSE(tool.InputProperties().has_value());
  EXPECT_FALSE(tool.RequiredProperties().has_value());
}

TEST_F(ScriptToolTest, SchemaWithoutPropertiesOrRequired) {
  // A schema that parses but lacks "properties"/"required" should still
  // construct cleanly with no input properties.
  auto mojo_tool =
      MakeScriptTool("ping", "", std::string(R"({"type":"object"})"));
  ScriptTool tool(*mojo_tool, content::WeakDocumentPtr());
  EXPECT_FALSE(tool.InputProperties().has_value());
  EXPECT_FALSE(tool.RequiredProperties().has_value());
}

TEST_F(ScriptToolTest, UseToolWithoutLiveDocumentReturnsEmpty) {
  // An empty WeakDocumentPtr resolves to nullptr in
  // AsRenderFrameHostIfValid(); UseTool must short-circuit and run the
  // callback with empty results rather than crashing.
  auto mojo_tool = MakeScriptTool("noop", "");
  ScriptTool tool(*mojo_tool, content::WeakDocumentPtr());

  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> future;
  tool.UseTool("{}", future.GetCallback());
  auto [output, artifacts] = future.Take();
  EXPECT_TRUE(output.empty());
  EXPECT_TRUE(artifacts.empty());
}

}  // namespace ai_chat
