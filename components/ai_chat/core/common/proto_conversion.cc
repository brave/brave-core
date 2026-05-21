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
      DVLOG(1) << "Invalid WebSourcesEvent found in database with url: "
               << proto_source.url();
      continue;
    }
    mojom_source->favicon_url = GURL(proto_source.favicon_url());
    if (!mojom_source->favicon_url.is_valid()) {
      DVLOG(1) << "Invalid WebSourcesEvent found in database with favicon url: "
               << proto_source.favicon_url();
      continue;
    }
    mojom_event->sources.push_back(std::move(mojom_source));
  }

  mojom_event->rich_results.reserve(proto_event.rich_results_size());
  for (const auto& rich_result : proto_event.rich_results()) {
    mojom_event->rich_results.push_back(rich_result);
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
      DVLOG(1) << "Invalid WebSourcesEvent found for persistence, with url: "
               << mojom_source->url.spec()
               << " and favicon url: " << mojom_source->favicon_url.spec();
      continue;
    }
    store::WebSourceProto* proto_source = proto_event->add_sources();
    proto_source->set_title(mojom_source->title);
    proto_source->set_url(mojom_source->url.spec());
    proto_source->set_favicon_url(mojom_source->favicon_url.spec());
  }

  proto_event->clear_rich_results();
  for (const auto& rich_result : mojom_event->rich_results) {
    proto_event->add_rich_results(rich_result);
  }
}

mojom::InlineSearchEventPtr DeserializeInlineSearchEvent(
    const store::InlineSearchEventProto& proto_event) {
  return mojom::InlineSearchEvent::New(proto_event.query(),
                                       proto_event.results_json());
}

void SerializeInlineSearchEvent(const mojom::InlineSearchEventPtr& mojom_event,
                                store::InlineSearchEventProto* proto_event) {
  CHECK(mojom_event);
  CHECK(proto_event);

  proto_event->set_query(mojom_event->query);
  proto_event->set_results_json(mojom_event->results_json);
}

mojom::ToolUseEventPtr DeserializeToolUseEvent(
    const store::ToolUseEventProto& proto_event) {
  auto mojom_event = mojom::ToolUseEvent::New(
      proto_event.tool_name(), proto_event.id(), proto_event.arguments_json(),
      std::nullopt, std::nullopt, nullptr, proto_event.is_server_result());

  // Convert artifacts
  if (proto_event.artifacts_size() > 0) {
    mojom_event->artifacts = std::vector<mojom::ToolArtifactPtr>();
    mojom_event->artifacts->reserve(
        static_cast<size_t>(proto_event.artifacts_size()));
    for (const auto& proto_artifact : proto_event.artifacts()) {
      auto mojom_artifact = mojom::ToolArtifact::New();
      if (proto_artifact.has_id()) {
        mojom_artifact->id = proto_artifact.id();
      }
      mojom_artifact->type = proto_artifact.type();
      mojom_artifact->content_json = proto_artifact.content_json();
      mojom_event->artifacts->push_back(std::move(mojom_artifact));
    }
  }

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
        case store::ContentBlockProto::kWebSourcesContentBlock: {
          const auto& proto_sources = proto_block.web_sources_content_block();
          auto mojom_sources = mojom::WebSourcesContentBlock::New();
          mojom_sources->sources.reserve(proto_sources.sources_size());
          for (const auto& proto_source : proto_sources.sources()) {
            auto mojom_source = mojom::WebSource::New();
            mojom_source->title = proto_source.title();
            mojom_source->url = GURL(proto_source.url());
            if (!mojom_source->url.is_valid()) {
              DVLOG(1) << "Invalid WebSourcesContentBlock url in database: "
                       << proto_source.url();
              continue;
            }
            mojom_source->favicon_url = GURL(proto_source.favicon_url());
            if (!mojom_source->favicon_url.is_valid()) {
              DVLOG(1)
                  << "Invalid WebSourcesContentBlock favicon url in database: "
                  << proto_source.favicon_url();
              continue;
            }
            if (proto_source.has_page_content()) {
              mojom_source->page_content = proto_source.page_content();
            }
            if (proto_source.extra_snippets_size() > 0) {
              mojom_source->extra_snippets.emplace(
                  proto_source.extra_snippets().begin(),
                  proto_source.extra_snippets().end());
            }
            mojom_sources->sources.push_back(std::move(mojom_source));
          }
          mojom_sources->queries.reserve(proto_sources.queries_size());
          for (const auto& query : proto_sources.queries()) {
            mojom_sources->queries.push_back(query);
          }
          mojom_sources->rich_results.reserve(
              proto_sources.rich_results_size());
          for (const auto& rich_result : proto_sources.rich_results()) {
            mojom_sources->rich_results.push_back(rich_result);
          }
          mojom_event->output->push_back(
              mojom::ContentBlock::NewWebSourcesContentBlock(
                  std::move(mojom_sources)));
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
  proto_event->set_is_server_result(mojom_event->is_server_result);

  // Convert artifacts
  proto_event->clear_artifacts();
  if (mojom_event->artifacts) {
    for (const auto& mojom_artifact : mojom_event->artifacts.value()) {
      auto* proto_artifact = proto_event->add_artifacts();
      if (mojom_artifact->id) {
        proto_artifact->set_id(*mojom_artifact->id);
      }
      proto_artifact->set_type(mojom_artifact->type);
      proto_artifact->set_content_json(mojom_artifact->content_json);
    }
  }

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
        case mojom::ContentBlock::Tag::kWebSourcesContentBlock: {
          const auto& mojom_sources =
              mojom_block->get_web_sources_content_block();
          auto* proto_sources =
              proto_block->mutable_web_sources_content_block();
          for (const auto& mojom_source : mojom_sources->sources) {
            if (!mojom_source->url.is_valid() ||
                !mojom_source->favicon_url.is_valid()) {
              DVLOG(1)
                  << "Invalid WebSourcesContentBlock found for persistence";
              continue;
            }
            store::WebSourceProto* proto_source = proto_sources->add_sources();
            proto_source->set_title(mojom_source->title);
            proto_source->set_url(mojom_source->url.spec());
            proto_source->set_favicon_url(mojom_source->favicon_url.spec());
            if (mojom_source->page_content.has_value()) {
              proto_source->set_page_content(
                  mojom_source->page_content.value());
            }
            if (mojom_source->extra_snippets.has_value()) {
              proto_source->mutable_extra_snippets()->Assign(
                  mojom_source->extra_snippets->begin(),
                  mojom_source->extra_snippets->end());
            }
          }
          for (const auto& q : mojom_sources->queries) {
            proto_sources->add_queries(q);
          }
          for (const auto& rich_result : mojom_sources->rich_results) {
            proto_sources->add_rich_results(rich_result);
          }
          break;
        }
        default:
          DVLOG(2) << "Non standard content types are not supported yet.";
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
