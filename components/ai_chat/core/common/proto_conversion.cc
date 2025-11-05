// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/proto_conversion.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/proto/store.pb.h"
#include "url/gurl.h"

namespace ai_chat {

mojom::WebSourcesEventPtr DeserializeWebSourcesEvent(
    const store::WebSourcesEventProto& proto_event) {
  auto mojom_event = mojom::WebSourcesEvent::New();
  mojom_event->sources.reserve(proto_event.sources_size());

  for (const auto& proto_source : proto_event.sources()) {
    auto mojom_source = mojom::WebSource::New();
    mojom_source->title = proto_source.title();
    mojom_source->url = GURL(proto_source.url());
    if (!mojom_source->url.is_valid()) {
      DLOG(ERROR) << "Invalid WebSourcesEvent found in database with url: "
                  << proto_source.url();
      continue;
    }
    mojom_source->favicon_url = GURL(proto_source.favicon_url());
    if (!mojom_source->favicon_url.is_valid()) {
      DLOG(ERROR)
          << "Invalid WebSourcesEvent found in database with favicon url: "
          << proto_source.favicon_url();
      continue;
    }
    mojom_event->sources.push_back(std::move(mojom_source));
  }
  return mojom_event;
}

void SerializeWebSourcesEvent(const mojom::WebSourcesEventPtr& mojom_event,
                              store::WebSourcesEventProto* proto_event) {
  CHECK(mojom_event);
  CHECK(proto_event);

  proto_event->clear_sources();

  for (const auto& mojom_source : mojom_event->sources) {
    if (!mojom_source->url.is_valid() ||
        !mojom_source->favicon_url.is_valid()) {
      DLOG(ERROR) << "Invalid WebSourcesEvent found for persistence, with url: "
                  << mojom_source->url.spec()
                  << " and favicon url: " << mojom_source->favicon_url.spec();
      continue;
    }
    store::WebSourceProto* proto_source = proto_event->add_sources();
    proto_source->set_title(mojom_source->title);
    proto_source->set_url(mojom_source->url.spec());
    proto_source->set_favicon_url(mojom_source->favicon_url.spec());
  }
}

mojom::ToolUseEventPtr DeserializeToolUseEvent(
    const store::ToolUseEventProto& proto_event) {
  auto mojom_event =
      mojom::ToolUseEvent::New(proto_event.tool_name(), proto_event.id(),
                               proto_event.arguments_json(), std::nullopt);

  // Convert output ContentBlocks
  if (proto_event.output_size() > 0) {
    mojom_event->output = std::vector<mojom::ContentBlockPtr>();
    mojom_event->output->reserve(
        static_cast<size_t>(proto_event.output_size()));

    for (const auto& proto_block : proto_event.output()) {
      switch (proto_block.content_case()) {
        case store::ContentBlockProto::kImageContentBlock: {
          auto image_block = mojom::ImageContentBlock::New();
          image_block->image_url =
              GURL(proto_block.image_content_block().image_url());
          mojom_event->output->push_back(
              mojom::ContentBlock::NewImageContentBlock(
                  std::move(image_block)));
          break;
        }
        case store::ContentBlockProto::kTextContentBlock: {
          auto text_block = mojom::TextContentBlock::New();
          text_block->text = proto_block.text_content_block().text();
          mojom_event->output->push_back(
              mojom::ContentBlock::NewTextContentBlock(std::move(text_block)));
          break;
        }
        case store::ContentBlockProto::CONTENT_NOT_SET:
          // Skip invalid blocks
          break;
      }
    }
  }

  return mojom_event;
}

bool SerializeToolUseEvent(const mojom::ToolUseEventPtr& mojom_event,
                           store::ToolUseEventProto* proto_event) {
  CHECK(mojom_event);
  CHECK(proto_event);

  // Since this is only used for storage, we enforce required fields for tool
  // use and identification.
  if (mojom_event->id.empty()) {
    DLOG(ERROR) << "Invalid ToolUseEvent found for persistence, with empty id";
    return false;
  }

  if (mojom_event->tool_name.empty()) {
    DLOG(ERROR)
        << "Invalid ToolUseEvent found for persistence, with empty tool name";
    return false;
  }

  proto_event->set_tool_name(mojom_event->tool_name);
  proto_event->set_id(mojom_event->id);
  proto_event->set_arguments_json(mojom_event->arguments_json);

  // Convert output ContentBlocks
  proto_event->clear_output();
  if (mojom_event->output) {
    for (const auto& mojom_block : mojom_event->output.value()) {
      store::ContentBlockProto* proto_block = proto_event->add_output();

      switch (mojom_block->which()) {
        case mojom::ContentBlock::Tag::kImageContentBlock: {
          auto* proto_image = proto_block->mutable_image_content_block();
          proto_image->set_image_url(
              mojom_block->get_image_content_block()->image_url.spec());
          break;
        }
        case mojom::ContentBlock::Tag::kTextContentBlock: {
          auto* proto_text = proto_block->mutable_text_content_block();
          proto_text->set_text(mojom_block->get_text_content_block()->text);
          break;
        }
      }
    }
  }

  return true;
}

mojom::SkillEntryPtr DeserializeSkillEntry(
    const store::SkillEntryProto& proto_entry) {
  return mojom::SkillEntry::New(proto_entry.shortcut(), proto_entry.prompt());
}

void SerializeSkillEntry(const mojom::SkillEntryPtr& mojom_entry,
                         store::SkillEntryProto* proto_entry) {
  CHECK(mojom_entry);
  CHECK(proto_entry);

  proto_entry->set_shortcut(mojom_entry->shortcut);
  proto_entry->set_prompt(mojom_entry->prompt);
}

}  // namespace ai_chat
