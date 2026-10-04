// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/proto_conversion.h"

#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

// Tests for WebSourcesEvent conversion functions

TEST(ProtoConversionTest, SerializeDeserializeWebSourcesEvent_ValidData) {
  // Create mojom WebSourcesEvent
  auto mojom_event = mojom::WebSourcesEvent::New();

  auto source1 = mojom::WebSource::New(
      "Test Title 1", GURL("https://example.com/page1"),
      GURL("https://example.com/favicon1.ico"), std::nullopt, std::nullopt);
  mojom_event->sources.push_back(std::move(source1));

  auto source2 = mojom::WebSource::New(
      "Test Title 2", GURL("https://example.com/page2"),
      GURL("https://example.com/favicon2.ico"), std::nullopt, std::nullopt);
  mojom_event->sources.push_back(std::move(source2));

  // Add rich_results
  mojom_event->rich_results.push_back(
      R"({"type":"knowledge_graph","title":"Test Knowledge Graph"})");
  mojom_event->rich_results.push_back(
      R"({"type":"video","url":"https://example.com/video.mp4"})");

  // Serialize to proto
  store::WebSourcesEventProto proto_event;
  SerializeWebSourcesEvent(mojom_event, &proto_event);

  // Verify proto data
  ASSERT_EQ(proto_event.sources_size(), 2);
  EXPECT_EQ(proto_event.sources(0).title(), "Test Title 1");
  EXPECT_EQ(proto_event.sources(0).url(), "https://example.com/page1");
  EXPECT_EQ(proto_event.sources(0).favicon_url(),
            "https://example.com/favicon1.ico");
  EXPECT_EQ(proto_event.sources(1).title(), "Test Title 2");
  EXPECT_EQ(proto_event.sources(1).url(), "https://example.com/page2");
  EXPECT_EQ(proto_event.sources(1).favicon_url(),
            "https://example.com/favicon2.ico");

  // Verify rich_results in proto
  ASSERT_EQ(proto_event.rich_results_size(), 2);
  EXPECT_EQ(proto_event.rich_results(0),
            R"({"type":"knowledge_graph","title":"Test Knowledge Graph"})");
  EXPECT_EQ(proto_event.rich_results(1),
            R"({"type":"video","url":"https://example.com/video.mp4"})");

  // Deserialize back to mojom
  auto deserialized_event = DeserializeWebSourcesEvent(proto_event);

  // Verify deserialized data matches original
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest, SerializeWebSourcesEvent_InvalidUrls) {
  // Create mojom WebSourcesEvent with invalid URLs
  auto mojom_event = mojom::WebSourcesEvent::New();

  auto valid_source = mojom::WebSource::New(
      "Valid Source", GURL("https://example.com/valid"),
      GURL("https://example.com/valid.ico"), std::nullopt, std::nullopt);
  mojom_event->sources.push_back(std::move(valid_source));

  auto invalid_url_source = mojom::WebSource::New(
      "Invalid URL Source", GURL("invalid-url"),
      GURL("https://example.com/valid.ico"), std::nullopt, std::nullopt);
  mojom_event->sources.push_back(std::move(invalid_url_source));

  auto invalid_favicon_source = mojom::WebSource::New(
      "Invalid Favicon Source", GURL("https://example.com/valid"),
      GURL("invalid-favicon-url"), std::nullopt, std::nullopt);
  mojom_event->sources.push_back(std::move(invalid_favicon_source));

  // Serialize to proto
  store::WebSourcesEventProto proto_event;
  SerializeWebSourcesEvent(mojom_event, &proto_event);

  // Only the valid source should be serialized
  EXPECT_EQ(proto_event.sources_size(), 1);
  EXPECT_EQ(proto_event.sources(0).title(), "Valid Source");
}

TEST(ProtoConversionTest, DeserializeWebSourcesEvent_InvalidUrls) {
  // Create proto WebSourcesEvent with invalid URLs
  store::WebSourcesEventProto proto_event;

  auto* valid_source = proto_event.add_sources();
  valid_source->set_title("Valid Source");
  valid_source->set_url("https://example.com/valid");
  valid_source->set_favicon_url("https://example.com/valid.ico");

  auto* invalid_url_source = proto_event.add_sources();
  invalid_url_source->set_title("Invalid URL Source");
  invalid_url_source->set_url("invalid-url");
  invalid_url_source->set_favicon_url("https://example.com/valid.ico");

  auto* invalid_favicon_source = proto_event.add_sources();
  invalid_favicon_source->set_title("Invalid Favicon Source");
  invalid_favicon_source->set_url("https://example.com/valid");
  invalid_favicon_source->set_favicon_url("invalid-favicon-url");

  // Deserialize to mojom
  auto mojom_event = DeserializeWebSourcesEvent(proto_event);

  // Only the valid source should be deserialized
  EXPECT_EQ(mojom_event->sources.size(), 1u);
  EXPECT_EQ(mojom_event->sources[0]->title, "Valid Source");
}

TEST(ProtoConversionTest, SerializeDeserializeWebSourcesEvent_EmptySources) {
  // Create empty mojom WebSourcesEvent
  auto mojom_event = mojom::WebSourcesEvent::New();

  // Serialize to proto
  store::WebSourcesEventProto proto_event;
  SerializeWebSourcesEvent(mojom_event, &proto_event);

  // Verify empty proto
  EXPECT_EQ(proto_event.sources_size(), 0);
  EXPECT_EQ(proto_event.rich_results_size(), 0);

  // Deserialize back to mojom
  auto deserialized_event = DeserializeWebSourcesEvent(proto_event);

  // Verify empty mojom
  EXPECT_EQ(deserialized_event->sources.size(), 0u);
  EXPECT_EQ(deserialized_event->rich_results.size(), 0u);
}

TEST(ProtoConversionTest, SerializeDeserializeWebSourcesEvent_RichResults) {
  // Test with complex nested JSON in rich_results
  auto mojom_event = mojom::WebSourcesEvent::New();

  auto source = mojom::WebSource::New("Example", GURL("https://example.com"),
                                      GURL("https://example.com/favicon.ico"),
                                      std::nullopt, std::nullopt);
  mojom_event->sources.push_back(std::move(source));

  // Add complex nested JSON structures
  mojom_event->rich_results.push_back(R"({
    "type":"knowledge_graph",
    "entity":"Python Programming",
    "properties":{
      "category":"Programming Language",
      "year":1991,
      "creator":"Guido van Rossum"
    },
    "links":[
      {"title":"Official Site","url":"https://python.org"},
      {"title":"Documentation","url":"https://docs.python.org"}
    ]
  })");

  // Serialize to proto
  store::WebSourcesEventProto proto_event;
  SerializeWebSourcesEvent(mojom_event, &proto_event);

  // Deserialize back to mojom
  auto deserialized_event = DeserializeWebSourcesEvent(proto_event);

  // Verify deserialized data matches original
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest,
     SerializeDeserializeWebSourcesEvent_EmptyRichResultStrings) {
  // Test with empty strings in rich_results
  auto mojom_event = mojom::WebSourcesEvent::New();

  auto source = mojom::WebSource::New("Example", GURL("https://example.com"),
                                      GURL("https://example.com/favicon.ico"),
                                      std::nullopt, std::nullopt);
  mojom_event->sources.push_back(std::move(source));

  // Add valid and empty rich_results
  mojom_event->rich_results.push_back(R"({"type":"valid_data"})");
  mojom_event->rich_results.push_back("");  // empty string
  mojom_event->rich_results.push_back(R"({"type":"another_valid"})");

  // Serialize to proto
  store::WebSourcesEventProto proto_event;
  SerializeWebSourcesEvent(mojom_event, &proto_event);

  // Deserialize back to mojom
  auto deserialized_event = DeserializeWebSourcesEvent(proto_event);

  // Verify deserialized data matches original
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

// Tests for ToolUseEvent conversion functions

TEST(ProtoConversionTest, SerializeDeserializeToolUseEvent_ValidData) {
  // Create mojom ToolUseEvent
  auto mojom_event = mojom::ToolUseEvent::New(
      "test_tool", "tool_id_123", "anything for arguments_json",
      std::vector<mojom::ContentBlockPtr>(), std::nullopt, nullptr, false);

  // mixed content blocks
  auto text_block = mojom::TextContentBlock::New();
  text_block->text = "This is a text response";
  mojom_event->output->push_back(
      mojom::ContentBlock::NewTextContentBlock(std::move(text_block)));

  auto image_block = mojom::ImageContentBlock::New();
  image_block->image_url = GURL("https://example.com/image.png");
  mojom_event->output->push_back(
      mojom::ContentBlock::NewImageContentBlock(std::move(image_block)));

  // Serialize to proto
  store::ToolUseEventProto proto_event;
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_TRUE(success);
  EXPECT_EQ(proto_event.tool_name(), "test_tool");
  EXPECT_EQ(proto_event.id(), "tool_id_123");
  EXPECT_EQ(proto_event.arguments_json(), "anything for arguments_json");
  EXPECT_EQ(proto_event.output_size(), 2);

  // Check first output block (text)
  EXPECT_TRUE(proto_event.output(0).has_text_content_block());
  EXPECT_EQ(proto_event.output(0).text_content_block().text(),
            "This is a text response");

  // Check second output block (image)
  EXPECT_TRUE(proto_event.output(1).has_image_content_block());
  EXPECT_EQ(proto_event.output(1).image_content_block().image_url(),
            "https://example.com/image.png");

  // Deserialize back to mojom
  auto deserialized_event = DeserializeToolUseEvent(proto_event);

  // Verify deserialized data
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest, SerializeDeserializeToolUseEvent_WithArtifacts) {
  const std::string content_json = R"({"data": [1, 2, 3]})";
  // Create mojom ToolUseEvent with artifacts
  auto mojom_event = mojom::ToolUseEvent::New(
      "test_tool", "tool_id_123", "{}", std::vector<mojom::ContentBlockPtr>(),
      std::vector<mojom::ToolArtifactPtr>(), nullptr, false);

  auto artifact = mojom::ToolArtifact::New();
  artifact->id = "test-artifact-id-123";
  artifact->type = mojom::kLineChartArtifactType;
  artifact->content_json = content_json;
  mojom_event->artifacts->push_back(std::move(artifact));

  auto text_block = mojom::TextContentBlock::New();
  text_block->text = "This is a text response";
  mojom_event->output->push_back(
      mojom::ContentBlock::NewTextContentBlock(std::move(text_block)));

  // Serialize to proto
  store::ToolUseEventProto proto_event;
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_TRUE(success);
  EXPECT_EQ(proto_event.artifacts_size(), 1);
  EXPECT_EQ(proto_event.artifacts(0).id(), "test-artifact-id-123");
  EXPECT_EQ(proto_event.artifacts(0).type(), mojom::kLineChartArtifactType);
  EXPECT_EQ(proto_event.artifacts(0).content_json(), content_json);
  EXPECT_EQ(proto_event.output_size(), 1);

  // Deserialize back to mojom
  auto deserialized_event = DeserializeToolUseEvent(proto_event);

  // Verify deserialized data
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest, SerializeDeserializeToolUseEvent_NoOutput) {
  // Create mojom ToolUseEvent without output
  auto mojom_event =
      mojom::ToolUseEvent::New("test_tool", "tool_id_123", "{}", std::nullopt,
                               std::nullopt, nullptr, false);

  // Serialize to proto
  store::ToolUseEventProto proto_event;
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_TRUE(success);
  EXPECT_EQ(proto_event.output_size(), 0);

  // Deserialize back to mojom
  auto deserialized_event = DeserializeToolUseEvent(proto_event);

  EXPECT_FALSE(deserialized_event->output.has_value());
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest, SerializeToolUseEvent_InvalidId) {
  store::ToolUseEventProto proto_event;

  auto mojom_event = mojom::ToolUseEvent::New(
      "test_tool", "", "{}", std::nullopt, std::nullopt, nullptr, false);
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_FALSE(success);
  // Did not do any serialization
  EXPECT_EQ(proto_event.tool_name(), "");
  EXPECT_EQ(proto_event.id(), "");
}

TEST(ProtoConversionTest, SerializeToolUseEvent_InvalidToolName) {
  store::ToolUseEventProto proto_event;

  auto mojom_event = mojom::ToolUseEvent::New(
      "", "tool_id_123", "{}", std::nullopt, std::nullopt, nullptr, false);
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_FALSE(success);
  // Did not do any serialization
  EXPECT_EQ(proto_event.tool_name(), "");
  EXPECT_EQ(proto_event.id(), "");
}

TEST(ProtoConversionTest, DeserializeToolUseEvent_InvalidContentBlocks) {
  // Create proto ToolUseEvent with invalid content blocks
  store::ToolUseEventProto proto_event;
  proto_event.set_tool_name("test_tool");
  proto_event.set_id("tool_id_123");

  // Add a valid text block
  auto* valid_block = proto_event.add_output();
  valid_block->mutable_text_content_block()->set_text("Valid text");

  // Add an invalid block (CONTENT_NOT_SET)
  proto_event.add_output();
  // Don't set any content - this will be CONTENT_NOT_SET

  // Deserialize to mojom
  auto mojom_event = DeserializeToolUseEvent(proto_event);

  // Only the valid block should be deserialized
  ASSERT_TRUE(mojom_event->output.has_value());
  EXPECT_EQ(mojom_event->output->size(), 1u);
  EXPECT_TRUE(mojom_event->output->at(0)->is_text_content_block());
  EXPECT_EQ(mojom_event->output->at(0)->get_text_content_block()->text,
            "Valid text");
}

TEST(ProtoConversionTest, SerializeDeserializeToolUseEvent_IsServerResult) {
  // Test is_server_result = true
  auto mojom_event_server = mojom::ToolUseEvent::New(
      "brave_web_search", "tooluse_server", R"({"query": "test"})",
      std::nullopt, std::nullopt, nullptr, true);

  store::ToolUseEventProto proto_event;
  EXPECT_TRUE(SerializeToolUseEvent(mojom_event_server, &proto_event));
  EXPECT_TRUE(proto_event.is_server_result());

  auto deserialized = DeserializeToolUseEvent(proto_event);
  EXPECT_TRUE(deserialized->is_server_result);

  // Test is_server_result = false (default)
  auto mojom_event_client =
      mojom::ToolUseEvent::New("client_tool", "tooluse_client", "{}",
                               std::nullopt, std::nullopt, nullptr, false);

  store::ToolUseEventProto proto_event2;
  EXPECT_TRUE(SerializeToolUseEvent(mojom_event_client, &proto_event2));
  EXPECT_FALSE(proto_event2.is_server_result());

  auto deserialized2 = DeserializeToolUseEvent(proto_event2);
  EXPECT_FALSE(deserialized2->is_server_result);
}

TEST(ProtoConversionTest,
     SerializeDeserializeToolUseEvent_WithWebSourcesContentBlock) {
  // Create mojom ToolUseEvent with WebSourcesContentBlock output
  auto mojom_event = mojom::ToolUseEvent::New(
      "brave_web_search", "tooluse_search123",
      R"({"query": "weather", "country": "US"})",
      std::vector<mojom::ContentBlockPtr>(), std::nullopt, nullptr, false);

  // Create WebSourcesContentBlock
  auto web_sources_block = mojom::WebSourcesContentBlock::New();
  web_sources_block->queries = std::vector<std::string>{"weather in San Jose"};

  auto source1 = mojom::WebSource::New(
      "Weather.com", GURL("https://weather.com/sanjose"),
      GURL("https://weather.com/favicon.ico"), std::nullopt, std::nullopt);
  web_sources_block->sources.push_back(std::move(source1));

  auto source2 = mojom::WebSource::New(
      "AccuWeather", GURL("https://accuweather.com/sanjose"),
      GURL("https://accuweather.com/favicon.ico"), std::nullopt, std::nullopt);
  web_sources_block->sources.push_back(std::move(source2));

  web_sources_block->rich_results.push_back(
      R"({"type":"weather","temp":"72F"})");
  web_sources_block->rich_results.push_back(
      R"({"type":"forecast","days":["Mon","Tue"]})");

  mojom_event->output->push_back(mojom::ContentBlock::NewWebSourcesContentBlock(
      std::move(web_sources_block)));

  // Serialize to proto
  store::ToolUseEventProto proto_event;
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_TRUE(success);
  EXPECT_EQ(proto_event.tool_name(), "brave_web_search");
  EXPECT_EQ(proto_event.id(), "tooluse_search123");
  ASSERT_EQ(proto_event.output_size(), 1);

  // Verify WebSourcesContentBlock serialization
  ASSERT_TRUE(proto_event.output(0).has_web_sources_content_block());
  const auto& proto_sources = proto_event.output(0).web_sources_content_block();
  ASSERT_EQ(proto_sources.queries_size(), 1);
  EXPECT_EQ(proto_sources.queries(0), "weather in San Jose");
  ASSERT_EQ(proto_sources.sources_size(), 2);
  EXPECT_EQ(proto_sources.sources(0).title(), "Weather.com");
  EXPECT_EQ(proto_sources.sources(0).url(), "https://weather.com/sanjose");
  EXPECT_EQ(proto_sources.sources(0).favicon_url(),
            "https://weather.com/favicon.ico");
  EXPECT_EQ(proto_sources.sources(1).title(), "AccuWeather");
  ASSERT_EQ(proto_sources.rich_results_size(), 2);
  EXPECT_EQ(proto_sources.rich_results(0),
            R"({"type":"weather","temp":"72F"})");
  EXPECT_EQ(proto_sources.rich_results(1),
            R"({"type":"forecast","days":["Mon","Tue"]})");

  // Deserialize back to mojom
  auto deserialized_event = DeserializeToolUseEvent(proto_event);

  // Verify deserialized data matches original
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest,
     SerializeDeserializeToolUseEvent_WebSourcesContentBlockWithoutQuery) {
  // Create mojom ToolUseEvent with WebSourcesContentBlock without query
  auto mojom_event = mojom::ToolUseEvent::New(
      "brave_web_search", "tooluse_456", R"({"query": "test"})",
      std::vector<mojom::ContentBlockPtr>(), std::nullopt, nullptr, false);

  auto web_sources_block = mojom::WebSourcesContentBlock::New();
  // No query set

  auto source = mojom::WebSource::New("Example", GURL("https://example.com"),
                                      GURL("https://example.com/favicon.ico"),
                                      std::nullopt, std::nullopt);
  web_sources_block->sources.push_back(std::move(source));

  mojom_event->output->push_back(mojom::ContentBlock::NewWebSourcesContentBlock(
      std::move(web_sources_block)));

  // Serialize to proto
  store::ToolUseEventProto proto_event;
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_TRUE(success);
  ASSERT_EQ(proto_event.output_size(), 1);
  ASSERT_TRUE(proto_event.output(0).has_web_sources_content_block());
  EXPECT_EQ(proto_event.output(0).web_sources_content_block().queries_size(),
            0);

  // Deserialize back to mojom
  auto deserialized_event = DeserializeToolUseEvent(proto_event);

  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest, SerializeDeserializeToolUseEvent_MixedContentBlocks) {
  // Test with mixed content blocks including WebSourcesContentBlock
  auto mojom_event = mojom::ToolUseEvent::New(
      "multi_tool", "tooluse_mixed", "{}",
      std::vector<mojom::ContentBlockPtr>(), std::nullopt, nullptr, false);

  // Add text block
  auto text_block = mojom::TextContentBlock::New();
  text_block->text = "Search results:";
  mojom_event->output->push_back(
      mojom::ContentBlock::NewTextContentBlock(std::move(text_block)));

  // Add WebSourcesContentBlock
  auto web_sources_block = mojom::WebSourcesContentBlock::New();
  web_sources_block->queries = std::vector<std::string>{"test query"};
  auto source = mojom::WebSource::New("Test Site", GURL("https://test.com"),
                                      GURL("https://test.com/favicon.ico"),
                                      std::nullopt, std::nullopt);
  web_sources_block->sources.push_back(std::move(source));
  mojom_event->output->push_back(mojom::ContentBlock::NewWebSourcesContentBlock(
      std::move(web_sources_block)));

  // Add image block
  auto image_block = mojom::ImageContentBlock::New();
  image_block->image_url = GURL("https://test.com/image.png");
  mojom_event->output->push_back(
      mojom::ContentBlock::NewImageContentBlock(std::move(image_block)));

  // Serialize to proto
  store::ToolUseEventProto proto_event;
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_TRUE(success);
  ASSERT_EQ(proto_event.output_size(), 3);
  EXPECT_TRUE(proto_event.output(0).has_text_content_block());
  EXPECT_TRUE(proto_event.output(1).has_web_sources_content_block());
  EXPECT_TRUE(proto_event.output(2).has_image_content_block());

  // Deserialize back to mojom
  auto deserialized_event = DeserializeToolUseEvent(proto_event);

  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest,
     SerializeToolUseEvent_WebSourcesContentBlockInvalidUrls) {
  // Test that invalid URLs in WebSourcesContentBlock are skipped
  auto mojom_event = mojom::ToolUseEvent::New(
      "brave_web_search", "tooluse_789", "{}",
      std::vector<mojom::ContentBlockPtr>(), std::nullopt, nullptr, false);

  auto web_sources_block = mojom::WebSourcesContentBlock::New();

  // Valid source
  auto valid_source = mojom::WebSource::New(
      "Valid", GURL("https://valid.com"), GURL("https://valid.com/favicon.ico"),
      std::nullopt, std::nullopt);
  web_sources_block->sources.push_back(std::move(valid_source));

  // Invalid URL source
  auto invalid_url_source = mojom::WebSource::New(
      "Invalid URL", GURL("invalid-url"), GURL("https://valid.com/favicon.ico"),
      std::nullopt, std::nullopt);
  web_sources_block->sources.push_back(std::move(invalid_url_source));

  // Invalid favicon source
  auto invalid_favicon_source = mojom::WebSource::New(
      "Invalid Favicon", GURL("https://valid.com"), GURL("invalid-favicon"),
      std::nullopt, std::nullopt);
  web_sources_block->sources.push_back(std::move(invalid_favicon_source));

  mojom_event->output->push_back(mojom::ContentBlock::NewWebSourcesContentBlock(
      std::move(web_sources_block)));

  // Serialize to proto
  store::ToolUseEventProto proto_event;
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_TRUE(success);
  ASSERT_EQ(proto_event.output_size(), 1);
  ASSERT_TRUE(proto_event.output(0).has_web_sources_content_block());
  // Only the valid source should be serialized
  EXPECT_EQ(proto_event.output(0).web_sources_content_block().sources_size(),
            1);
  EXPECT_EQ(
      proto_event.output(0).web_sources_content_block().sources(0).title(),
      "Valid");
  EXPECT_EQ(proto_event.output(0).web_sources_content_block().sources(0).url(),
            "https://valid.com/");
  EXPECT_EQ(proto_event.output(0)
                .web_sources_content_block()
                .sources(0)
                .favicon_url(),
            "https://valid.com/favicon.ico");
}

TEST(ProtoConversionTest,
     DeserializeToolUseEvent_WebSourcesContentBlockInvalidUrls) {
  // Test that invalid URLs in WebSourcesContentBlock are skipped during
  // deserialization
  store::ToolUseEventProto proto_event;
  proto_event.set_tool_name("brave_web_search");
  proto_event.set_id("tooluse_789");

  auto* proto_block = proto_event.add_output();
  auto* proto_sources = proto_block->mutable_web_sources_content_block();
  proto_sources->add_queries("test query");

  // Valid source
  auto* valid_source = proto_sources->add_sources();
  valid_source->set_title("Valid");
  valid_source->set_url("https://valid.com");
  valid_source->set_favicon_url("https://valid.com/favicon.ico");

  // Invalid URL source
  auto* invalid_url_source = proto_sources->add_sources();
  invalid_url_source->set_title("Invalid URL");
  invalid_url_source->set_url("invalid-url");
  invalid_url_source->set_favicon_url("https://valid.com/favicon.ico");

  // Invalid favicon URL source
  auto* invalid_favicon_source = proto_sources->add_sources();
  invalid_favicon_source->set_title("Invalid Favicon");
  invalid_favicon_source->set_url("https://valid.com");
  invalid_favicon_source->set_favicon_url("invalid-favicon");

  // Deserialize to mojom
  auto mojom_event = DeserializeToolUseEvent(proto_event);

  // Only the valid source should be deserialized
  ASSERT_TRUE(mojom_event->output.has_value());
  ASSERT_EQ(mojom_event->output->size(), 1u);
  ASSERT_TRUE(mojom_event->output->at(0)->is_web_sources_content_block());
  const auto& web_sources =
      mojom_event->output->at(0)->get_web_sources_content_block();
  EXPECT_THAT(web_sources->queries, testing::ElementsAre("test query"));
  ASSERT_EQ(web_sources->sources.size(), 1u);
  EXPECT_EQ(web_sources->sources[0]->title, "Valid");
  EXPECT_EQ(web_sources->sources[0]->url.spec(), "https://valid.com/");
  EXPECT_EQ(web_sources->sources[0]->favicon_url.spec(),
            "https://valid.com/favicon.ico");
}

TEST(ProtoConversionTest,
     SerializeDeserializeToolUseEvent_WebSourcesWithPageContentAndSnippets) {
  // Create mojom ToolUseEvent with WebSourcesContentBlock containing
  // page_content and extra_snippets in various combinations
  auto mojom_event = mojom::ToolUseEvent::New(
      "brave_web_search", "tooluse_page_content",
      R"({"query": "test page content"})",
      std::vector<mojom::ContentBlockPtr>(), std::nullopt, nullptr, false);

  auto web_sources_block = mojom::WebSourcesContentBlock::New();
  web_sources_block->queries = std::vector<std::string>{"test page content"};

  // Source 1: both page_content and extra_snippets set
  auto source1 = mojom::WebSource::New(
      "Source One", GURL("https://source1.com"),
      GURL("https://source1.com/favicon.ico"),
      std::make_optional<std::string>("Full page text content..."),
      std::make_optional<std::vector<std::string>>(
          {"Relevant snippet 1", "Relevant snippet 2", "Relevant snippet 3"}));
  web_sources_block->sources.push_back(std::move(source1));

  // Source 2: only page_content set, extra_snippets nullopt
  auto source2 = mojom::WebSource::New(
      "Source Two", GURL("https://source2.com"),
      GURL("https://source2.com/favicon.ico"),
      std::make_optional<std::string>("Another page content"), std::nullopt);
  web_sources_block->sources.push_back(std::move(source2));

  // Source 3: page_content nullopt, only extra_snippets set
  auto source3 = mojom::WebSource::New(
      "Source Three", GURL("https://source3.com"),
      GURL("https://source3.com/favicon.ico"), std::nullopt,
      std::make_optional<std::vector<std::string>>({"Single snippet"}));
  web_sources_block->sources.push_back(std::move(source3));

  mojom_event->output->push_back(mojom::ContentBlock::NewWebSourcesContentBlock(
      std::move(web_sources_block)));

  // Serialize to proto
  store::ToolUseEventProto proto_event;
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_TRUE(success);
  ASSERT_EQ(proto_event.output_size(), 1);
  ASSERT_TRUE(proto_event.output(0).has_web_sources_content_block());
  const auto& proto_sources = proto_event.output(0).web_sources_content_block();
  ASSERT_EQ(proto_sources.sources_size(), 3);

  // Verify source 1: both fields set
  EXPECT_TRUE(proto_sources.sources(0).has_page_content());
  EXPECT_EQ(proto_sources.sources(0).page_content(),
            "Full page text content...");
  ASSERT_EQ(proto_sources.sources(0).extra_snippets_size(), 3);
  EXPECT_EQ(proto_sources.sources(0).extra_snippets(0), "Relevant snippet 1");
  EXPECT_EQ(proto_sources.sources(0).extra_snippets(1), "Relevant snippet 2");
  EXPECT_EQ(proto_sources.sources(0).extra_snippets(2), "Relevant snippet 3");

  // Verify source 2: only page_content
  EXPECT_TRUE(proto_sources.sources(1).has_page_content());
  EXPECT_EQ(proto_sources.sources(1).page_content(), "Another page content");
  EXPECT_EQ(proto_sources.sources(1).extra_snippets_size(), 0);

  // Verify source 3: only extra_snippets
  EXPECT_FALSE(proto_sources.sources(2).has_page_content());
  ASSERT_EQ(proto_sources.sources(2).extra_snippets_size(), 1);
  EXPECT_EQ(proto_sources.sources(2).extra_snippets(0), "Single snippet");

  // Deserialize back to mojom and verify round-trip
  auto deserialized_event = DeserializeToolUseEvent(proto_event);

  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

// Tests for InlineSearchEvent conversion functions

TEST(ProtoConversionTest, SerializeDeserializeInlineSearchEvent_ValidData) {
  auto mojom_event = mojom::InlineSearchEvent::New(
      "brave search", R"([{"title":"Result 1","url":"https://example.com"}])");

  store::InlineSearchEventProto proto_event;
  SerializeInlineSearchEvent(mojom_event, &proto_event);

  EXPECT_EQ(proto_event.query(), "brave search");
  EXPECT_EQ(proto_event.results_json(),
            R"([{"title":"Result 1","url":"https://example.com"}])");

  auto deserialized_event = DeserializeInlineSearchEvent(proto_event);
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest,
     SerializeDeserializeInlineSearchEvent_EmptyResultsJson) {
  auto mojom_event = mojom::InlineSearchEvent::New("test query", "");

  store::InlineSearchEventProto proto_event;
  SerializeInlineSearchEvent(mojom_event, &proto_event);

  EXPECT_EQ(proto_event.query(), "test query");
  EXPECT_EQ(proto_event.results_json(), "");

  auto deserialized_event = DeserializeInlineSearchEvent(proto_event);
  EXPECT_MOJOM_EQ(*deserialized_event, *mojom_event);
}

TEST(ProtoConversionTest, SerializeDeserializeSkillEntry) {
  // Create mojom SkillEntry
  auto mojom_entry =
      mojom::SkillEntry::New("summarize", "Please summarize this content");

  // Serialize to proto
  store::SkillEntryProto proto_entry;
  SerializeSkillEntry(mojom_entry, &proto_entry);

  // Verify proto data
  EXPECT_EQ(proto_entry.shortcut(), "summarize");
  EXPECT_EQ(proto_entry.prompt(), "Please summarize this content");

  // Deserialize back to mojom
  auto deserialized_entry = DeserializeSkillEntry(proto_entry);

  // Verify deserialized data matches original
  EXPECT_EQ(deserialized_entry->shortcut, mojom_entry->shortcut);
  EXPECT_EQ(deserialized_entry->prompt, mojom_entry->prompt);
}

}  // namespace ai_chat
