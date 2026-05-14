// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/script_tool.h"

#include <string>
#include <vector>

#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/content_extraction/script_tools.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kTestUrl[] = "https://example.com/page";
constexpr char kTestHost[] = "example.com";
// "example.com" with non-alnum/underscore characters replaced by '_'.
constexpr char kTestHostSanitized[] = "example_com";

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

class ScriptToolTest : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    content::WebContentsTester::For(web_contents())
        ->NavigateAndCommit(GURL(kTestUrl));
  }

  content::WeakDocumentPtr weak_document() {
    return main_rfh()->GetWeakDocumentPtr();
  }
};

TEST_F(ScriptToolTest, NamePrefixesHostAndSanitizes) {
  auto mojo_tool = MakeScriptTool("highlight", "Highlight the page");
  ScriptTool tool(*mojo_tool, weak_document());
  // Name format is "{sanitized-host}_{tool-name}". The dot in the host is
  // replaced with an underscore because tool names only allow alphanumeric
  // characters and underscores.
  EXPECT_EQ(tool.Name(),
            std::string(kTestHostSanitized) + std::string("_highlight"));
}

TEST_F(ScriptToolTest, NameSanitizesDisallowedCharactersInToolName) {
  auto mojo_tool = MakeScriptTool("do-thing.now!", "");
  ScriptTool tool(*mojo_tool, weak_document());
  EXPECT_EQ(tool.Name(),
            std::string(kTestHostSanitized) + std::string("_do_thing_now_"));
}

TEST_F(ScriptToolTest, DescriptionIncludesHostAndOriginalMetadata) {
  auto mojo_tool = MakeScriptTool("highlight", "Highlight the page");
  ScriptTool tool(*mojo_tool, weak_document());
  // The description embeds the host, the page-provided tool name and the
  // page-provided description so the LLM has the context it needs.
  std::string description(tool.Description());
  EXPECT_NE(description.find(kTestHost), std::string::npos);
  EXPECT_NE(description.find("Name: highlight"), std::string::npos);
  EXPECT_NE(
      description.find("Website-provided description: Highlight the page"),
      std::string::npos);
}

TEST_F(ScriptToolTest, MissingInputSchemaYieldsNoProperties) {
  auto mojo_tool = MakeScriptTool("noop", "");
  ScriptTool tool(*mojo_tool, weak_document());
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
  ScriptTool tool(*mojo_tool, weak_document());

  auto input_properties = tool.InputProperties();
  ASSERT_TRUE(input_properties.has_value());
  EXPECT_TRUE(input_properties->contains("query"));
  EXPECT_TRUE(input_properties->contains("limit"));

  ASSERT_TRUE(tool.RequiredProperties().has_value());
  EXPECT_THAT(*tool.RequiredProperties(), ::testing::ElementsAre("query"));
}

TEST_F(ScriptToolTest, MalformedSchemaFallsBackToNoProperties) {
  auto mojo_tool = MakeScriptTool("broken", "", std::string("not json"));
  ScriptTool tool(*mojo_tool, weak_document());
  EXPECT_FALSE(tool.InputProperties().has_value());
  EXPECT_FALSE(tool.RequiredProperties().has_value());
}

TEST_F(ScriptToolTest, SchemaWithoutPropertiesOrRequired) {
  // A schema that parses but lacks "properties"/"required" should still
  // construct cleanly with no input properties.
  auto mojo_tool =
      MakeScriptTool("ping", "", std::string(R"({"type":"object"})"));
  ScriptTool tool(*mojo_tool, weak_document());
  EXPECT_FALSE(tool.InputProperties().has_value());
  EXPECT_FALSE(tool.RequiredProperties().has_value());
}

TEST_F(ScriptToolTest, RequiresPermissionChallengeUntilGranted) {
  auto mojo_tool = MakeScriptTool("echo", "");
  ScriptTool tool(*mojo_tool, weak_document());

  auto tool_use = mojom::ToolUseEvent::New();
  auto result = tool.RequiresUserInteractionBeforeHandling(*tool_use);
  ASSERT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(result));
  EXPECT_TRUE(std::get<mojom::PermissionChallengePtr>(result));

  tool.UserPermissionGranted(/*tool_use_id=*/"any");

  auto after = tool.RequiresUserInteractionBeforeHandling(*tool_use);
  ASSERT_TRUE(std::holds_alternative<bool>(after));
  EXPECT_FALSE(std::get<bool>(after));
}

TEST_F(ScriptToolTest, UseToolAfterDocumentGoneReturnsEmpty) {
  // The WeakDocumentPtr can go stale between construction and UseTool (e.g.
  // when the underlying WebContents is destroyed). UseTool must short-circuit
  // and run the callback with empty results rather than crashing.
  auto mojo_tool = MakeScriptTool("noop", "");
  ScriptTool tool(*mojo_tool, weak_document());

  DeleteContents();

  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> future;
  tool.UseTool("{}", future.GetCallback());
  auto [output, artifacts] = future.Take();
  EXPECT_TRUE(output.empty());
  EXPECT_TRUE(artifacts.empty());
}

}  // namespace ai_chat
