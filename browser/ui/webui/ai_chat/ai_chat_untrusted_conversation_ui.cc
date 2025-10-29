// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "base/strings/escape.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/browser/ui/webui/untrusted_sanitized_image_source.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/untrusted_frame.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/webui/webui_util.h"
#include "url/url_constants.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/android/ai_chat/brave_leo_settings_launcher_helper.h"
#else
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/thumbnails/thumbnail_tracker.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/browser/ui/webui/util/image_util.h"
#endif

namespace {

// Implments the interface to calls from the UI to the browser
class UIHandler : public ai_chat::mojom::UntrustedUIHandler {
 public:
  UIHandler(content::WebUI* web_ui,
            mojo::PendingReceiver<ai_chat::mojom::UntrustedUIHandler> receiver)
      :
#if !BUILDFLAG(IS_ANDROID)  // Match thumnbail_tracker.h GN guard
        thumbnail_tracker_(base::BindRepeating(&UIHandler::ThumbnailUpdated,
                                               base::Unretained(this))),
#endif
        web_ui_(web_ui),

        receiver_(this, std::move(receiver)) {
    // Set up pref change observer for memory changes
    PrefService* prefs = Profile::FromWebUI(web_ui_)->GetPrefs();
    pref_change_registrar_.Init(prefs);
    pref_change_registrar_.Add(
        ai_chat::prefs::kBraveAIChatUserMemories,
        base::BindRepeating(&UIHandler::OnMemoriesChanged,
                            // It's safe because we own pref_change_registrar_.
                            base::Unretained(this)));
  }

  UIHandler(const UIHandler&) = delete;
  UIHandler& operator=(const UIHandler&) = delete;

  ~UIHandler() override = default;

  // ai_chat::mojom::UntrustedConversationUIHandler
  void OpenLearnMoreAboutBraveSearchWithLeo() override {
    if (!web_ui_->GetRenderFrameHost()->HasTransientUserActivation()) {
      return;
    }
    OpenURL(GURL(ai_chat::kLeoBraveSearchSupportUrl));
  }

  void OpenSearchURL(const std::string& search_query) override {
    if (!web_ui_->GetRenderFrameHost()->HasTransientUserActivation()) {
      return;
    }
    OpenURL(GURL("https://search.brave.com/search?q=" +
                 base::EscapeQueryParamValue(search_query, true)));
  }

  void OpenURLFromResponse(const GURL& url) override {
    if (!web_ui_->GetRenderFrameHost()->HasTransientUserActivation()) {
      return;
    }
    if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
      return;
    }
    OpenURL(url);
  }

  void AddTabToThumbnailTracker(int32_t tab_id) override {
#if !BUILDFLAG(IS_ANDROID)  // Match thumnbail_tracker.h GN guard
    // Security check: only allow if the conversation has the tab as a task
    if (conversation_id_.empty()) {
      DVLOG(0) << "Cannot add tab to thumbnail tracker for empty conversation";
      return;
    }

    ai_chat::AIChatService* service =
        ai_chat::AIChatServiceFactory::GetForBrowserContext(
            web_ui_->GetWebContents()->GetBrowserContext());
    // Service should be available if WebUI class is instantiated
    CHECK(service);

    // We can use the sync version of GetConversation as the conversation must
    // be in-memory for the UI to be displayed.
    ai_chat::ConversationHandler* conversation =
        service->GetConversation(conversation_id_);
    if (!conversation) {
      // Technically it's possible for a conversation to be unloaded whilst the
      // UI is still open. This can especially happen if the conversation is
      // deleted (or all data chosen to be cleared). At that point, the UI
      // shouldn't be asking to track new tab thumbnails, but we don't need
      // to crash.
      DVLOG(0) << __func__ << " Conversation not found for conversation id: "
               << conversation_id_;
      return;
    }

    if (!conversation->get_task_tab_ids().contains(tab_id)) {
      DVLOG(0) << __func__ << " Tab id: " << tab_id
               << " is not a task for conversation: " << conversation_id_;
      return;
    }

    auto* tab_handle = tabs::TabHandle(tab_id).Get();
    if (!tab_handle) {
      DVLOG(0) << __func__
               << " Failed to get tab handle for tab id: " << tab_id;
      return;
    }
    thumbnail_tracker_.AddTab(tab_handle->GetContents());
#endif
  }

  void RemoveTabFromThumbnailTracker(int32_t tab_id) override {
#if !BUILDFLAG(IS_ANDROID)  // Match thumnbail_tracker.h GN guard
    auto* tab_handle = tabs::TabHandle(tab_id).Get();
    if (!tab_handle) {
      DVLOG(0) << __func__
               << " Failed to get tab handle for tab id: " << tab_id;
      return;
    }
    thumbnail_tracker_.RemoveTab(tab_handle->GetContents());
#endif
  }

  void BindParentPage(mojo::PendingReceiver<ai_chat::mojom::ParentUIFrame>
                          parent_ui_frame_receiver) override {
    // Route the receiver to the parent frame
    auto* rfh = web_ui_->GetWebContents()->GetPrimaryMainFrame();
    if (!rfh) {
      return;
    }

    // We should not be embedded on a non-WebUI page
    CHECK(rfh->GetWebUI());

    AIChatUI* ai_chat_ui_controller =
        rfh->GetWebUI()->GetController()->GetAs<AIChatUI>();
    // We should not be embedded on any non AIChatUI page
    CHECK(ai_chat_ui_controller);

    ai_chat_ui_controller->BindInterface(std::move(parent_ui_frame_receiver));
  }

  void DeleteMemory(const std::string& memory) override {
    ai_chat::prefs::DeleteMemoryFromPrefs(
        memory, *Profile::FromWebUI(web_ui_)->GetPrefs());
  }

  void HasMemory(const std::string& memory,
                 HasMemoryCallback callback) override {
    std::move(callback).Run(ai_chat::prefs::HasMemoryFromPrefs(
        memory, *Profile::FromWebUI(web_ui_)->GetPrefs()));
  }

  void BindConversationHandler(
      const std::string& conversation_id,
      mojo::PendingReceiver<ai_chat::mojom::UntrustedConversationHandler>
          untrusted_conversation_handler_receiver) override {
    if (conversation_id.empty()) {
      return;
    }

    conversation_id_ = conversation_id;

    ai_chat::AIChatService* service =
        ai_chat::AIChatServiceFactory::GetForBrowserContext(
            web_ui_->GetWebContents()->GetBrowserContext());

    if (!service) {
      return;
    }

    service->GetConversation(
        conversation_id,
        base::BindOnce(
            [](mojo::PendingReceiver<
                   ai_chat::mojom::UntrustedConversationHandler> receiver,
               ai_chat::ConversationHandler* conversation_handler) {
              if (!conversation_handler) {
                DVLOG(0)
                    << "Failed to get conversation handler for conversation "
                       "entries frame";
                return;
              }
              conversation_handler->Bind(std::move(receiver));
            },
            std::move(untrusted_conversation_handler_receiver)));
  }

  void BindUntrustedUI(
      mojo::PendingRemote<ai_chat::mojom::UntrustedUI> untrusted_ui) override {
    untrusted_ui_.Bind(std::move(untrusted_ui));
  }

  void OpenAIChatCustomizationSettings() override {
#if !BUILDFLAG(IS_ANDROID)
    chrome::ShowSettingsSubPageForProfile(
        Profile::FromWebUI(web_ui_), ai_chat::kBraveAIChatCustomizationSubPage);
#else
    NOTIMPLEMENTED();
#endif
  }

 private:
  void OpenURL(GURL url) {
    if (!url.SchemeIs(url::kHttpsScheme)) {
      return;
    }

#if !BUILDFLAG(IS_ANDROID)
    Browser* browser =
        ai_chat::GetBrowserForWebContents(web_ui_->GetWebContents());
    browser->OpenURL(
        {url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
         ui::PAGE_TRANSITION_LINK, false},
        /*navigation_handle_callback=*/{});
#else
    // We handle open link different on Android as we need to close the chat
    // window because it's always full screen
    ai_chat::OpenURL(url.spec());
#endif
  }

  void OnMemoriesChanged() {
    if (!untrusted_ui_.is_bound()) {
      return;
    }

    // Get updated memories from prefs
    std::vector<std::string> memories = ai_chat::prefs::GetMemoriesFromPrefs(
        *Profile::FromWebUI(web_ui_)->GetPrefs());

    // Notify the UI
    untrusted_ui_->OnMemoriesChanged(memories);
  }

#if !BUILDFLAG(IS_ANDROID)  // Match thumnbail_tracker.h GN guard
  // Callback for ThumbnailTracker
  void ThumbnailUpdated(content::WebContents* contents,
                        ThumbnailTracker::CompressedThumbnailData image) {
    if (!image) {
      return;
    }
    auto tab_id =
        tabs::TabInterface::GetFromContents(contents)->GetHandle().raw_value();
    std::string data_uri;
    data_uri = webui::MakeDataURIForImage(base::span(image->data), "jpeg");
    untrusted_ui_->ThumbnailUpdated(tab_id, data_uri);
  }

  ThumbnailTracker thumbnail_tracker_;
#endif  // !BUILDFLAG(IS_ANDROID)

  raw_ptr<content::WebUI> web_ui_ = nullptr;

  mojo::Receiver<ai_chat::mojom::UntrustedUIHandler> receiver_;
  mojo::Remote<ai_chat::mojom::UntrustedUI> untrusted_ui_;
  PrefChangeRegistrar pref_change_registrar_;

  // ID of the conversation this UI is displaying. This frame page is single-use
  // for a single conversation.
  std::string conversation_id_;
};

}  // namespace

bool AIChatUntrustedConversationUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  // Only enabled if we have a valid service
  return (ai_chat::AIChatServiceFactory::GetForBrowserContext(
              browser_context) != nullptr);
}

std::unique_ptr<content::WebUIController>
AIChatUntrustedConversationUIConfig::CreateWebUIController(
    content::WebUI* web_ui,
    const GURL& url) {
  return std::make_unique<AIChatUntrustedConversationUI>(web_ui);
}

AIChatUntrustedConversationUIConfig::AIChatUntrustedConversationUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  kAIChatUntrustedConversationUIHost) {}

AIChatUntrustedConversationUIConfig::~AIChatUntrustedConversationUIConfig() =
    default;

AIChatUntrustedConversationUI::AIChatUntrustedConversationUI(
    content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  // Create a URLDataSource and add resources.
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(),
      kAIChatUntrustedConversationUIURL);
  webui::SetupWebUIDataSource(source, kAiChatUiGenerated,
                              IDR_AI_CHAT_UNTRUSTED_CONVERSATION_UI_HTML);

  source->AddLocalizedStrings(webui::kAiChatStrings);

  constexpr bool kIsMobile = BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS);
  source->AddBoolean("isMobile", kIsMobile);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources "
      "chrome-untrusted://theme;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' blob: data: chrome-untrusted://resources "
      "chrome-untrusted://image chrome-untrusted://favicon2;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      absl::StrFormat("frame-ancestors %s;", kAIChatUIURL));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types default;");

  Profile* profile = Profile::FromWebUI(web_ui);
  content::URLDataSource::Add(profile,
                              std::make_unique<FaviconSource>(
                                  profile, chrome::FaviconUrlFormat::kFavicon2,
                                  /*serve_untrusted=*/true));
  content::URLDataSource::Add(
      profile, std::make_unique<UntrustedSanitizedImageSource>(profile));
#if !BUILDFLAG(IS_ANDROID)
  content::URLDataSource::Add(profile, std::make_unique<ThemeSource>(
                                           profile, /*serve_untrusted=*/true));
#endif
}

AIChatUntrustedConversationUI::~AIChatUntrustedConversationUI() = default;

void AIChatUntrustedConversationUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::UntrustedUIHandler> receiver) {
  ui_handler_ = std::make_unique<UIHandler>(web_ui(), std::move(receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(AIChatUntrustedConversationUI)
