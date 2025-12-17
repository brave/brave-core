// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"

#include "base/containers/adapters.h"
#include "base/strings/escape.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {

mojom::ContentBlockPtr GetContentBlockFromAssociatedContent(
    const PageContent& content,
    uint32_t remaining_length,
    base::FunctionRef<void(std::string&)> sanitize_input) {
  std::string truncated = content.content.substr(0, remaining_length);
  sanitize_input(truncated);
  if (content.is_video) {
    return mojom::ContentBlock::NewVideoTranscriptContentBlock(
        mojom::VideoTranscriptContentBlock::New(std::move(truncated)));
  } else {
    return mojom::ContentBlock::NewPageTextContentBlock(
        mojom::PageTextContentBlock::New(std::move(truncated)));
  }
}

std::vector<mojom::ContentBlockPtr> BuildOAIPageContentBlocks(
    const PageContents& page_contents,
    uint32_t& max_associated_content_length,
    base::FunctionRef<void(std::string&)> sanitize_input,
    std::optional<uint32_t> max_per_content_length = std::nullopt) {
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

  // Step 1:
  //   - generate content blocks for the page contents which we're going to
  //   keep.
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

    if (remaining_length == 0) {
      break;
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

    oai_messages.emplace_back(std::move(oai_message));
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

}  // namespace ai_chat
