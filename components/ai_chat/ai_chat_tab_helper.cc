/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_tab_helper.h"

#include <queue>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/constants.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/base/l10n/l10n_util.h"

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;
using ai_chat::mojom::ConversationTurnVisibility;

namespace {
static const ax::mojom::Role kContentRoles[] = {
    ax::mojom::Role::kHeading,
    ax::mojom::Role::kParagraph,
};

static const ax::mojom::Role kRolesToSkip[] = {
    ax::mojom::Role::kAudio,
    ax::mojom::Role::kBanner,
    ax::mojom::Role::kButton,
    ax::mojom::Role::kComplementary,
    ax::mojom::Role::kContentInfo,
    ax::mojom::Role::kFooter,
    ax::mojom::Role::kFooterAsNonLandmark,
    ax::mojom::Role::kImage,
    ax::mojom::Role::kLabelText,
    ax::mojom::Role::kNavigation,
    /* input elements */
    ax::mojom::Role::kTextField,
    ax::mojom::Role::kTextFieldWithComboBox,
    ax::mojom::Role::kComboBoxSelect,
    ax::mojom::Role::kListBox,
    ax::mojom::Role::kListBoxOption,
    ax::mojom::Role::kCheckBox,
    ax::mojom::Role::kRadioButton,
    ax::mojom::Role::kSlider,
    ax::mojom::Role::kSpinButton,
    ax::mojom::Role::kSearchBox,
};

void GetContentRootNodes(const ui::AXNode* root,
                         std::vector<const ui::AXNode*>* content_root_nodes) {
  std::queue<const ui::AXNode*> queue;
  queue.push(root);
  while (!queue.empty()) {
    const ui::AXNode* node = queue.front();
    queue.pop();
    // If a main or article node is found, add it to the list of content root
    // nodes and continue. Do not explore children for nested article nodes.
    if (node->GetRole() == ax::mojom::Role::kMain ||
        node->GetRole() == ax::mojom::Role::kArticle) {
      content_root_nodes->push_back(node);
      continue;
    }
    for (auto iter = node->UnignoredChildrenBegin();
         iter != node->UnignoredChildrenEnd(); ++iter) {
      queue.push(iter.get());
    }
  }
}

void AddContentNodesToVector(const ui::AXNode* node,
                             std::vector<const ui::AXNode*>* content_nodes) {
  if (base::Contains(kContentRoles, node->GetRole())) {
    content_nodes->emplace_back(node);
    return;
  }
  if (base::Contains(kRolesToSkip, node->GetRole())) {
    return;
  }
  for (auto iter = node->UnignoredChildrenBegin();
       iter != node->UnignoredChildrenEnd(); ++iter) {
    AddContentNodesToVector(iter.get(), content_nodes);
  }
}

void AddTextNodesToVector(const ui::AXNode* node,
                          std::vector<std::u16string>* strings) {
  const ui::AXNodeData& node_data = node->data();

  if (base::Contains(kRolesToSkip, node_data.role)) {
    return;
  }

  if (node_data.role == ax::mojom::Role::kStaticText) {
    if (node_data.HasStringAttribute(ax::mojom::StringAttribute::kName)) {
      strings->push_back(
          node_data.GetString16Attribute(ax::mojom::StringAttribute::kName));
    }
    return;
  }

  for (const auto* child : node->children()) {
    AddTextNodesToVector(child, strings);
  }
}
}  // namespace

AIChatTabHelper::AIChatTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AIChatTabHelper>(*web_contents) {
  ai_chat_api_ =
      std::make_unique<AIChatAPI>(web_contents->GetBrowserContext()
                                      ->GetDefaultStoragePartition()
                                      ->GetURLLoaderFactoryForBrowserProcess());
}

AIChatTabHelper::~AIChatTabHelper() = default;

const std::vector<ConversationTurn>& AIChatTabHelper::GetConversationHistory() {
  return chat_history_;
}

const std::string& AIChatTabHelper::GetConversationHistoryString() {
  std::vector<std::string> turn_strings;
  for (const ConversationTurn& turn : chat_history_) {
    turn_strings.push_back((turn.character_type == CharacterType::HUMAN
                                ? ai_chat::kHumanPromptPlaceholder
                                : ai_chat::kAIPromptPlaceholder) +
                           turn.text);
  }

  history_text_ = base::JoinString(turn_strings, "");

  return history_text_;
}

void AIChatTabHelper::AddToConversationHistory(const ConversationTurn& turn) {
  chat_history_.push_back(turn);

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::UpdateOrCreateLastAssistantEntry(
    const std::string& updated_text) {
  if (chat_history_.empty() ||
      chat_history_.back().character_type != CharacterType::ASSISTANT) {
    AddToConversationHistory({CharacterType::ASSISTANT,
                              ConversationTurnVisibility::VISIBLE,
                              updated_text});
  } else {
    chat_history_.back().text = updated_text;
  }

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::SetArticleSummaryString(const std::string& text) {
  article_summary_ = text;
}

void AIChatTabHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void AIChatTabHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AIChatTabHelper::RequestSummary() {
  if (!article_summary_.empty()) {
    VLOG(1) << __func__ << " Article summary is in cache\n";

    AddToConversationHistory({
        CharacterType::ASSISTANT,
        ConversationTurnVisibility::VISIBLE,
        article_summary_,
    });
    return;
  }

  auto* primary_rfh = web_contents()->GetPrimaryMainFrame();

  if (!primary_rfh) {
    // TODO(petemill): Don't allow the UI to submit requests at this state
    VLOG(1) << "Summary request submitted for a WebContents without a "
               "primary main frame";
    for (auto& obs : observers_) {
      obs.OnRequestSummaryFailed();
    }
    return;
  }

  ui::AXTreeID tree_id = web_contents()->GetPrimaryMainFrame()->GetAXTreeID();
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromAXTreeID(tree_id);

  if (!rfh) {
    // TODO(petemill): Don't allow the UI to submit requests at this state
    VLOG(1) << "Summary request submitted for a WebContents without a"
               "primary AXTree-associated RenderFrameHost yet";
    for (auto& obs : observers_) {
      obs.OnRequestSummaryFailed();
    }
    return;
  }

  // TODO(@nullhook): Add a timeout and test this on real pages
  web_contents()->RequestAXTreeSnapshot(
      base::BindOnce(&AIChatTabHelper::OnSnapshotFinished,
                     base::Unretained(this)),
      ui::AXMode::kWebContents,
      /* max_nodes= */ 5000,
      /* timeout= */ {});
}

void AIChatTabHelper::OnSnapshotFinished(const ui::AXTreeUpdate& snapshot) {
  ui::AXTree tree;
  if (!tree.Unserialize(snapshot)) {
    for (auto& obs : observers_) {
      obs.OnRequestSummaryFailed();
    }
    return;
  }

  // Start AX distillation process
  // Don't copy the tree, as it can be expensive.
  DistillViaAlgorithm(tree);
}

void AIChatTabHelper::DistillViaAlgorithm(const ui::AXTree& tree) {
  std::vector<const ui::AXNode*> content_root_nodes;
  std::vector<const ui::AXNode*> content_nodes;
  GetContentRootNodes(tree.root(), &content_root_nodes);

  for (const ui::AXNode* content_root_node : content_root_nodes) {
    AddContentNodesToVector(content_root_node, &content_nodes);
  }

  std::vector<std::u16string> text_node_contents;
  for (const ui::AXNode* content_node : content_nodes) {
    AddTextNodesToVector(content_node, &text_node_contents);
  }

  // TODO(nullhook): The assumption here is that 9300 chars equate to
  // approximately 2k tokens, which is a rough estimate. A proper tokenizer is
  // needed for accurate measurement.
  std::string contents_text = base::UTF16ToUTF8(
      base::JoinString(text_node_contents, u" ").substr(0, 9300));
  if (contents_text.empty()) {
    VLOG(1) << __func__ << " Contents is empty\n";

    for (auto& obs : observers_) {
      obs.OnRequestSummaryFailed();
    }
    return;
  }

  // Prevent indirect prompt injections being sent to the AI model.
  // TODO(nullhook): Abstract prompt injection cleanups to a central place
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, ai_chat::kHumanPrompt,
                                     "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, ai_chat::kAIPrompt, "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<article>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</article>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<history>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</history>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<question>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</question>", "");

  VLOG(1) << __func__
          << " Number of chars in content text = " << contents_text.length()
          << "\n";

  article_text_ = contents_text;

  std::string summarize_prompt = "Summarize the above article.";

  // We hide the prompt with article content from the user
  MakeAPIRequestWithConversationHistoryUpdate(
      {CharacterType::HUMAN, ConversationTurnVisibility::INTERNAL,
       summarize_prompt});
}

void AIChatTabHelper::CleanUp() {
  chat_history_.clear();
  article_summary_.clear();
  article_text_.clear();
  is_request_in_progress_ = false;

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::MakeAPIRequestWithConversationHistoryUpdate(
    const ConversationTurn& turn) {
  std::string prompt = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF8(IDS_AI_CHAT_SUMMARIZE_PROMPT),
      {article_text_, GetConversationHistoryString(), turn.text}, nullptr);

  std::string prompt_with_history =
      base::StrCat({prompt, ai_chat::kAIPrompt, " <response>\n"});

  if (turn.visibility != ConversationTurnVisibility::INTERNAL) {
    AddToConversationHistory(turn);
  }

  DCHECK(ai_chat_api_);

  // Assuming a hidden conversation has a summary prompt,
  // the incoming response is expected to include the AI-generated summary.
  // TODO(nullhook): Improve this heuristic, as it may or may not be true.
  bool is_summarize_prompt =
      turn.visibility == ConversationTurnVisibility::INTERNAL ? true : false;

  auto data_received_callback = base::BindRepeating(
      &AIChatTabHelper::OnAPIStreamDataReceived, base::Unretained(this));

  auto data_completed_callback =
      base::BindOnce(&AIChatTabHelper::OnAPIStreamDataComplete,
                     base::Unretained(this), is_summarize_prompt);

  is_request_in_progress_ = true;
  ai_chat_api_->QueryPrompt(std::move(prompt_with_history),
                            std::move(data_completed_callback),
                            std::move(data_received_callback));
}

bool AIChatTabHelper::IsRequestInProgress() {
  DCHECK(ai_chat_api_);

  return is_request_in_progress_;
}

void AIChatTabHelper::OnAPIStreamDataReceived(
    data_decoder::DataDecoder::ValueOrError result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  if (const std::string* completion =
          result->GetDict().FindString("completion")) {
    UpdateOrCreateLastAssistantEntry(*completion);

    // Trigger an observer update to refresh the UI.
    for (auto& obs : observers_) {
      obs.OnAPIRequestInProgress(IsRequestInProgress());
    }
  }
}

void AIChatTabHelper::OnAPIStreamDataComplete(
    bool is_summarize_prompt,
    api_request_helper::APIRequestResult result,
    bool success) {
  if (success) {
    // TODO(nullhook): Remove this as we don't cache summaries anymore
    if (is_summarize_prompt && !chat_history_.empty()) {
      const ConversationTurn& last_turn = chat_history_.back();
      if (last_turn.character_type == CharacterType::ASSISTANT) {
        SetArticleSummaryString(last_turn.text);
      }
    }

    // We're checking for a value body in case for non-streaming API results.
    if (result.value_body().is_dict()) {
      if (const std::string* completion =
              result.value_body().GetDict().FindString("completion")) {
        AddToConversationHistory(
            ConversationTurn{CharacterType::ASSISTANT,
                             ConversationTurnVisibility::VISIBLE, *completion});
      }
    }
  }

  if (!success || !result.Is2XXResponseCode()) {
    // TODO(petemill): show error state separate from assistant message
    AddToConversationHistory(ConversationTurn{
        CharacterType::ASSISTANT, ConversationTurnVisibility::VISIBLE,
        l10n_util::GetStringUTF8(IDS_CHAT_UI_API_ERROR)});
  }

  is_request_in_progress_ = false;

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void AIChatTabHelper::PrimaryPageChanged(content::Page& page) {
  // TODO(nullhook): Cancel inflight API request
  CleanUp();
}

void AIChatTabHelper::WebContentsDestroyed() {
  CleanUp();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);
