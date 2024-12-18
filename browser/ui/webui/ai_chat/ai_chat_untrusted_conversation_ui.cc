// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.h"

#include <string>
#include <utility>

#include "base/strings/escape.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/ai_chat_urls.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/untrusted_frame.mojom.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/android/ai_chat/brave_leo_settings_launcher_helper.h"
#else
#include "chrome/browser/ui/browser.h"
#endif

namespace {
constexpr char kURLLearnMoreBraveSearchLeo[] =
    "https://support.brave.com/hc/en-us/articles/"
    "27586048343309-How-does-Leo-get-current-information";

// Implments the interface to calls from the UI to the browser
class UIHandler : public ai_chat::mojom::UntrustedUIHandler {
 public:
  UIHandler(content::WebUI* web_ui,
            mojo::PendingReceiver<ai_chat::mojom::UntrustedUIHandler> receiver)
      : web_ui_(web_ui), receiver_(this, std::move(receiver)) {}
  UIHandler(const UIHandler&) = delete;
  UIHandler& operator=(const UIHandler&) = delete;

  ~UIHandler() override = default;

  // ai_chat::mojom::UntrustedConversationUIHandler
  void OpenLearnMoreAboutBraveSearchWithLeo() override {
    if (!web_ui_->GetRenderFrameHost()->HasTransientUserActivation()) {
      return;
    }
    OpenURL(GURL(kURLLearnMoreBraveSearchLeo));
  }

  void OpenSearchURL(const std::string& search_query) override {
    if (!web_ui_->GetRenderFrameHost()->HasTransientUserActivation()) {
      return;
    }
    OpenURL(GURL("https://search.brave.com/search?q=" +
                 base::EscapeQueryParamValue(search_query, true)));
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

  raw_ptr<content::WebUI> web_ui_ = nullptr;
  mojo::Receiver<ai_chat::mojom::UntrustedUIHandler> receiver_;
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

  for (const auto& str : ai_chat::GetLocalizedStrings()) {
    source->AddString(str.name,
                      brave_l10n::GetLocalizedResourceUTF16String(str.id));
  }

  constexpr bool kIsMobile = BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS);
  source->AddBoolean("isMobile", kIsMobile);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' blob: chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      base::StringPrintf("frame-ancestors %s;", kAIChatUIURL));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types default;");
}

AIChatUntrustedConversationUI::~AIChatUntrustedConversationUI() = default;

void AIChatUntrustedConversationUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::UntrustedUIHandler> receiver) {
  ui_handler_ = std::make_unique<UIHandler>(web_ui(), std::move(receiver));
}

void AIChatUntrustedConversationUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::UntrustedConversationHandler>
        receiver) {
  // Get conversation from URL
  std::string_view conversation_uuid = ai_chat::ConversationUUIDFromURL(
      web_ui()->GetRenderFrameHost()->GetLastCommittedURL());
  DVLOG(2) << "Binding conversation frame for conversation uuid:"
           << conversation_uuid;
  if (conversation_uuid.empty()) {
    return;
  }

  ai_chat::AIChatService* service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(
          web_ui()->GetWebContents()->GetBrowserContext());

  if (!service) {
    return;
  }

  service->GetConversation(
      conversation_uuid,
      base::BindOnce(
          [](mojo::PendingReceiver<ai_chat::mojom::UntrustedConversationHandler>
                 receiver,
             ai_chat::ConversationHandler* conversation_handler) {
            if (!conversation_handler) {
              DVLOG(0) << "Failed to get conversation handler for conversation "
                          "entries frame";
              return;
            }
            conversation_handler->Bind(std::move(receiver));
          },
          std::move(receiver)));
}

WEB_UI_CONTROLLER_TYPE_IMPL(AIChatUntrustedConversationUI)
