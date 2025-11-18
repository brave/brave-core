// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/brave_tab_helpers.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/components/ai_chat/content/browser/associated_url_content.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "components/favicon/core/favicon_service.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/url_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/notimplemented.h"
#include "brave/browser/ui/android/ai_chat/brave_leo_settings_launcher_helper.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#else
#include "chrome/browser/ui/chrome_pages.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#endif

namespace {
#if !BUILDFLAG(IS_ANDROID)
constexpr char kURLManagePremium[] = "https://account.brave.com/";
#endif
}  // namespace

namespace ai_chat {

namespace {

// Invokes a callback when the WebContents has finished loading. Note: If the
// WebContents is destroyed before loading is completed, the callback will not
// be invoked.
// The lifetime of this class is tied to the WebContents it is observing - it
// will be destroyed when |WebContentsDestroyed| is called, or when the
// Navigation finishes, whichever happens first.
class WaitForCommit : public content::WebContentsObserver {
 public:
  WaitForCommit(
      content::WebContents* contents,
      base::OnceCallback<void(content::WebContents* contents)> on_loaded)
      : WebContentsObserver(contents), on_loaded_(std::move(on_loaded)) {}
  ~WaitForCommit() override = default;

  void DidFinishNavigation(content::NavigationHandle* handle) override {
    if (handle->IsInMainFrame() && handle->HasCommitted()) {
      std::move(on_loaded_).Run(web_contents());
      delete this;
    }
  }

  void WebContentsDestroyed() override { delete this; }

 private:
  base::OnceCallback<void(content::WebContents* contents)> on_loaded_;
};

// Note: we need to ensure the WebContents is loaded before associating content
// with a conversation.
void EnsureWebContentsLoaded(
    content::WebContents* contents,
    base::OnceCallback<void(content::WebContents* contents)> on_loaded) {
  if (!contents->GetController().NeedsReload()) {
    std::move(on_loaded).Run(contents);
    return;
  }

  // Deletes when the load completes or the WebContents is destroyed
  new WaitForCommit(contents, std::move(on_loaded));
  contents->GetController().LoadIfNecessary();
}

content::WebContents* GetWebContentsFromTabId(int32_t tab_id) {
#if BUILDFLAG(IS_ANDROID)
  TabAndroid* tab = nullptr;
  for (TabModel* model : TabModelList::models()) {
    const size_t tab_count = model->GetTabCount();
    for (size_t index = 0; index < tab_count; index++) {
      auto* current_tab = model->GetTabAt(index);
      if (tab_id == current_tab->GetAndroidId()) {
        tab = current_tab;
        break;
      }
    }
  }
  if (!tab) {
    return nullptr;
  }

  auto* contents = tab->web_contents();
#else
  auto* tab = tabs::TabInterface::Handle(tab_id).Get();
  if (!tab) {
    return nullptr;
  }
  auto* contents = tab->GetContents();
#endif
  return contents;
}

}  // namespace

using mojom::CharacterType;
using mojom::ConversationTurn;

AIChatUIPageHandler::ChatContextObserver::ChatContextObserver(
    content::WebContents* web_contents,
    AIChatUIPageHandler& page_handler)
    : content::WebContentsObserver(web_contents), page_handler_(page_handler) {}

AIChatUIPageHandler::ChatContextObserver::~ChatContextObserver() = default;

AIChatUIPageHandler::AIChatUIPageHandler(
    content::WebContents* owner_web_contents,
    content::WebContents* chat_context_web_contents,
    Profile* profile,
    mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver)
    : owner_web_contents_(owner_web_contents),
      profile_(profile),
      receiver_(this, std::move(receiver)),
      conversations_are_content_associated_(
          !profile_->IsAIChatAgent() &&
          !features::IsAIChatGlobalSidePanelEverywhereEnabled()) {
  // Standalone mode means Chat is opened as its own tab in the tab strip and
  // not a side panel. chat_context_web_contents is nullptr in that case
  const bool is_standalone = chat_context_web_contents == nullptr;
  auto* profile_metrics =
      misc_metrics::ProfileMiscMetricsServiceFactory::GetServiceForContext(
          profile);
  if (profile_metrics) {
    ai_chat_metrics_ = profile_metrics->GetAIChatMetrics();
  }
  if (!is_standalone) {
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
    if (ai_chat_metrics_) {
      ai_chat_metrics_->RecordSidebarUsage();
    }
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
    active_chat_tab_helper_ =
        ai_chat::AIChatTabHelper::FromWebContents(chat_context_web_contents);

    associated_content_delegate_observation_.Observe(
        &active_chat_tab_helper_->web_contents_content());
    chat_context_observer_ =
        std::make_unique<ChatContextObserver>(chat_context_web_contents, *this);
  }
}

AIChatUIPageHandler::~AIChatUIPageHandler() = default;

void AIChatUIPageHandler::HandleVoiceRecognition(
    const std::string& conversation_uuid) {
#if BUILDFLAG(IS_ANDROID)
  ai_chat::HandleVoiceRecognition(owner_web_contents_.get(), conversation_uuid);
#endif
}

void AIChatUIPageHandler::ShowSoftKeyboard() {
#if BUILDFLAG(IS_ANDROID)
  ai_chat::HandleShowSoftKeyboard(owner_web_contents_.get());
#endif
}

void AIChatUIPageHandler::UploadFile(bool use_media_capture,
                                     UploadFileCallback callback) {
  if (!upload_file_helper_) {
    upload_file_helper_ =
        std::make_unique<UploadFileHelper>(owner_web_contents_, profile_);
    upload_file_helper_observation_.Observe(upload_file_helper_.get());
  }
  upload_file_helper_->UploadFile(
      std::make_unique<ChromeSelectFilePolicy>(owner_web_contents_),
#if BUILDFLAG(IS_ANDROID)
      use_media_capture,
#endif
      std::move(callback));
}

void AIChatUIPageHandler::ProcessImageFile(
    const std::vector<uint8_t>& file_data,
    const std::string& filename,
    ProcessImageFileCallback callback) {
  UploadFileHelper::ProcessImageData(
      &data_decoder_, file_data,
      base::BindOnce(
          [](const std::string& filename, ProcessImageFileCallback callback,
             std::optional<std::vector<uint8_t>> processed_data) {
            if (!processed_data) {
              std::move(callback).Run(nullptr);
              return;
            }
            auto uploaded_file = ai_chat::mojom::UploadedFile::New(
                filename, processed_data->size(), *processed_data,
                ai_chat::mojom::UploadedFileType::kImage);
            std::move(callback).Run(std::move(uploaded_file));
          },
          filename, std::move(callback)));
}

void AIChatUIPageHandler::GetPluralString(const std::string& key,
                                          int32_t count,
                                          GetPluralStringCallback callback) {
  auto iter = std::ranges::find(webui::kAiChatStrings, key,
                                &webui::LocalizedString::name);
  CHECK(iter != webui::kAiChatStrings.end());
  std::move(callback).Run(l10n_util::GetPluralStringFUTF8(iter->id, count));
}

void AIChatUIPageHandler::OpenAIChatSettings() {
#if !BUILDFLAG(IS_ANDROID)
  const GURL url(kAIChatSettingsURL);
  ShowSingletonTab(profile_, url);
#else
  ai_chat::ShowBraveLeoSettings(owner_web_contents_.get());
#endif
}

void AIChatUIPageHandler::OpenMemorySettings() {
#if !BUILDFLAG(IS_ANDROID)
  chrome::ShowSettingsSubPageForProfile(
      profile_, ai_chat::kBraveAIChatCustomizationSubPage);
#else
  NOTIMPLEMENTED();
#endif
}

void AIChatUIPageHandler::OpenConversationFullPage(
    const std::string& conversation_uuid) {
  CHECK(ai_chat::features::IsAIChatHistoryEnabled());
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  if (ai_chat_metrics_) {
    ai_chat_metrics_->RecordFullPageSwitch();
  }
#endif
  NavigateParams params(profile_, ConversationUrl(conversation_uuid),
                        ui::PAGE_TRANSITION_TYPED);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.referrer = content::Referrer();
  Navigate(&params);
}

void AIChatUIPageHandler::OpenAIChatAgentProfile() {
  CHECK(ai_chat::features::IsAIChatAgentProfileEnabled());
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
  ai_chat::OpenBrowserWindowForAIChatAgentProfile(*profile_);
#endif
}

void AIChatUIPageHandler::OpenURL(const GURL& url) {
  if (!url.SchemeIs(content::kChromeUIScheme) &&
      !url.SchemeIs(url::kHttpsScheme)) {
    return;
  }

  NavigateParams params(profile_, url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.referrer = content::Referrer();
  Navigate(&params);
}

void AIChatUIPageHandler::OpenStorageSupportUrl() {
  OpenURL(GURL(kLeoStorageSupportUrl));
}

void AIChatUIPageHandler::GoPremium() {
#if !BUILDFLAG(IS_ANDROID)
  OpenURL(GURL(kLeoGoPremiumUrl));
#else
  ai_chat::GoPremium(owner_web_contents_.get());
#endif
}

void AIChatUIPageHandler::RefreshPremiumSession() {
  OpenURL(GURL(kLeoRefreshPremiumSessionUrl));
}

void AIChatUIPageHandler::ManagePremium() {
#if !BUILDFLAG(IS_ANDROID)
  OpenURL(GURL(kURLManagePremium));
#else
  ai_chat::ManagePremium(owner_web_contents_.get());
#endif
}

void AIChatUIPageHandler::OpenModelSupportUrl() {
  OpenURL(GURL(kLeoModelSupportUrl));
}

void AIChatUIPageHandler::ChatContextObserver::WebContentsDestroyed() {
  page_handler_->HandleWebContentsDestroyed();
}

void AIChatUIPageHandler::HandleWebContentsDestroyed() {
  active_chat_tab_helper_ = nullptr;
  associated_content_delegate_observation_.Reset();
  chat_context_observer_.reset();
}

void AIChatUIPageHandler::OnRequestArchive(
    AssociatedContentDelegate* delegate) {
  // This is only applicable to content-adjacent UI, e.g. SidePanel on Desktop
  // where it would like to remain associated with the Tab and move away from
  // Conversations of previous navigations. That doens't apply to the standalone
  // UI where it will keep a previous navigation's conversation active.

  chat_ui_->OnNewDefaultConversation(
      active_chat_tab_helper_
          ? std::make_optional(
                active_chat_tab_helper_->web_contents_content().content_id())
          : std::nullopt);
}

void AIChatUIPageHandler::OnFilesSelected() {
  chat_ui_->OnUploadFilesSelected();
}

void AIChatUIPageHandler::CloseUI() {
#if !BUILDFLAG(IS_ANDROID)
  ai_chat::ClosePanel(owner_web_contents_);
#else
  ai_chat::CloseActivity(owner_web_contents_);
#endif
}

void AIChatUIPageHandler::SetChatUI(mojo::PendingRemote<mojom::ChatUI> chat_ui,
                                    SetChatUICallback callback) {
  chat_ui_.Bind(std::move(chat_ui));
  std::move(callback).Run(active_chat_tab_helper_ == nullptr);

  chat_ui_->OnNewDefaultConversation(
      active_chat_tab_helper_
          ? std::make_optional(
                active_chat_tab_helper_->web_contents_content().content_id())
          : std::nullopt);
}

void AIChatUIPageHandler::BindRelatedConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  // For global panel, don't recall conversations by their associated tab
  if (!active_chat_tab_helper_ || !conversations_are_content_associated_) {
    ConversationHandler* conversation =
        AIChatServiceFactory::GetForBrowserContext(profile_)
            ->CreateConversation();
    conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
    return;
  }

  ConversationHandler* conversation =
      AIChatServiceFactory::GetForBrowserContext(profile_)
          ->GetOrCreateConversationHandlerForContent(
              active_chat_tab_helper_->web_contents_content().content_id(),
              active_chat_tab_helper_->web_contents_content().GetWeakPtr());

  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::AssociateTab(mojom::TabDataPtr mojom_tab,
                                       const std::string& conversation_uuid) {
  auto* contents = GetWebContentsFromTabId(mojom_tab->id);
  if (!contents) {
    return;
  }

  EnsureWebContentsLoaded(
      contents, base::BindOnce(
                    [](const std::string& conversation_uuid,
                       content::WebContents* contents) {
                      auto* tab_helper =
                          ai_chat::AIChatTabHelper::FromWebContents(contents);
                      if (!tab_helper) {
                        return;
                      }

                      AIChatServiceFactory::GetForBrowserContext(
                          contents->GetBrowserContext())
                          ->MaybeAssociateContent(
                              &tab_helper->web_contents_content(),
                              conversation_uuid);
                    },
                    conversation_uuid));
}

void AIChatUIPageHandler::AssociateUrlContent(
    const GURL& url,
    const std::string& title,
    const std::string& conversation_uuid) {
  auto* context = owner_web_contents_->GetBrowserContext();
  auto* service = AIChatServiceFactory::GetForBrowserContext(context);
  auto content = std::make_unique<ai_chat::AssociatedURLContent>(
      url, base::UTF8ToUTF16(title), context,
      base::BindOnce(&brave::AttachPrivacySensitiveTabHelpers));
  service->AssociateOwnedContent(std::move(content), conversation_uuid);
}

void AIChatUIPageHandler::DisassociateContent(
    mojom::AssociatedContentPtr content,
    const std::string& conversation_uuid) {
  auto* service = AIChatServiceFactory::GetForBrowserContext(profile_);
  service->DisassociateContent(content, conversation_uuid);
}

void AIChatUIPageHandler::NewConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  ConversationHandler* conversation;
  // For standalone or global panel, don't recall conversations by their
  // associated tab.
  if (active_chat_tab_helper_ && conversations_are_content_associated_) {
    conversation =
        AIChatServiceFactory::GetForBrowserContext(profile_)
            ->CreateConversationHandlerForContent(
                active_chat_tab_helper_->web_contents_content().content_id(),
                active_chat_tab_helper_->web_contents_content().GetWeakPtr());
  } else {
    conversation = AIChatServiceFactory::GetForBrowserContext(profile_)
                       ->CreateConversation();
  }

  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::BindParentUIFrameFromChildFrame(
    mojo::PendingReceiver<mojom::ParentUIFrame> receiver) {
  chat_ui_->OnChildFrameBound(std::move(receiver));
}

}  // namespace ai_chat
