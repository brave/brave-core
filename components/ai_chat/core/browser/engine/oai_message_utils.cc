// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"

#include "base/containers/adapters.h"
#include "base/containers/span.h"
#include "base/json/json_writer.h"
#include "base/strings/escape.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace ai_chat {

namespace {

std::string SerializeTabsToJson(base::span<const Tab> tabs) {
  base::Value::List tab_value_list;
  for (const auto& tab : tabs) {
    tab_value_list.Append(base::Value::Dict()
                              .Set("id", tab.id)
                              .Set("title", tab.title)
                              .Set("url", tab.origin.Serialize()));
  }
  return base::WriteJson(tab_value_list).value_or("");
}

mojom::ContentBlockPtr GetContentBlockFromAssociatedContent(
    const PageContent& content,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input) {
  std::string truncated(
      base::TruncateUTF8ToByteSize(content.content, remaining_length));
  sanitize_input(truncated);
  if (content.is_video) {
    return mojom::ContentBlock::NewVideoTranscriptContentBlock(
        mojom::VideoTranscriptContentBlock::New(std::move(truncated)));
  } else {
    return mojom::ContentBlock::NewPageTextContentBlock(
        mojom::PageTextContentBlock::New(std::move(truncated)));
  }
}

std::optional<mojom::MemoryContentBlockPtr> BuildMemoryContentBlock(
    PrefService* prefs,
    bool is_temporary_chat) {
  if (is_temporary_chat || !prefs) {
    return std::nullopt;
  }

  auto memories = prefs::GetUserMemoryDictFromPrefs(*prefs);
  if (!memories) {
    return std::nullopt;
  }

  base::flat_map<std::string, mojom::MemoryValuePtr> result;
  for (const auto [key, value] : *memories) {
    if (value.is_string()) {
      result[key] = mojom::MemoryValue::NewStringValue(
          base::EscapeForHTML(value.GetString()));
    } else if (value.is_list()) {
      std::vector<std::string> escaped_list;
      for (const auto& item : value.GetList()) {
        if (item.is_string()) {
          escaped_list.push_back(base::EscapeForHTML(item.GetString()));
        }
      }
      result[key] = mojom::MemoryValue::NewListValue(std::move(escaped_list));
    }
  }

  return mojom::MemoryContentBlock::New(std::move(result));
}

}  // namespace

OAIMessage::OAIMessage() = default;

OAIMessage::OAIMessage(OAIMessage&&) = default;

OAIMessage& OAIMessage::operator=(OAIMessage&&) = default;

OAIMessage::~OAIMessage() = default;

std::vector<mojom::ContentBlockPtr> BuildOAIPageContentBlocks(
    const PageContents& page_contents,
    uint32_t& max_associated_content_length,
    base::FunctionRef<void(std::string&)> sanitize_input,
    std::optional<uint32_t> max_per_content_length) {
  std::vector<mojom::ContentBlockPtr> blocks;

  // Note: We iterate in reverse so that we prefer more recent page content
  // (i.e. the oldest content will be truncated when we run out of context).
  for (const auto& page_content : base::Reversed(page_contents)) {
    uint32_t effective_length_limit = max_associated_content_length;
    if (max_per_content_length.has_value()) {
      effective_length_limit =
          std::min(effective_length_limit, max_per_content_length.value());
    }

    auto block = GetContentBlockFromAssociatedContent(
        page_content, effective_length_limit, sanitize_input);
    uint32_t truncated_size = 0;
    if (block->is_video_transcript_content_block()) {
      truncated_size = block->get_video_transcript_content_block()->text.size();
    } else if (block->is_page_text_content_block()) {
      truncated_size = block->get_page_text_content_block()->text.size();
    }

    blocks.push_back(std::move(block));

    if (truncated_size >= max_associated_content_length) {
      max_associated_content_length = 0;
      break;
    } else {
      max_associated_content_length -= truncated_size;
    }
  }

  return blocks;
}

std::vector<OAIMessage> BuildOAIMessages(
    PageContentsMap&& page_contents,
    const EngineConsumer::ConversationHistory& conversation_history,
    PrefService* prefs,
    bool is_temporary_chat,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input) {
  std::vector<OAIMessage> oai_messages;

  // Key is conversation entry uuid, value is a list of content blocks for that
  // entry.
  // We use this so we can look up all the page content blocks for a given
  // conversation entry.
  base::flat_map<std::string, std::vector<mojom::ContentBlockPtr>>
      page_contents_blocks;

  // We're going to iterate over the conversation entries and
  // build a list of messages for the remote API.
  // We largely want to send the full conversation with all messages and content
  // blocks back to the model in order to preserve the context of the
  // conversation. However, some tool results are extremely large (especially
  // for images), and repetitive. We need a way to remove the noise in order to
  // 1) not overwhelm the model and 2) not surpass the max token limit. For now,
  // this is a rudimentary approach that only keeps the most recent large tool
  // results. Use a two-pass approach: first identify which large tool results
  // to keep, then build the conversation in chronological order.

  // Step 1:
  //   - identify large tool results and remember which ones to remove.
  //   - generate content blocks for the page contents which we're going to
  //   keep.
  absl::flat_hash_set<std::pair<size_t, size_t>> large_tool_result_remove_set;
  size_t large_tool_count = 0;

  for (size_t message_index = conversation_history.size(); message_index > 0;
       --message_index) {
    const auto& message = conversation_history[message_index - 1];
    DCHECK(message->uuid) << "Tried to send a turn without a uuid";
    if (!message->uuid) {
      continue;
    }

    // If we have page contents for this turn, generate a content block for
    // each.
    auto page_content_it = page_contents.find(message->uuid.value());
    if (page_content_it != page_contents.end() && remaining_length != 0) {
      page_contents_blocks[message->uuid.value()] = BuildOAIPageContentBlocks(
          page_content_it->second, remaining_length, sanitize_input);
    }

    if (message->character_type == mojom::CharacterType::ASSISTANT &&
        message->events.has_value() && !message->events->empty()) {
      for (size_t event_index = message->events->size(); event_index > 0;
           --event_index) {
        const auto& message_event = message->events.value()[event_index - 1];
        if (!message_event->is_tool_use_event()) {
          continue;
        }

        const auto& tool_event = message_event->get_tool_use_event();
        if (!tool_event->output.has_value() || tool_event->output->empty()) {
          continue;
        }

        // Check if this tool result is large
        bool is_large = false;
        size_t content_size = 0;
        for (const auto& content : tool_event->output.value()) {
          if (content->is_image_content_block()) {
            is_large = true;
            break;
          } else if (content->is_text_content_block()) {
            content_size += content->get_text_content_block()->text.size();
            if (content_size >= features::kContentSizeLargeToolUseEvent.Get()) {
              is_large = true;
              break;
            }
          }
        }

        if (is_large) {
          large_tool_count++;
          if (large_tool_count > features::kMaxCountLargeToolUseEvents.Get()) {
            large_tool_result_remove_set.insert(
                {message_index - 1, event_index - 1});
          }
        }
      }
    }
  }

  // Step 2: Main pass - build conversation in chronological order
  for (size_t message_index = 0; message_index < conversation_history.size();
       ++message_index) {
    const auto& message = conversation_history[message_index];

    OAIMessage oai_message;
    oai_message.role = message->character_type == mojom::CharacterType::HUMAN
                           ? "user"
                           : "assistant";

    // Add memory content block for latest human turn.
    if (message->character_type == mojom::CharacterType::HUMAN &&
        message_index == conversation_history.size() - 1) {
      auto memory_block = BuildMemoryContentBlock(prefs, is_temporary_chat);
      if (memory_block) {
        oai_message.content.push_back(
            mojom::ContentBlock::NewMemoryContentBlock(
                std::move(*memory_block)));
      }
    }

    // Append associated content for the message (if any).
    // Note: We don't create the blocks here because we want to keep the newest
    // page contents until we run out of context, so they need to be built in
    // reverse chronological order.
    auto page_content_it = page_contents_blocks.find(message->uuid.value());
    if (page_content_it != page_contents_blocks.end()) {
      for (auto& block : page_content_it->second) {
        oai_message.content.push_back(std::move(block));
      }
    }

    if (message->uploaded_files) {
      std::vector<mojom::ContentBlockPtr> uploaded_images_content_blocks;
      std::vector<mojom::ContentBlockPtr> screenshots_content_blocks;
      std::vector<mojom::ContentBlockPtr> uploaded_pdfs_content_blocks;

      uploaded_images_content_blocks.push_back(
          mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
              "These images are uploaded by the user")));
      screenshots_content_blocks.push_back(
          mojom::ContentBlock::NewTextContentBlock(
              mojom::TextContentBlock::New("These images are screenshots")));
      uploaded_pdfs_content_blocks.push_back(
          mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
              "These PDFs are uploaded by the user")));

      for (const auto& uploaded_file : *message->uploaded_files) {
        if (uploaded_file->type == mojom::UploadedFileType::kImage ||
            uploaded_file->type == mojom::UploadedFileType::kScreenshot) {
          auto image = mojom::ContentBlock::NewImageContentBlock(
              mojom::ImageContentBlock::New(
                  GURL(EngineConsumer::GetImageDataURL(uploaded_file->data))));
          if (uploaded_file->type == mojom::UploadedFileType::kImage) {
            uploaded_images_content_blocks.push_back(std::move(image));
          } else {
            screenshots_content_blocks.push_back(std::move(image));
          }
        } else if (uploaded_file->type == mojom::UploadedFileType::kPdf) {
          uploaded_pdfs_content_blocks.push_back(
              mojom::ContentBlock::NewFileContentBlock(
                  mojom::FileContentBlock::New(
                      GURL(EngineConsumer::GetPdfDataURL(uploaded_file->data)),
                      uploaded_file->filename.empty()
                          ? "uploaded.pdf"
                          : uploaded_file->filename)));
        }
      }

      if (uploaded_images_content_blocks.size() > 1) {
        oai_message.content.insert(
            oai_message.content.end(),
            std::make_move_iterator(uploaded_images_content_blocks.begin()),
            std::make_move_iterator(uploaded_images_content_blocks.end()));
      }

      if (screenshots_content_blocks.size() > 1) {
        oai_message.content.insert(
            oai_message.content.end(),
            std::make_move_iterator(screenshots_content_blocks.begin()),
            std::make_move_iterator(screenshots_content_blocks.end()));
      }

      if (uploaded_pdfs_content_blocks.size() > 1) {
        oai_message.content.insert(
            oai_message.content.end(),
            std::make_move_iterator(uploaded_pdfs_content_blocks.begin()),
            std::make_move_iterator(uploaded_pdfs_content_blocks.end()));
      }
    }

    if (message->selected_text.has_value() &&
        !message->selected_text->empty()) {
      oai_message.content.push_back(
          mojom::ContentBlock::NewPageExcerptContentBlock(
              mojom::PageExcerptContentBlock::New(*message->selected_text)));
    }

    // Add Skill definition content block if this turn has one
    if (message->character_type == mojom::CharacterType::HUMAN &&
        message->skill) {
      std::string skill_definition =
          EngineConsumer::BuildSkillDefinitionMessage(message->skill);
      oai_message.content.push_back(mojom::ContentBlock::NewTextContentBlock(
          mojom::TextContentBlock::New(skill_definition)));
    }

    // Add tool calls to assistant message
    if (message->character_type == mojom::CharacterType::ASSISTANT &&
        message->events.has_value() && !message->events->empty()) {
      for (size_t event_index = 0; event_index < message->events->size();
           ++event_index) {
        const auto& message_event = message->events.value()[event_index];
        if (!message_event->is_tool_use_event()) {
          continue;
        }

        const auto& tool_event = message_event->get_tool_use_event();
        if (tool_event->output.has_value()) {
          oai_message.tool_calls.push_back(tool_event->Clone());
        }
      }
    }

    // Build the main content block
    if (message->action_type == mojom::ActionType::SUMMARIZE_PAGE) {
      oai_message.content.push_back(
          mojom::ContentBlock::NewSimpleRequestContentBlock(
              mojom::SimpleRequestContentBlock::New(
                  mojom::SimpleRequestType::kRequestSummary)));
    } else {
      oai_message.content.push_back(
          mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
              EngineConsumer::GetPromptForEntry(message))));
    }

    // Add the assistant message first
    oai_messages.emplace_back(std::move(oai_message));

    // Add tool results as separate tool messages
    if (message->character_type == mojom::CharacterType::ASSISTANT &&
        message->events.has_value() && !message->events->empty()) {
      for (size_t event_index = 0; event_index < message->events->size();
           ++event_index) {
        const auto& message_event = message->events.value()[event_index];
        if (!message_event->is_tool_use_event()) {
          continue;
        }

        const auto& tool_event = message_event->get_tool_use_event();
        if (!tool_event->output.has_value()) {
          continue;
        }

        // Create separate OAIMessage for tool result
        OAIMessage tool_result_message;
        tool_result_message.role = "tool";
        tool_result_message.tool_call_id = tool_event->id;

        // Check if we should keep the full content for this large tool result
        bool should_keep_full_content = !large_tool_result_remove_set.contains(
            {message_index, event_index});

        if (should_keep_full_content) {
          for (const auto& item : tool_event->output.value()) {
            tool_result_message.content.push_back(item.Clone());
          }
        } else {
          // Add text block for truncated result
          tool_result_message.content.push_back(
              mojom::ContentBlock::NewTextContentBlock(
                  mojom::TextContentBlock::New(
                      "[Large result removed to save space for "
                      "subsequent results]")));
        }

        oai_messages.emplace_back(std::move(tool_result_message));
      }
    }
  }

  return oai_messages;
}

std::vector<OAIMessage> BuildOAIQuestionSuggestionsMessages(
    PageContents page_contents,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input) {
  std::vector<OAIMessage> messages;
  OAIMessage msg;
  msg.role = "user";

  auto blocks = BuildOAIPageContentBlocks(page_contents, remaining_length,
                                          sanitize_input);
  msg.content.insert(msg.content.end(), std::make_move_iterator(blocks.begin()),
                     std::make_move_iterator(blocks.end()));

  msg.content.push_back(mojom::ContentBlock::NewSimpleRequestContentBlock(
      mojom::SimpleRequestContentBlock::New(
          mojom::SimpleRequestType::kRequestQuestions)));
  messages.push_back(std::move(msg));

  return messages;
}

std::optional<std::vector<OAIMessage>> BuildOAIRewriteSuggestionMessages(
    const std::string& text,
    mojom::ActionType action_type) {
  std::vector<OAIMessage> messages;
  OAIMessage msg;
  msg.role = "user";

  msg.content.push_back(mojom::ContentBlock::NewPageExcerptContentBlock(
      mojom::PageExcerptContentBlock::New(text)));

  switch (action_type) {
    case mojom::ActionType::PARAPHRASE:
      msg.content.push_back(mojom::ContentBlock::NewSimpleRequestContentBlock(
          mojom::SimpleRequestContentBlock::New(
              mojom::SimpleRequestType::kParaphrase)));
      break;
    case mojom::ActionType::IMPROVE:
      msg.content.push_back(mojom::ContentBlock::NewSimpleRequestContentBlock(
          mojom::SimpleRequestContentBlock::New(
              mojom::SimpleRequestType::kImprove)));
      break;
    case mojom::ActionType::ACADEMICIZE:
      msg.content.push_back(mojom::ContentBlock::NewChangeToneContentBlock(
          mojom::ChangeToneContentBlock::New("", "academic")));
      break;
    case mojom::ActionType::PROFESSIONALIZE:
      msg.content.push_back(mojom::ContentBlock::NewChangeToneContentBlock(
          mojom::ChangeToneContentBlock::New("", "professional")));
      break;
    case mojom::ActionType::PERSUASIVE_TONE:
      msg.content.push_back(mojom::ContentBlock::NewChangeToneContentBlock(
          mojom::ChangeToneContentBlock::New("", "persuasive")));
      break;
    case mojom::ActionType::CASUALIZE:
      msg.content.push_back(mojom::ContentBlock::NewChangeToneContentBlock(
          mojom::ChangeToneContentBlock::New("", "casual")));
      break;
    case mojom::ActionType::FUNNY_TONE:
      msg.content.push_back(mojom::ContentBlock::NewChangeToneContentBlock(
          mojom::ChangeToneContentBlock::New("", "funny")));
      break;
    case mojom::ActionType::SHORTEN:
      msg.content.push_back(mojom::ContentBlock::NewSimpleRequestContentBlock(
          mojom::SimpleRequestContentBlock::New(
              mojom::SimpleRequestType::kShorten)));
      break;
    case mojom::ActionType::EXPAND:
      msg.content.push_back(mojom::ContentBlock::NewSimpleRequestContentBlock(
          mojom::SimpleRequestContentBlock::New(
              mojom::SimpleRequestType::kExpand)));
      break;
    default:
      return std::nullopt;
  }

  messages.push_back(std::move(msg));
  return messages;
}

std::optional<std::vector<OAIMessage>>
BuildOAIGenerateConversationTitleMessages(
    const PageContentsMap& page_contents,
    const EngineConsumer::ConversationHistory& conversation_history,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input) {
  // Validate we have the expected conversation structure
  if (conversation_history.size() != 2 ||
      conversation_history[0]->character_type != mojom::CharacterType::HUMAN ||
      conversation_history[1]->character_type !=
          mojom::CharacterType::ASSISTANT) {
    return std::nullopt;
  }

  const auto& first_turn = conversation_history[0];
  const auto& assistant_turn = conversation_history[1];

  // Build messages for title generation
  std::vector<OAIMessage> messages;
  OAIMessage msg;
  msg.role = "user";

  // Add page contents from the first turn if available
  auto page_content_it = page_contents.find(first_turn->uuid.value());
  if (page_content_it != page_contents.end()) {
    auto blocks = BuildOAIPageContentBlocks(page_content_it->second,
                                            remaining_length, sanitize_input,
                                            kMaxContextCharsForTitleGeneration);
    msg.content.insert(msg.content.end(),
                       std::make_move_iterator(blocks.begin()),
                       std::make_move_iterator(blocks.end()));
  }

  // Add selected text as page excerpt if present
  if (first_turn->selected_text.has_value() &&
      !first_turn->selected_text->empty()) {
    msg.content.push_back(mojom::ContentBlock::NewPageExcerptContentBlock(
        mojom::PageExcerptContentBlock::New(*first_turn->selected_text)));
  }

  // Add a message for title generation.
  // Use first assistant response as the content if files are uploaded (image,
  // PDF), otherwise use the first human turn text.
  msg.content.push_back(mojom::ContentBlock::NewRequestTitleContentBlock(
      mojom::RequestTitleContentBlock::New(
          first_turn->uploaded_files
              ? assistant_turn->text
              : EngineConsumer::GetPromptForEntry(first_turn))));

  messages.push_back(std::move(msg));
  return messages;
}

OAIMessage BuildOAISeedMessage(const std::string& text) {
  OAIMessage message;
  message.role = "assistant";
  message.content.push_back(mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New(text)));
  return message;
}

std::vector<OAIMessage> BuildOAIDedupeTopicsMessages(
    const std::vector<std::string>& topics) {
  // Serialize topics to JSON array
  base::Value::List topic_list;
  for (const auto& topic : topics) {
    topic_list.Append(topic);
  }

  std::string topics_json = base::WriteJson(topic_list).value_or("");

  // Create ReduceFocusTopicsContentBlock
  auto content_block = mojom::ContentBlock::NewReduceFocusTopicsContentBlock(
      mojom::ReduceFocusTopicsContentBlock::New(topics_json));

  // Build and return message
  OAIMessage message;
  message.role = "user";
  message.content.push_back(std::move(content_block));

  std::vector<OAIMessage> messages;
  messages.push_back(std::move(message));
  return messages;
}

std::vector<std::vector<OAIMessage>> BuildChunkedTabFocusMessages(
    const std::vector<Tab>& tabs,
    const std::string& topic) {
  std::vector<std::vector<OAIMessage>> chunked_messages;
  size_t num_chunks = (tabs.size() + kTabListChunkSize - 1) / kTabListChunkSize;

  for (size_t chunk = 0; chunk < num_chunks; ++chunk) {
    size_t start = chunk * kTabListChunkSize;
    size_t end = std::min((chunk + 1) * kTabListChunkSize, tabs.size());
    base::span<const Tab> chunk_tabs =
        base::span(tabs).subspan(start, end - start);

    std::string tabs_json = SerializeTabsToJson(chunk_tabs);

    // Create appropriate content block
    mojom::ContentBlockPtr content_block;
    if (topic.empty()) {
      // Suggest topics (with or without emoji based on chunk count)
      // Single chunk: Create SuggestFocusTopicsWithEmojiContentBlock to get
      // topics with an emoji appended to each topic in one single request.
      // Multiple chunks: Create SuggestFocusTopicsContentBlock to get topics
      // without emoji in multiple requests, and EngineConsumer would trigger
      // DedupeTopics with results from all requests to get the final set of
      // topics with an emoji appended to each topic.
      content_block =
          num_chunks == 1u
              ? mojom::ContentBlock::NewSuggestFocusTopicsWithEmojiContentBlock(
                    mojom::SuggestFocusTopicsWithEmojiContentBlock::New(
                        tabs_json))
              : mojom::ContentBlock::NewSuggestFocusTopicsContentBlock(
                    mojom::SuggestFocusTopicsContentBlock::New(tabs_json));
    } else {
      // Filter tabs by topic
      content_block = mojom::ContentBlock::NewFilterTabsContentBlock(
          mojom::FilterTabsContentBlock::New(tabs_json, topic));
    }

    OAIMessage message;
    message.role = "user";
    message.content.push_back(std::move(content_block));

    std::vector<OAIMessage> messages;
    messages.push_back(std::move(message));
    chunked_messages.push_back(std::move(messages));
  }

  return chunked_messages;
}

}  // namespace ai_chat
