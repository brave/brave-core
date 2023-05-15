/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_tab_helper.h"

#include <queue>
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
                                ? ai_chat::kHumanPrompt
                                : ai_chat::kAIPrompt) +
                           turn.text);
  }

  history_text_ = base::JoinString(turn_strings, "\n");

  return history_text_;
}

void AIChatTabHelper::AddToConversationHistory(const ConversationTurn& turn) {
  chat_history_.push_back(turn);

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

  VLOG(1) << __func__
          << " Number of chars in content text = " << contents_text.length()
          << "\n";

  std::string summarize_prompt = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF8(IDS_AI_CHAT_SUMMARIZE_PROMPT), {contents_text},
      nullptr);

  // We hide the prompt with article content from the user
  MakeAPIRequestWithConversationHistoryUpdate(
      {CharacterType::HUMAN, ConversationTurnVisibility::HIDDEN,
       summarize_prompt});
}

void AIChatTabHelper::CleanUp() {
  chat_history_.clear();
  article_summary_.clear();
  SetRequestInProgress(false);

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::MakeAPIRequestWithConversationHistoryUpdate(
    const ConversationTurn& turn) {
  AddToConversationHistory(turn);

  std::string prompt_with_history =
      base::StrCat({GetConversationHistoryString(), ai_chat::kAIPrompt});

  DCHECK(ai_chat_api_);

  // Assuming a hidden conversation has a summary prompt,
  // the incoming response is expected to include the AI-generated summary.
  // TODO(nullhook): Improve this heuristic, as it may or may not be true.
  bool contains_summary =
      turn.visibility == ConversationTurnVisibility::HIDDEN ? true : false;

  SetRequestInProgress(true);

  ai_chat_api_->QueryPrompt(
      base::BindOnce(&AIChatTabHelper::OnAPIResponse, base::Unretained(this),
                     contains_summary),
      std::move(prompt_with_history));
}

void AIChatTabHelper::OnAPIResponse(bool contains_summary,
                                    const std::string& assistant_input,
                                    bool success) {
  SetRequestInProgress(false);

  if (!success) {
    // TODO(petemill): show error state separate from assistant message
    AddToConversationHistory(ConversationTurn{
        CharacterType::ASSISTANT, ConversationTurnVisibility::VISIBLE,
        l10n_util::GetStringUTF8(IDS_CHAT_UI_API_ERROR)});

    return;
  }

  ConversationTurn turn = {CharacterType::ASSISTANT,
                           ConversationTurnVisibility::VISIBLE,
                           assistant_input};

  if (contains_summary && !assistant_input.empty()) {
    SetArticleSummaryString(assistant_input);
  }

  AddToConversationHistory(turn);
}

void AIChatTabHelper::SetRequestInProgress(bool in_progress) {
  is_request_in_progress_ = in_progress;

  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void AIChatTabHelper::PrimaryPageChanged(content::Page& page) {
  // TODO(nullhook): Cancel inflight API requests
  CleanUp();
}

void AIChatTabHelper::WebContentsDestroyed() {
  CleanUp();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);
