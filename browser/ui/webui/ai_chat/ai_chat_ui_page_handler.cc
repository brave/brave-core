// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/browser/constants.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/common/pref_names.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/favicon/core/favicon_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace {
constexpr uint32_t kDesiredFaviconSizePixels = 32;
}  // namespace

namespace ai_chat {

using mojom::CharacterType;
using mojom::ConversationTurn;
using mojom::ConversationTurnVisibility;

AIChatUIPageHandler::AIChatUIPageHandler(
    content::WebContents* owner_web_contents,
    TabStripModel* tab_strip_model,
    Profile* profile,
    mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter)
    : content::WebContentsObserver(owner_web_contents),
      profile_(profile),
      receiver_(this, std::move(receiver)),
      skus_service_getter_(skus_service_getter) {
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
    bool is_visible =
        (web_contents->GetVisibility() == content::Visibility::VISIBLE) ? true
                                                                        : false;
    active_chat_tab_helper_->OnConversationActiveChanged(is_visible);
  } else {
    // TODO(petemill): Enable conversation without the TabHelper. Conversation
    // logic should be extracted from the TabHelper to a new virtual class, e.g.
    // AIChatConverser, that the TabHelper can implement and a
    // StandaloneAIChatConverser can also implement and be instantiated here.
    NOTIMPLEMENTED();
  }

  favicon_service_ = FaviconServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
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
  if (active_chat_tab_helper_) {
    bool can_generate;
    mojom::AutoGenerateQuestionsPref auto_generate;
    std::move(callback).Run(active_chat_tab_helper_->GetSuggestedQuestions(
                                can_generate, auto_generate),
                            can_generate, auto_generate);
  }
}

void AIChatUIPageHandler::GenerateQuestions() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->GenerateQuestions();
  }
}

void AIChatUIPageHandler::SetAutoGenerateQuestions(bool value) {
  profile_->GetPrefs()->SetBoolean(
      ai_chat::prefs::kBraveChatAutoGenerateQuestions, value);
}

void AIChatUIPageHandler::GetSiteInfo(GetSiteInfoCallback callback) {
  auto site_info = BuildSiteInfo();
  std::move(callback).Run(site_info.has_value() ? site_info.value().Clone()
                                                : nullptr);
}

void AIChatUIPageHandler::OpenBraveLeoSettings() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->web_contents()->OpenURL(
        {GURL("brave://settings/leo-assistant"), content::Referrer(),
         WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
         false});
  }
}

void AIChatUIPageHandler::OpenBraveLeoWiki() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->web_contents()->OpenURL(
        {GURL("https://github.com/brave/brave-browser/wiki/Brave-Leo"),
         content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
         ui::PAGE_TRANSITION_LINK, false});
  }
}

void AIChatUIPageHandler::DisconnectPageContents() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->DisconnectPageContents();
  }
}

void AIChatUIPageHandler::ClearConversationHistory() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->ClearConversationHistory();
  }
}

void AIChatUIPageHandler::RetryAPIRequest() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->RetryAPIRequest();
  }
}

void AIChatUIPageHandler::GetAPIResponseError(
    GetAPIResponseErrorCallback callback) {
  if (active_chat_tab_helper_) {
    std::move(callback).Run(active_chat_tab_helper_->GetCurrentAPIError());
  }
}

void AIChatUIPageHandler::MarkAgreementAccepted() {
  profile_->GetPrefs()->SetBoolean(ai_chat::prefs::kBraveChatHasSeenDisclaimer,
                                   true);
}

void AIChatUIPageHandler::UserHasValidPremiumCredentials(
    UserHasValidPremiumCredentialsCallback callback) {
  skus_service_->CredentialSummary(
      "domain - TODO", base::BindOnce(&AIChatUIPageHandler::OnCredentialSummary,
                                      base::Unretained(this),
                                      std::move(callback), "domain - TODO"));
}

void AIChatUIPageHandler::OnCredentialSummary(
    UserHasValidPremiumCredentialsCallback callback,
    const std::string& domain,
    const std::string& summary_string) {
  auto env = skus::GetEnvironmentForDomain(domain);
  std::string summary_string_trimmed;
  base::TrimWhitespaceASCII(summary_string, base::TrimPositions::TRIM_ALL,
                            &summary_string_trimmed);
  if (summary_string_trimmed.length() == 0) {
    // no credential found; person needs to login
    std::move(callback).Run(false);
    return;
  }

  absl::optional<base::Value> records_v = base::JSONReader::Read(
      summary_string, base::JSONParserOptions::JSON_PARSE_RFC);

  // Early return when summary is invalid or it's empty dict.
  if (!records_v || !records_v->is_dict()) {
    std::move(callback).Run(false);
    return;
  }

  // Empty dict - clean user.
  if (records_v->GetDict().empty()) {
    std::move(callback).Run(false);
    return;
  }

  const bool active = (*records_v).GetDict().FindBool("active").value_or(false);
  if (!active) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(true);
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

void AIChatUIPageHandler::OnAPIResponseError(mojom::APIError error) {
  if (page_.is_bound()) {
    page_->OnAPIResponseError(error);
  }
}

void AIChatUIPageHandler::OnSuggestedQuestionsChanged(
    std::vector<std::string> questions,
    bool has_generated,
    mojom::AutoGenerateQuestionsPref auto_generate) {
  if (page_.is_bound()) {
    page_->OnSuggestedQuestionsChanged(std::move(questions), has_generated,
                                       auto_generate);
  }
}

void AIChatUIPageHandler::OnFaviconImageDataChanged() {
  if (page_.is_bound()) {
    auto on_favicon_data =
        [](base::SafeRef<AIChatUIPageHandler> page_handler,
           const absl::optional<std::vector<uint8_t>>& bytes) {
          if (bytes.has_value()) {
            page_handler->page_->OnFaviconImageDataChanged(bytes.value());
          }
        };

    GetFaviconImageData(
        base::BindOnce(on_favicon_data, weak_ptr_factory_.GetSafeRef()));
  }
}

void AIChatUIPageHandler::OnPageHasContent() {
  if (page_.is_bound()) {
    auto site_info = BuildSiteInfo();

    page_->OnSiteInfoChanged(site_info.has_value() ? site_info.value().Clone()
                                                   : nullptr);
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
      // Let the tab helper know if the UI is visible
      active_chat_tab_helper_->OnConversationActiveChanged(
          (web_contents()->GetVisibility() == content::Visibility::VISIBLE)
              ? true
              : false);
      chat_tab_helper_observation_.Observe(active_chat_tab_helper_);
    }
    // Reset state
    page_->OnTargetTabChanged();
  }
}

void AIChatUIPageHandler::GetFaviconImageData(
    GetFaviconImageDataCallback callback) {
  if (!active_chat_tab_helper_) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  const GURL active_page_url =
      active_chat_tab_helper_->web_contents()->GetLastCommittedURL();
  favicon_base::IconTypeSet icon_types{favicon_base::IconType::kFavicon,
                                       favicon_base::IconType::kTouchIcon};

  auto on_favicon_available =
      [](GetFaviconImageDataCallback callback,
         const favicon_base::FaviconRawBitmapResult& result) {
        if (!result.is_valid()) {
          std::move(callback).Run(absl::nullopt);
          return;
        }

        scoped_refptr<base::RefCountedMemory> bytes = result.bitmap_data;
        std::vector<uint8_t> buffer(bytes->front_as<uint8_t>(),
                                    bytes->front_as<uint8_t>() + bytes->size());
        std::move(callback).Run(std::move(buffer));
      };

  favicon_service_->GetRawFaviconForPageURL(
      active_page_url, icon_types, kDesiredFaviconSizePixels, true,
      base::BindOnce(on_favicon_available, std::move(callback)),
      &favicon_task_tracker_);
}

absl::optional<mojom::SiteInfo> AIChatUIPageHandler::BuildSiteInfo() {
  if (active_chat_tab_helper_ && active_chat_tab_helper_->HasPageContent()) {
    mojom::SiteInfo site_info;
    site_info.title =
        base::UTF16ToUTF8(active_chat_tab_helper_->web_contents()->GetTitle());

    return site_info;
  }

  return absl::nullopt;
}

void AIChatUIPageHandler::OnVisibilityChanged(content::Visibility visibility) {
  // WebUI visibility changed (not target tab)
  if (active_chat_tab_helper_) {
    bool is_visible =
        (visibility == content::Visibility::VISIBLE) ? true : false;
    active_chat_tab_helper_->OnConversationActiveChanged(is_visible);
  }
}

void AIChatUIPageHandler::EnsureMojoConnected() {
  if (!skus_service_) {
    auto pending = skus_service_getter_.Run();
    skus_service_.Bind(std::move(pending));
  }
  DCHECK(skus_service_);
  skus_service_.set_disconnect_handler(base::BindOnce(
      &AIChatUIPageHandler::OnMojoConnectionError, base::Unretained(this)));
}

void AIChatUIPageHandler::OnMojoConnectionError() {
  skus_service_.reset();
  EnsureMojoConnected();
}

}  // namespace ai_chat
