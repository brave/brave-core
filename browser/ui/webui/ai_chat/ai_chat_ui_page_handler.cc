// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "components/favicon/core/favicon_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/url_constants.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/android/ai_chat/brave_leo_settings_launcher_helper.h"
#endif

namespace {
constexpr uint32_t kDesiredFaviconSizePixels = 32;
constexpr char kURLRefreshPremiumSession[] =
    "https://account.brave.com/?intent=recover&product=leo";
constexpr char kURLLearnMoreBraveSearchLeo[] =
    "https://support.brave.com/hc/en-us/categories/20990938292237-Brave-Leo";
#if !BUILDFLAG(IS_ANDROID)
constexpr char kURLGoPremium[] =
    "https://account.brave.com/account/?intent=checkout&product=leo";
constexpr char kURLManagePremium[] = "https://account.brave.com/";
#endif
}  // namespace

namespace ai_chat {

using mojom::CharacterType;
using mojom::ConversationTurn;
using mojom::ConversationTurnVisibility;

AIChatUIPageHandler::ChatContextObserver::ChatContextObserver(
    content::WebContents* web_contents,
    AIChatUIPageHandler& page_handler)
    : content::WebContentsObserver(web_contents), page_handler_(page_handler) {}

AIChatUIPageHandler::ChatContextObserver::~ChatContextObserver() = default;

AIChatUIPageHandler::AIChatUIPageHandler(
    content::WebContents* owner_web_contents,
    content::WebContents* chat_context_web_contents,
    Profile* profile,
    mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver)
    : content::WebContentsObserver(owner_web_contents),
      profile_(profile),
      receiver_(this, std::move(receiver)) {
  // Standalone mode means Chat is opened as its own tab in the tab strip and
  // not a side panel. chat_context_web_contents is nullptr in that case
  const bool is_standalone = chat_context_web_contents == nullptr;
  if (!is_standalone) {
    active_chat_tab_helper_ =
        ai_chat::AIChatTabHelper::FromWebContents(chat_context_web_contents);
    chat_tab_helper_observation_.Observe(active_chat_tab_helper_);
    chat_context_observer_ =
        std::make_unique<ChatContextObserver>(chat_context_web_contents, *this);
    // Report visibility of AI Chat UI to the Conversation, so that
    // automatic actions are only performed when neccessary.
    bool is_visible =
        (owner_web_contents->GetVisibility() == content::Visibility::VISIBLE)
            ? true
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

  // In some cases, this page handler hasn't been created and remote might not
  // have been set yet.
  // ex. A user may ask a question from the location bar
  if (active_chat_tab_helper_ &&
      active_chat_tab_helper_->HasPendingConversationEntry()) {
    OnHistoryUpdate();
  }
}

void AIChatUIPageHandler::GetModels(GetModelsCallback callback) {
  if (!active_chat_tab_helper_) {
    VLOG(2) << "Chat tab helper is not set";
    std::move(callback).Run(std::vector<mojom::ModelPtr>(), std::string());
    return;
  }

  const auto& models = active_chat_tab_helper_->GetModels();
  std::vector<mojom::ModelPtr> models_copy(models.size());
  std::transform(models.cbegin(), models.cend(), models_copy.begin(),
                 [](auto& model) { return model.Clone(); });

  std::move(callback).Run(std::move(models_copy),
                          active_chat_tab_helper_->GetCurrentModel().key);
}

void AIChatUIPageHandler::ChangeModel(const std::string& model_key) {
  active_chat_tab_helper_->ChangeModel(model_key);
}

void AIChatUIPageHandler::SubmitHumanConversationEntry(
    const std::string& input) {
  DCHECK(!active_chat_tab_helper_->IsRequestInProgress())
      << "Should not be able to submit more"
      << "than a single human conversation turn at a time.";

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      CharacterType::HUMAN, mojom::ActionType::UNSPECIFIED,
      ConversationTurnVisibility::VISIBLE, input, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, false);
  active_chat_tab_helper_->SubmitHumanConversationEntry(std::move(turn));
}

void AIChatUIPageHandler::SubmitHumanConversationEntryWithAction(
    const std::string& input,
    mojom::ActionType action_type) {
  DCHECK(!active_chat_tab_helper_->IsRequestInProgress())
      << "Should not be able to submit more"
      << "than a single human conversation turn at a time.";

  active_chat_tab_helper_->SubmitSelectedText(input, action_type);
}

void AIChatUIPageHandler::SubmitSummarizationRequest() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->SubmitSummarizationRequest();
  }
}

void AIChatUIPageHandler::HandleVoiceRecognition() {
#if BUILDFLAG(IS_ANDROID)
  auto* context_web_contents = (active_chat_tab_helper_)
                                   ? active_chat_tab_helper_->web_contents()
                                   : web_contents();
  ai_chat::HandleVoiceRecognition(web_contents(), context_web_contents);
#endif
}

void AIChatUIPageHandler::GetConversationHistory(
    GetConversationHistoryCallback callback) {
  if (!active_chat_tab_helper_) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(
      active_chat_tab_helper_->GetVisibleConversationHistory());
}

void AIChatUIPageHandler::GetSuggestedQuestions(
    GetSuggestedQuestionsCallback callback) {
  if (!active_chat_tab_helper_) {
    std::move(callback).Run({}, mojom::SuggestionGenerationStatus::None);
    return;
  }
  mojom::SuggestionGenerationStatus suggestion_status;
  std::move(callback).Run(
      active_chat_tab_helper_->GetSuggestedQuestions(suggestion_status),
      suggestion_status);
}

void AIChatUIPageHandler::GenerateQuestions() {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->GenerateQuestions();
  }
}

void AIChatUIPageHandler::GetSiteInfo(GetSiteInfoCallback callback) {
  if (!active_chat_tab_helper_) {
    VLOG(2) << "Chat tab helper is not set";
    std::move(callback).Run({});
    return;
  }

  auto site_info = active_chat_tab_helper_->BuildSiteInfo();
  std::move(callback).Run(site_info.Clone());
}

void AIChatUIPageHandler::OpenBraveLeoSettings() {
  auto* contents_to_navigate = (active_chat_tab_helper_)
                                   ? active_chat_tab_helper_->web_contents()
                                   : web_contents();
#if !BUILDFLAG(IS_ANDROID)
  const GURL url("brave://settings/leo-assistant");
  if (auto* browser = chrome::FindBrowserWithTab(contents_to_navigate)) {
    ShowSingletonTab(browser, url);
  } else {
    contents_to_navigate->OpenURL(
        {url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
         ui::PAGE_TRANSITION_LINK, false},
        /*navigation_handle_callback=*/{});
  }
#else
  ai_chat::ShowBraveLeoSettings(contents_to_navigate);
#endif
}

void AIChatUIPageHandler::OpenURL(const GURL& url) {
  if (!url.SchemeIs(content::kChromeUIScheme) &&
      !url.SchemeIs(url::kHttpsScheme)) {
    return;
  }

#if !BUILDFLAG(IS_ANDROID)
  auto* contents_to_navigate = (active_chat_tab_helper_)
                                   ? active_chat_tab_helper_->web_contents()
                                   : web_contents();
  contents_to_navigate->OpenURL(
      {url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
       ui::PAGE_TRANSITION_LINK, false},
      /*navigation_handle_callback=*/{});
#else
  // We handle open link different on Android as we need to close the chat
  // window because it's always full screen
  ai_chat::OpenURL(url.spec());
#endif
}

void AIChatUIPageHandler::GoPremium() {
#if !BUILDFLAG(IS_ANDROID)
  OpenURL(GURL(kURLGoPremium));
#else
  auto* contents_to_navigate = (active_chat_tab_helper_)
                                   ? active_chat_tab_helper_->web_contents()
                                   : web_contents();
  ai_chat::GoPremium(contents_to_navigate);
#endif
}

void AIChatUIPageHandler::RefreshPremiumSession() {
  OpenURL(GURL(kURLRefreshPremiumSession));
}

void AIChatUIPageHandler::ManagePremium() {
#if !BUILDFLAG(IS_ANDROID)
  OpenURL(GURL(kURLManagePremium));
#else
  auto* contents_to_navigate = (active_chat_tab_helper_)
                                   ? active_chat_tab_helper_->web_contents()
                                   : web_contents();
  ai_chat::ManagePremium(contents_to_navigate);
#endif
}

void AIChatUIPageHandler::OpenLearnMoreAboutBraveSearchWithLeo() {
  OpenURL(GURL(kURLLearnMoreBraveSearchLeo));
}

void AIChatUIPageHandler::SetShouldSendPageContents(bool should_send) {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->SetShouldSendPageContents(should_send);
  }
}

void AIChatUIPageHandler::GetShouldSendPageContents(
    GetShouldSendPageContentsCallback callback) {
  if (active_chat_tab_helper_) {
    std::move(callback).Run(
        active_chat_tab_helper_->GetShouldSendPageContents());
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
  if (!active_chat_tab_helper_) {
    std::move(callback).Run(mojom::APIError::None);
    return;
  }
  std::move(callback).Run(active_chat_tab_helper_->GetCurrentAPIError());
}

void AIChatUIPageHandler::ClearErrorAndGetFailedMessage(
    ClearErrorAndGetFailedMessageCallback callback) {
  mojom::ConversationTurnPtr failed_turn =
      active_chat_tab_helper_->ClearErrorAndGetFailedMessage();
  std::move(callback).Run(std::move(failed_turn));
}

void AIChatUIPageHandler::GetCanShowPremiumPrompt(
    GetCanShowPremiumPromptCallback callback) {
  std::move(callback).Run(active_chat_tab_helper_->GetCanShowPremium());
}

void AIChatUIPageHandler::DismissPremiumPrompt() {
  active_chat_tab_helper_->DismissPremiumPrompt();
}

void AIChatUIPageHandler::RateMessage(bool is_liked,
                                      uint32_t turn_id,
                                      RateMessageCallback callback) {
  active_chat_tab_helper_->RateMessage(is_liked, turn_id, std::move(callback));
}

void AIChatUIPageHandler::SendFeedback(const std::string& category,
                                       const std::string& feedback,
                                       const std::string& rating_id,
                                       bool send_hostname,
                                       SendFeedbackCallback callback) {
  active_chat_tab_helper_->SendFeedback(category, feedback, rating_id,
                                        send_hostname, std::move(callback));
}

void AIChatUIPageHandler::MarkAgreementAccepted() {
  active_chat_tab_helper_->SetUserOptedIn(true);
}

void AIChatUIPageHandler::ChatContextObserver::WebContentsDestroyed() {
  page_handler_->HandleWebContentsDestroyed();
}

void AIChatUIPageHandler::HandleWebContentsDestroyed() {
  chat_tab_helper_observation_.Reset();
  chat_context_observer_.reset();
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

void AIChatUIPageHandler::OnModelDataChanged(
    const std::string& model_key,
    const std::vector<mojom::ModelPtr>& model_list) {
  if (page_.is_bound()) {
    std::vector<mojom::ModelPtr> models_copy(model_list.size());
    std::transform(model_list.cbegin(), model_list.cend(), models_copy.begin(),
                   [](auto& model) { return model.Clone(); });

    page_->OnModelDataChanged(model_key, std::move(models_copy));
  }
}

void AIChatUIPageHandler::OnSuggestedQuestionsChanged(
    std::vector<std::string> questions,
    mojom::SuggestionGenerationStatus suggestion_generation_status) {
  if (page_.is_bound()) {
    page_->OnSuggestedQuestionsChanged(std::move(questions),
                                       suggestion_generation_status);
  }
}

void AIChatUIPageHandler::OnFaviconImageDataChanged() {
  if (page_.is_bound()) {
    auto on_favicon_data =
        [](base::SafeRef<AIChatUIPageHandler> page_handler,
           const std::optional<std::vector<uint8_t>>& bytes) {
          if (bytes.has_value()) {
            page_handler->page_->OnFaviconImageDataChanged(bytes.value());
          }
        };

    GetFaviconImageData(
        base::BindOnce(on_favicon_data, weak_ptr_factory_.GetSafeRef()));
  }
}

void AIChatUIPageHandler::OnPageHasContent(mojom::SiteInfoPtr site_info) {
  if (page_.is_bound()) {
    page_->OnSiteInfoChanged(std::move(site_info));
  }
}

void AIChatUIPageHandler::OnPrintPreviewRequested(bool is_pdf) {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  print_preview_extractor_ = std::make_unique<PrintPreviewExtractor>(
      active_chat_tab_helper_->web_contents(), profile_, is_pdf);
  print_preview_extractor_->CreatePrintPreview();
#endif
}

void AIChatUIPageHandler::GetFaviconImageData(
    GetFaviconImageDataCallback callback) {
  if (!active_chat_tab_helper_) {
    std::move(callback).Run(std::nullopt);
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
          std::move(callback).Run(std::nullopt);
          return;
        }

        std::vector<uint8_t> bytes(result.bitmap_data->begin(),
                                   result.bitmap_data->end());
        std::move(callback).Run(std::move(bytes));
      };

  favicon_service_->GetRawFaviconForPageURL(
      active_page_url, icon_types, kDesiredFaviconSizePixels, true,
      base::BindOnce(on_favicon_available, std::move(callback)),
      &favicon_task_tracker_);
}

void AIChatUIPageHandler::OnVisibilityChanged(content::Visibility visibility) {
  // WebUI visibility changed (not target tab)
  if (!active_chat_tab_helper_) {
    return;
  }
  bool is_visible = (visibility == content::Visibility::VISIBLE) ? true : false;
  active_chat_tab_helper_->OnConversationActiveChanged(is_visible);
}

void AIChatUIPageHandler::GetPremiumStatus(GetPremiumStatusCallback callback) {
  if (!active_chat_tab_helper_) {
    VLOG(2) << "Chat tab helper is not set";
    std::move(callback).Run(mojom::PremiumStatus::Inactive, nullptr);
    return;
  }

  // Don't pass |callback| directly to tab helper because this PageHandler
  // binding could be closed before running it.
  active_chat_tab_helper_->GetPremiumStatus(
      base::BindOnce(&AIChatUIPageHandler::OnGetPremiumStatus,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatUIPageHandler::ClosePanel() {
#if !BUILDFLAG(IS_ANDROID)
  ai_chat::ClosePanel(web_contents());
#else
  ai_chat::CloseActivity(web_contents());
#endif
}

void AIChatUIPageHandler::GetActionMenuList(
    GetActionMenuListCallback callback) {
  std::move(callback).Run(ai_chat::GetActionMenuList());
}

void AIChatUIPageHandler::OpenModelSupportUrl() {
  OpenURL(GURL(kLeoModelSupportUrl));
}

void AIChatUIPageHandler::OnGetPremiumStatus(
    GetPremiumStatusCallback callback,
    ai_chat::mojom::PremiumStatus status,
    ai_chat::mojom::PremiumInfoPtr info) {
  if (page_.is_bound()) {
#if BUILDFLAG(IS_ANDROID)
    // There is no UI for android to "refresh" with an iAP - we are likely still
    // authenticating after first iAP, so we should show as active.
    if (status == mojom::PremiumStatus::ActiveDisconnected &&
        profile_->GetPrefs()->GetBoolean(
            prefs::kBraveChatSubscriptionActiveAndroid)) {
      status = mojom::PremiumStatus::Active;
    }
#endif
    std::move(callback).Run(status, std::move(info));
  }
}

void AIChatUIPageHandler::ModifyConversation(uint32_t turn_index,
                                             const std::string& new_text) {
  if (active_chat_tab_helper_) {
    active_chat_tab_helper_->ModifyConversation(turn_index, new_text);
  }
}

}  // namespace ai_chat
