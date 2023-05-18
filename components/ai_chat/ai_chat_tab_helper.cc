/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_tab_helper.h"

#include <queue>
#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
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
  // TODO(petemill): Observe opt-in - don't send any requests before opt-in
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

void AIChatTabHelper::SetArticleSummaryString(const std::string& text) {
  article_summary_ = text;
}

void AIChatTabHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AIChatTabHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AIChatTabHelper::GeneratePageText() {
  auto* primary_rfh = web_contents()->GetPrimaryMainFrame();

  if (!primary_rfh) {
    VLOG(1) << "Summary request submitted for a WebContents without a "
               "primary main frame";
    return;
  }

  ui::AXTreeID tree_id = web_contents()->GetPrimaryMainFrame()->GetAXTreeID();
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromAXTreeID(tree_id);

  if (!rfh) {
    VLOG(1) << "Summary request submitted for a WebContents without a"
               "primary AXTree-associated RenderFrameHost yet";
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
  // TODO(petemill): Check that the page hasn't navigated away, either
  // by checking navigation ID or refactoring this to a per-navigation
  // class instance.
  ui::AXTree tree;
  if (!tree.Unserialize(snapshot)) {
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

  // Now that we have article text, we can suggest to summarize it
  DCHECK(suggested_questions_.empty())
      << "Expected suggested questions to be clear when there has been no"
      << " previous text content but there were " << suggested_questions_.size()
      << " suggested questions: "
      << base::JoinString(suggested_questions_, ", ");
  suggested_questions_.emplace_back("Summarize this page");
  DVLOG(1) << "Got content text, notifying observers.";
  for (auto& obs : observers_) {
    obs.OnPageTextIsAvailable();
  }
}

void AIChatTabHelper::CleanUp() {
  chat_history_.clear();
  article_summary_.clear();
  article_text_.clear();
  suggested_questions_.clear();
  SetRequestInProgress(false);

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
    obs.OnSuggestedQuestionsChanged(suggested_questions_);
  }
}

std::vector<std::string> AIChatTabHelper::GetSuggestedQuestions() {
  // The first call here should make the API call to fetch suggested questions.
  // We can fetch questions if we have article text and we haven't already
  // got some.
  bool can_fetch_questions =
      !article_text_.empty() && (suggested_questions_.size() <= 1);
  if (can_fetch_questions) {
    GenerateQuestions();
  }
  // Meanwhile, return what we have so far
  // (might be empty or the summarize prompt).
  return suggested_questions_;
}

void AIChatTabHelper::GenerateQuestions() {
  DVLOG(1) << __func__;
  // Can't operate if we don't have an article text
  if (article_text_.empty()) {
    return;
  }
  // Don't perform the operation more than once
  if (suggested_questions_.size() > 1u) {
    return;
  }
  DCHECK(!is_request_in_progress_);
  std::string prompt = base::StrCat(
      {base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT),
           {article_text_}, nullptr),
       "\n\n", l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PROMPT_SEGMENT),
       ai_chat::kAIPrompt, " <response>"});
  // Make API request for questions.
  // Do not call SetRequestInProgress, this progress
  // does not need to be shown to the UI.
  auto navigation_id_for_query = current_navigation_id_;
  ai_chat_api_->QueryPrompt(
      base::BindOnce(
          [](AIChatTabHelper* tab_helper, int64_t navigation_id_for_query,
             const std::string& response, bool success) {
            VLOG(1) << "Received " << (success ? "success" : "failed")
                    << " suggested questions response: " << response;
            // We might have navigated away whilst this async operation is in
            // progress, so check if we're the same navigation.
            if (tab_helper->current_navigation_id_ != navigation_id_for_query) {
              VLOG(1) << "Navigation id was different: "
                      << tab_helper->current_navigation_id_ << " "
                      << navigation_id_for_query;
              return;
            }
            if (!success || response.empty()) {
              return;
            }
            // Parse questions
            auto questions = base::SplitString(
                response, "|", base::WhitespaceHandling::TRIM_WHITESPACE,
                base::SplitResult::SPLIT_WANT_NONEMPTY);
            for (auto question : questions) {
              tab_helper->suggested_questions_.emplace_back(question);
            }
            // Notify observers
            for (auto& obs : tab_helper->observers_) {
              obs.OnSuggestedQuestionsChanged(tab_helper->suggested_questions_);
            }
            VLOG(1) << "Got questions:"
                    << base::JoinString(tab_helper->suggested_questions_, "\n");
          },
          base::Unretained(this), std::move(navigation_id_for_query)),
      prompt);
}

void AIChatTabHelper::MakeAPIRequestWithConversationHistoryUpdate(
    const ConversationTurn& turn) {
  // If it's a suggested question, remove it
  if (turn.character_type == CharacterType::HUMAN) {
    auto found_question_iter =
        base::ranges::find(suggested_questions_, turn.text);
    if (found_question_iter != suggested_questions_.end()) {
      suggested_questions_.erase(found_question_iter);
      for (auto& obs : observers_) {
        obs.OnSuggestedQuestionsChanged(suggested_questions_);
      }
    }
  }

  auto prompt_segment_article =
      article_text_.empty()
          ? ""
          : base::StrCat({base::ReplaceStringPlaceholders(
                              l10n_util::GetStringUTF8(
                                  IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT),
                              {article_text_}, nullptr),
                          "\n\n"});

  auto prompt_segment_history =
      chat_history_.empty()
          ? ""
          : base::ReplaceStringPlaceholders(
                l10n_util::GetStringUTF8(
                    IDS_AI_CHAT_ASSISTANT_HISTORY_PROMPT_SEGMENT),
                {GetConversationHistoryString()}, nullptr);

  std::string prompt = base::StrCat(
      {prompt_segment_article,
       base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(IDS_AI_CHAT_ASSISTANT_PROMPT_SEGMENT),
           {prompt_segment_history, turn.text}, nullptr),
       ai_chat::kAIPrompt, " <response>\n"});

  if (turn.visibility != ConversationTurnVisibility::INTERNAL) {
    AddToConversationHistory(turn);
  }

  DCHECK(ai_chat_api_);

  SetRequestInProgress(true);

  // Assuming a hidden conversation has a summary prompt,
  // the incoming response is expected to include the AI-generated summary.
  // TODO(nullhook): Improve this heuristic, as it may or may not be true.
  bool is_summarize_prompt =
      turn.visibility == ConversationTurnVisibility::INTERNAL ? true : false;

  ai_chat_api_->QueryPrompt(
      base::BindOnce(&AIChatTabHelper::OnAPIResponse, base::Unretained(this),
                     is_summarize_prompt),
      std::move(prompt));
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

void AIChatTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  DVLOG(2) << __func__ << navigation_handle->GetNavigationId()
           << " url: " << navigation_handle->GetURL().spec();
  current_navigation_id_ = navigation_handle->GetNavigationId();
}

void AIChatTabHelper::PrimaryPageChanged(content::Page& page) {
  // TODO(nullhook): Cancel inflight API requests
  CleanUp();
}

void AIChatTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  // We might have content here, so check.
  // TODO(petemill): If there are other navigation events to also
  // check if content is available at, then start a queue and make
  // sure we don't have multiple async distills going on at the same time.
  GeneratePageText();
}

void AIChatTabHelper::WebContentsDestroyed() {
  CleanUp();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);
