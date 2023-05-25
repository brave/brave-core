// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "brave/components/ai_chat/ai_chat.mojom.h"
#include "brave/components/ai_chat/constants.h"
#include "brave/components/ai_chat/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

using mojom::CharacterType;
using mojom::ConversationTurn;
using mojom::ConversationTurnVisibility;

AIChatUIPageHandler::AIChatUIPageHandler(
    content::WebContents* owner_web_contents,
    TabStripModel* tab_strip_model,
    Profile* profile,
    mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver)
    : profile_(profile), receiver_(this, std::move(receiver)) {
  DCHECK(tab_strip_model);
  // Detect if we are in target-tab mode, or standalone mode. Standalone mode
  // means Chat is opened as its own tab in the tab strip and not a side panel.
  bool is_standalone = (tab_strip_model->GetIndexOfWebContents(
                            owner_web_contents) != TabStripModel::kNoTab);
  if (!is_standalone) {
    tab_strip_model->AddObserver(this);
    auto* web_contents = tab_strip_model->GetActiveWebContents();
    if (!web_contents) {
      return;
    }
    active_chat_tab_helper_ =
        ai_chat::AIChatTabHelper::FromWebContents(web_contents);
    chat_tab_helper_observation_.Observe(active_chat_tab_helper_);
  } else {
    // TODO(petemill): Enable conversation without the TabHelper. Conversation
    // logic should be extracted from the TabHelper to a new virtual class, e.g.
    // AIChatConverser, that the TabHelper can implement and a
    // StandaloneAIChatConverser can also implement and be instantiated here.
    NOTIMPLEMENTED();
  }
}

AIChatUIPageHandler::~AIChatUIPageHandler() = default;

void AIChatUIPageHandler::SetClientPage(
    mojo::PendingRemote<ai_chat::mojom::ChatUIPage> page) {
  page_.Bind(std::move(page));
}

void AIChatUIPageHandler::SubmitHumanConversationEntry(
    const std::string& input) {
  // TODO(nullhook): Avoid copy
  std::string input_copy = input;

  // Prevent indirect prompt injections being sent to the AI model.
  // TODO(nullhook): Abstract prompt injection cleanups to a central place
  base::ReplaceSubstringsAfterOffset(&input_copy, 0, ai_chat::kHumanPrompt, "");
  base::ReplaceSubstringsAfterOffset(&input_copy, 0, ai_chat::kAIPrompt, "");
  base::ReplaceSubstringsAfterOffset(&input_copy, 0, "<article>", "");
  base::ReplaceSubstringsAfterOffset(&input_copy, 0, "</article>", "");
  base::ReplaceSubstringsAfterOffset(&input_copy, 0, "<history>", "");
  base::ReplaceSubstringsAfterOffset(&input_copy, 0, "</history>", "");
  base::ReplaceSubstringsAfterOffset(&input_copy, 0, "<question>", "");
  base::ReplaceSubstringsAfterOffset(&input_copy, 0, "</question>", "");

  active_chat_tab_helper_->MakeAPIRequestWithConversationHistoryUpdate(
      {CharacterType::HUMAN, ConversationTurnVisibility::VISIBLE, input_copy});
}

void AIChatUIPageHandler::GetConversationHistory(
    GetConversationHistoryCallback callback) {
  if (!active_chat_tab_helper_) {
    std::move(callback).Run({});
    return;
  }
  std::vector<ConversationTurn> history =
      active_chat_tab_helper_->GetConversationHistory();

  std::vector<ai_chat::mojom::ConversationTurnPtr> list;

  // Remove conversations that are meant to be hidden from the user
  auto new_end_it = std::remove_if(
      history.begin(), history.end(), [](const ConversationTurn& turn) {
        return turn.visibility == ConversationTurnVisibility::HIDDEN;
      });

  std::transform(history.begin(), new_end_it, std::back_inserter(list),
                 [](const ConversationTurn& turn) { return turn.Clone(); });

  std::move(callback).Run(std::move(list));
}

void AIChatUIPageHandler::GetSuggestedQuestions(
    GetSuggestedQuestionsCallback callback) {
  bool can_generate;
  bool auto_generate;
  std::move(callback).Run(active_chat_tab_helper_->GetSuggestedQuestions(
                              can_generate, auto_generate),
                          can_generate, auto_generate);
}

void AIChatUIPageHandler::GenerateQuestions() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->GenerateQuestions();
  }
}

void AIChatUIPageHandler::SetAutoGenerateQuestions(bool value) {
  profile_->GetOriginalProfile()->GetPrefs()->SetBoolean(
      ai_chat::prefs::kBraveChatAutoGenerateQuestions, value);
}

void AIChatUIPageHandler::MarkAgreementAccepted() {
  profile_->GetOriginalProfile()->GetPrefs()->SetBoolean(
      ai_chat::prefs::kBraveChatHasSeenDisclaimer, true);
}

void AIChatUIPageHandler::OnHistoryUpdate() {
  if (page_.is_bound()) {
    page_->OnConversationHistoryUpdate();
  }
}

void AIChatUIPageHandler::OnAPIRequestInProgress(bool in_progress) {
  if (page_.is_bound()) {
    page_->OnAPIRequestInProgress(in_progress);
  }
}

void AIChatUIPageHandler::OnSuggestedQuestionsChanged(
    std::vector<std::string> questions,
    bool has_generated,
    bool auto_generate) {
  if (page_.is_bound()) {
    page_->OnSuggestedQuestionsChanged(std::move(questions), has_generated,
                                       auto_generate);
  }
}

void AIChatUIPageHandler::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    if (active_chat_tab_helper_) {
      active_chat_tab_helper_ = nullptr;
      chat_tab_helper_observation_.Reset();
    }

    if (selection.new_contents) {
      active_chat_tab_helper_ =
          AIChatTabHelper::FromWebContents(selection.new_contents);
      chat_tab_helper_observation_.Observe(active_chat_tab_helper_);
    }
    // Reset state
    page_->OnTargetTabChanged();
  }
}

}  // namespace ai_chat
