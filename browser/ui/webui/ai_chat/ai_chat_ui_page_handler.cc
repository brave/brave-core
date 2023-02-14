// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "brave/browser/ai_chat/constants.h"
#include "brave/browser/ai_chat/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;
using ai_chat::mojom::ConversationTurnVisibility;

AIChatUIPageHandler::AIChatUIPageHandler(
    TabStripModel* tab_strip_model,
    Profile* profile,
    mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver)
    : profile_(profile), receiver_(this, std::move(receiver)) {
  DCHECK(tab_strip_model);
  tab_strip_model->AddObserver(this);

  auto* web_contents = tab_strip_model->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  active_chat_tab_helper_ = AIChatTabHelper::FromWebContents(web_contents);

  chat_tab_helper_observation_.Observe(active_chat_tab_helper_);
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

  active_chat_tab_helper_->MakeAPIRequestWithConversationHistoryUpdate(
      {CharacterType::HUMAN, ConversationTurnVisibility::VISIBLE, input_copy});
}

void AIChatUIPageHandler::GetConversationHistory(
    GetConversationHistoryCallback callback) {
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

void AIChatUIPageHandler::RequestSummary() {
  active_chat_tab_helper_->RequestSummary();
}

void AIChatUIPageHandler::MarkAgreementAccepted() {
  profile_->GetOriginalProfile()->GetPrefs()->SetBoolean(
      ai_chat::prefs::kBraveChatHasSeenDisclaimer, true);
}

void AIChatUIPageHandler::OnHistoryUpdate() {
  page_.get()->OnConversationHistoryUpdate();
  page_.get()->OnAPIRequestInProgress(
      active_chat_tab_helper_->IsRequestInProgress());
}

void AIChatUIPageHandler::OnAPIRequestInProgress(bool in_progress) {
  page_.get()->OnAPIRequestInProgress(in_progress);
}

void AIChatUIPageHandler::OnRequestSummaryFailed() {
  page_.get()->OnContentSummarizationFailed();
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

      OnHistoryUpdate();
    }
  }
}
