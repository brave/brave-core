// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/content_tool.h"

#include <string>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/content_extraction/script_tools.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kTestUrl[] = "https://example.com/page";
constexpr char kTestHost[] = "example.com";
// "example.com/page" (host + path) with non-alnum/underscore characters
// replaced by '_'. The path is part of the name so tools with the same name on
// different pages of the same host don't collapse.
constexpr char kTestHostPathSanitized[] = "example_com_page";

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

class MockPageContentExtractor : public mojom::PageContentExtractor {
 public:
  MockPageContentExtractor() = default;
  ~MockPageContentExtractor() override = default;

  MOCK_METHOD(void,
              ExtractPageContent,
              (ExtractPageContentCallback),
              (override));
  MOCK_METHOD(void,
              GetSearchSummarizerKey,
              (GetSearchSummarizerKeyCallback),
              (override));
  MOCK_METHOD(void,
              GetOpenAIChatButtonNonce,
              (GetOpenAIChatButtonNonceCallback),
              (override));
  MOCK_METHOD(void,
              ExecuteContentTool,
              (const std::string& name,
               const std::string& input_json,
               ExecuteContentToolCallback),
              (override));

  void Bind(mojo::ScopedMessagePipeHandle handle) {
    receiver_.Bind(
        mojo::PendingReceiver<mojom::PageContentExtractor>(std::move(handle)));
  }

 private:
  mojo::Receiver<mojom::PageContentExtractor> receiver_{this};
};

}  // namespace

class ContentToolTest : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    content::WebContentsTester::For(web_contents())
        ->NavigateAndCommit(GURL(kTestUrl));
  }

  content::WeakDocumentPtr weak_document() {
    return main_rfh()->GetWeakDocumentPtr();
  }

  // Binds a mock PageContentExtractor for the main frame so UseTool's mojo call
  // can be intercepted.
  MockPageContentExtractor* SetUpMockExtractor() {
    content::RenderFrameHostTester::For(web_contents()->GetPrimaryMainFrame())
        ->InitializeRenderFrameIfNeeded();
    service_manager::InterfaceProvider::TestApi test_api(
        web_contents()->GetPrimaryMainFrame()->GetRemoteInterfaces());
    mock_extractor_ = std::make_unique<MockPageContentExtractor>();
    test_api.SetBinderForName(
        mojom::PageContentExtractor::Name_,
        base::BindRepeating(&MockPageContentExtractor::Bind,
                            base::Unretained(mock_extractor_.get())));
    return mock_extractor_.get();
  }

  // Runs UseTool with `input_json` against a freshly-bound mock extractor and
  // returns the input_json the tool actually forwarded to the renderer.
  std::string ForwardedInputFor(const std::string& input_json) {
    MockPageContentExtractor* extractor = SetUpMockExtractor();
    auto mojo_tool = MakeScriptTool("noop", "");
    ContentTool tool(*mojo_tool, weak_document());

    base::test::TestFuture<std::string> input_future;
    EXPECT_CALL(*extractor,
                ExecuteContentTool(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(
            [&](const std::string& name, const std::string& forwarded,
                mojom::PageContentExtractor::ExecuteContentToolCallback cb) {
              input_future.SetValue(forwarded);
              std::move(cb).Run("ok");
            });
    tool.UseTool(input_json, base::DoNothing());
    return input_future.Get();
  }

 private:
  std::unique_ptr<MockPageContentExtractor> mock_extractor_;
};

TEST_F(ContentToolTest, NamePrefixesHostAndSanitizes) {
  auto mojo_tool = MakeScriptTool("highlight", "Highlight the page");
  ContentTool tool(*mojo_tool, weak_document());
  // Name format is "{sanitized-host}{sanitized-path}_{tool-name}". The dot in
  // the host and the slash in the path are replaced with underscores because
  // tool names only allow alphanumeric characters and underscores.
  EXPECT_EQ(tool.Name(),
            std::string(kTestHostPathSanitized) + std::string("_highlight"));
}

TEST_F(ContentToolTest, NameSanitizesDisallowedCharactersInToolName) {
  auto mojo_tool = MakeScriptTool("do-thing.now!", "");
  ContentTool tool(*mojo_tool, weak_document());
  EXPECT_EQ(tool.Name(), std::string(kTestHostPathSanitized) +
                             std::string("_do_thing_now_"));
}

TEST_F(ContentToolTest, DescriptionIncludesHostAndOriginalMetadata) {
  auto mojo_tool = MakeScriptTool("highlight", "Highlight the page");
  ContentTool tool(*mojo_tool, weak_document());
  // The description embeds the host, the page-provided tool name and the
  // page-provided description so the LLM has the context it needs.
  std::string description(tool.Description());
  EXPECT_NE(description.find(kTestHost), std::string::npos);
  EXPECT_NE(description.find("Name: highlight"), std::string::npos);
  EXPECT_NE(
      description.find("Website-provided description: Highlight the page"),
      std::string::npos);
}

TEST_F(ContentToolTest, MissingInputSchemaYieldsNoProperties) {
  auto mojo_tool = MakeScriptTool("noop", "");
  ContentTool tool(*mojo_tool, weak_document());
  EXPECT_FALSE(tool.InputProperties().has_value());
  EXPECT_FALSE(tool.RequiredProperties().has_value());
}

TEST_F(ContentToolTest, ParsesPropertiesAndRequired) {
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
  ContentTool tool(*mojo_tool, weak_document());

  auto input_properties = tool.InputProperties();
  ASSERT_TRUE(input_properties.has_value());
  EXPECT_TRUE(input_properties->contains("query"));
  EXPECT_TRUE(input_properties->contains("limit"));

  ASSERT_TRUE(tool.RequiredProperties().has_value());
  EXPECT_THAT(*tool.RequiredProperties(), ::testing::ElementsAre("query"));
}

TEST_F(ContentToolTest, MalformedSchemaFallsBackToNoProperties) {
  auto mojo_tool = MakeScriptTool("broken", "", std::string("not json"));
  ContentTool tool(*mojo_tool, weak_document());
  EXPECT_FALSE(tool.InputProperties().has_value());
  EXPECT_FALSE(tool.RequiredProperties().has_value());
}

TEST_F(ContentToolTest, SchemaWithoutPropertiesOrRequired) {
  // A schema that parses but lacks "properties"/"required" should still
  // construct cleanly with no input properties.
  auto mojo_tool =
      MakeScriptTool("ping", "", std::string(R"({"type":"object"})"));
  ContentTool tool(*mojo_tool, weak_document());
  EXPECT_FALSE(tool.InputProperties().has_value());
  EXPECT_FALSE(tool.RequiredProperties().has_value());
}

TEST_F(ContentToolTest, RequiresPermissionChallengeUntilGranted) {
  auto mojo_tool = MakeScriptTool("echo", "");
  ContentTool tool(*mojo_tool, weak_document());

  auto tool_use = mojom::ToolUseEvent::New();
  auto result = tool.RequiresUserInteractionBeforeHandling(*tool_use);
  ASSERT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(result));
  EXPECT_TRUE(std::get<mojom::PermissionChallengePtr>(result));

  tool.UserPermissionGranted(/*tool_use_id=*/"any");

  auto after = tool.RequiresUserInteractionBeforeHandling(*tool_use);
  ASSERT_TRUE(std::holds_alternative<bool>(after));
  EXPECT_FALSE(std::get<bool>(after));
}

TEST_F(ContentToolTest, UseToolNormalizesEmptyInputToObject) {
  // Models may emit an empty string for a tool that takes no parameters. The
  // renderer fails to parse "" as JSON, so UseTool must forward "{}" instead.
  EXPECT_EQ(ForwardedInputFor(/*input_json=*/""), "{}");
}

TEST_F(ContentToolTest, UseToolForwardsNonEmptyInputUnchanged) {
  EXPECT_EQ(ForwardedInputFor(R"({"query":"weather"})"),
            R"({"query":"weather"})");
}

TEST_F(ContentToolTest, UseToolForwardsEmptyIshJsonValuesUnchanged) {
  // Only a truly empty string is normalized. Valid JSON values that merely look
  // "empty" (null, an empty JSON string, an empty array, an empty object) are
  // non-empty strings and must be forwarded verbatim.
  for (const std::string args : {"null", R"("")", "[]", "{}"}) {
    EXPECT_EQ(ForwardedInputFor(args), args) << "args=" << args;
  }
}

TEST_F(ContentToolTest, UseToolAfterDocumentGoneReturnsEmpty) {
  // The WeakDocumentPtr can go stale between construction and UseTool (e.g.
  // when the underlying WebContents is destroyed). UseTool must short-circuit
  // and run the callback with empty results rather than crashing.
  auto mojo_tool = MakeScriptTool("noop", "");
  ContentTool tool(*mojo_tool, weak_document());

  DeleteContents();

  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> future;
  tool.UseTool("{}", future.GetCallback());
  auto [output, artifacts] = future.Take();
  EXPECT_TRUE(output.empty());
  EXPECT_TRUE(artifacts.empty());
}

}  // namespace ai_chat
