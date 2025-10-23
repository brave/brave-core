// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/proto_conversion.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

// Tests for WebSourcesEvent conversion functions

TEST(ProtoConversionTest, SerializeDeserializeWebSourcesEvent_ValidData) {
  // Create mojom WebSourcesEvent
  auto mojom_event = mojom::WebSourcesEvent::New();

  auto source1 =
      mojom::WebSource::New("Test Title 1", GURL("https://example.com/page1"),
                            GURL("https://example.com/favicon1.ico"));
  mojom_event->sources.push_back(std::move(source1));

  auto source2 =
      mojom::WebSource::New("Test Title 2", GURL("https://example.com/page2"),
                            GURL("https://example.com/favicon2.ico"));
  mojom_event->sources.push_back(std::move(source2));

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

  // Deserialize back to mojom
  auto deserialized_event = DeserializeWebSourcesEvent(proto_event);

  // Verify deserialized data
  ASSERT_EQ(deserialized_event->sources.size(), 2u);
  EXPECT_MOJOM_EQ(*deserialized_event->sources[0], *mojom_event->sources[0]);
  EXPECT_MOJOM_EQ(*deserialized_event->sources[1], *mojom_event->sources[1]);
}

TEST(ProtoConversionTest, SerializeWebSourcesEvent_InvalidUrls) {
  // Create mojom WebSourcesEvent with invalid URLs
  auto mojom_event = mojom::WebSourcesEvent::New();

  auto valid_source =
      mojom::WebSource::New("Valid Source", GURL("https://example.com/valid"),
                            GURL("https://example.com/valid.ico"));
  mojom_event->sources.push_back(std::move(valid_source));

  auto invalid_url_source =
      mojom::WebSource::New("Invalid URL Source", GURL("invalid-url"),
                            GURL("https://example.com/valid.ico"));
  mojom_event->sources.push_back(std::move(invalid_url_source));

  auto invalid_favicon_source = mojom::WebSource::New(
      "Invalid Favicon Source", GURL("https://example.com/valid"),
      GURL("invalid-favicon-url"));
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

  // Deserialize back to mojom
  auto deserialized_event = DeserializeWebSourcesEvent(proto_event);

  // Verify empty mojom
  EXPECT_EQ(deserialized_event->sources.size(), 0u);
}

// Tests for ToolUseEvent conversion functions

TEST(ProtoConversionTest, SerializeDeserializeToolUseEvent_ValidData) {
  // Create mojom ToolUseEvent
  auto mojom_event = mojom::ToolUseEvent::New(
      "test_tool", "tool_id_123", "anything for arguments_json",
      std::vector<mojom::ContentBlockPtr>());

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

TEST(ProtoConversionTest, SerializeDeserializeToolUseEvent_NoOutput) {
  // Create mojom ToolUseEvent without output
  auto mojom_event =
      mojom::ToolUseEvent::New("test_tool", "tool_id_123", "{}", std::nullopt);

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

  auto mojom_event =
      mojom::ToolUseEvent::New("test_tool", "", "{}", std::nullopt);
  bool success = SerializeToolUseEvent(mojom_event, &proto_event);

  EXPECT_FALSE(success);
  // Did not do any serialization
  EXPECT_EQ(proto_event.tool_name(), "");
  EXPECT_EQ(proto_event.id(), "");
}

TEST(ProtoConversionTest, SerializeToolUseEvent_InvalidToolName) {
  store::ToolUseEventProto proto_event;

  auto mojom_event =
      mojom::ToolUseEvent::New("", "tool_id_123", "{}", std::nullopt);
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
