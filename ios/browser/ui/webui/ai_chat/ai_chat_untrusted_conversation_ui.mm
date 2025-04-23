// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.h"

#include <string>
#include <utility>

#include "base/strings/escape.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/untrusted_frame.mojom.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/ios/browser/ui/webui/favicon_source.h"
#include "brave/ios/browser/ui/webui/untrusted_sanitized_image_source.h"
#include "brave/ios/web/webui/brave_url_data_source_ios.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web/web_state/web_state_impl.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/webui/webui_util.h"
#include "url/url_constants.h"

namespace {
constexpr char kURLLearnMoreBraveSearchLeo[] =
    "https://support.brave.com/hc/en-us/articles/"
    "27586048343309-How-does-Leo-get-current-information";

// Implments the interface to calls from the UI to the browser
class UIHandler : public ai_chat::mojom::UntrustedUIHandler {
 public:
  UIHandler(web::WebUIIOS* web_ui,
            mojo::PendingReceiver<ai_chat::mojom::UntrustedUIHandler> receiver)
      : web_ui_(web_ui), receiver_(this, std::move(receiver)) {}
  UIHandler(const UIHandler&) = delete;
  UIHandler& operator=(const UIHandler&) = delete;

  ~UIHandler() override = default;

  // ai_chat::mojom::UntrustedConversationUIHandler
  void OpenLearnMoreAboutBraveSearchWithLeo() override {
    OpenURL(GURL(kURLLearnMoreBraveSearchLeo));
  }

  void OpenSearchURL(const std::string& search_query) override {
    OpenURL(GURL("https://search.brave.com/search?q=" +
                 base::EscapeQueryParamValue(search_query, true)));
  }

  void OpenURLFromResponse(const GURL& url) override {
    if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
      return;
    }
    OpenURL(url);
  }

  void BindParentPage(mojo::PendingReceiver<ai_chat::mojom::ParentUIFrame>
                          parent_ui_frame_receiver) override {
    auto* web_state = web_ui_->GetWebState();
    if (!web_state) {
      return;
    }

    // Route the receiver to the parent frame
    // We should not be embedded on a non-WebUI page
    auto* main_web_ui =
        static_cast<web::WebStateImpl*>(web_state)->GetMainWebUI();
    if (!main_web_ui) {
      return;
    }

    AIChatUI* ai_chat_ui_controller =
        static_cast<AIChatUI*>(main_web_ui->GetController());
    // We should not be embedded on any non AIChatUI page
    CHECK(ai_chat_ui_controller);

    ai_chat_ui_controller->BindInterfaceParentUIFrame(
        std::move(parent_ui_frame_receiver));
  }

 private:
  void OpenURL(GURL url) {
    if (!url.SchemeIs(url::kHttpsScheme)) {
      return;
    }

    web_ui_->GetWebState()->OpenURL({
        url,
        web::Referrer(),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui::PAGE_TRANSITION_LINK,
        false,
    });
  }

  raw_ptr<web::WebUIIOS> web_ui_ = nullptr;
  mojo::Receiver<ai_chat::mojom::UntrustedUIHandler> receiver_;
};

}  // namespace

AIChatUntrustedConversationUI::AIChatUntrustedConversationUI(
    web::WebUIIOS* web_ui,
    const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()) {
  // Create a URLDataSource and add resources.
  BraveWebUIIOSDataSource* source = brave::CreateAndAddWebUIDataSource(
      web_ui, url.host(), kAiChatUiGenerated,
      IDR_AI_CHAT_UNTRUSTED_CONVERSATION_UI_HTML);

  source->AddLocalizedStrings(webui::kAiChatStrings);

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
      "img-src 'self' blob: chrome-untrusted://resources "
      "chrome-untrusted://image;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      base::StringPrintf("frame-ancestors %s;", kAIChatUIURL));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types default;");

  //  ProfileIOS* profile = ProfileIOS::FromWebUIIOS(web_ui);

  // TODO: Fix
  //  content::URLDataSource::Add(
  //      profile, std::make_unique<UntrustedSanitizedImageSource>(profile));

  // Bind Mojom Interface
  web_ui->GetWebState()
      ->GetInterfaceBinderForMainFrame()
      ->AddUntrustedInterface(
          url,
          base::BindRepeating(
              &AIChatUntrustedConversationUI::BindInterfaceUntrustedUIHandler,
              base::Unretained(this)));

  web_ui->GetWebState()
      ->GetInterfaceBinderForMainFrame()
      ->AddUntrustedInterface(
          url,
          base::BindRepeating(&AIChatUntrustedConversationUI::
                                  BindInterfaceUntrustedConversationHandler,
                              base::Unretained(this)));
}

AIChatUntrustedConversationUI::~AIChatUntrustedConversationUI() {
  const auto url = GURL(base::StrCat(
      {kChromeUIUntrustedScheme, url::kStandardSchemeSeparator, GetHost()}));
  web_ui()
      ->GetWebState()
      ->GetInterfaceBinderForMainFrame()
      ->RemoveUntrustedInterface(
          url, ai_chat::mojom::UntrustedConversationHandler::Name_);
  web_ui()
      ->GetWebState()
      ->GetInterfaceBinderForMainFrame()
      ->RemoveUntrustedInterface(url,
                                 ai_chat::mojom::UntrustedUIHandler::Name_);
}

void AIChatUntrustedConversationUI::BindInterfaceUntrustedUIHandler(
    mojo::PendingReceiver<ai_chat::mojom::UntrustedUIHandler> receiver) {
  ui_handler_ = std::make_unique<UIHandler>(web_ui(), std::move(receiver));
}

void AIChatUntrustedConversationUI::BindInterfaceUntrustedConversationHandler(
    mojo::PendingReceiver<ai_chat::mojom::UntrustedConversationHandler>
        receiver) {
  // Get conversation from URL
  std::string_view conversation_uuid = ai_chat::ConversationUUIDFromURL(
      web_ui()->GetWebState()->GetLastCommittedURL());
  DVLOG(2) << "Binding conversation frame for conversation uuid:"
           << conversation_uuid;
  if (conversation_uuid.empty()) {
    return;
  }

  ai_chat::AIChatService* service =
      ai_chat::AIChatServiceFactory::GetForProfile(
          ProfileIOS::FromWebUIIOS(web_ui()));

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
