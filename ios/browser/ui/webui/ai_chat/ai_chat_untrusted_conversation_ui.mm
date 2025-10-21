// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.h"

#include <string>
#include <utility>

#include "base/notimplemented.h"
#include "base/strings/escape.h"
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
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/ios/browser/ui/webui/favicon_source.h"
#include "brave/ios/browser/ui/webui/untrusted_sanitized_image_source.h"
#include "brave/ios/web/webui/brave_url_data_source_ios.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web/web_state/web_state_impl.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/url_constants.h"

namespace {

// Implments the interface to calls from the UI to the browser
class UIHandler : public ai_chat::mojom::UntrustedUIHandler {
 public:
  UIHandler(web::WebUIIOS* web_ui,
            mojo::PendingReceiver<ai_chat::mojom::UntrustedUIHandler> receiver)
      : web_ui_(web_ui), receiver_(this, std::move(receiver)) {
    // Set up pref change observer for memory changes
    PrefService* prefs = ProfileIOS::FromWebUIIOS(web_ui_)->GetPrefs();
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
    OpenURL(GURL(ai_chat::kLeoBraveSearchSupportUrl));
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

  void AddTabToThumbnailTracker(int32_t tab_id) override {}

  void RemoveTabFromThumbnailTracker(int32_t tab_id) override {}

  void BindParentPage(mojo::PendingReceiver<ai_chat::mojom::ParentUIFrame>
                          parent_ui_frame_receiver) override {
    // Route the receiver to the parent frame
    // We should not be embedded on a non-WebUI page
    auto* main_web_ui = web::WebStateImpl::FromWebState(web_ui_->GetWebState())
                            ->GetMainFrameWebUI();
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

  void DeleteMemory(const std::string& memory) override {
    ai_chat::prefs::DeleteMemoryFromPrefs(
        memory, *ProfileIOS::FromWebUIIOS(web_ui_)->GetPrefs());
  }

  void HasMemory(const std::string& memory,
                 HasMemoryCallback callback) override {
    std::move(callback).Run(ai_chat::prefs::HasMemoryFromPrefs(
        memory, *ProfileIOS::FromWebUIIOS(web_ui_)->GetPrefs()));
  }

  void BindConversationHandler(
      const std::string& conversation_id,
      mojo::PendingReceiver<ai_chat::mojom::UntrustedConversationHandler>
          untrusted_conversation_handler_receiver) override {
    if (conversation_id.empty()) {
      return;
    }

    ai_chat::AIChatService* service =
        ai_chat::AIChatServiceFactory::GetForProfile(
            ProfileIOS::FromWebUIIOS(web_ui_));

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

  void OpenAIChatCustomizationSettings() override { NOTIMPLEMENTED(); }

 private:
  void OpenURL(GURL url) {
    if (!url.SchemeIs(url::kHttpsScheme)) {
      return;
    }

    // iOS is full-screen. Maybe need to call the WebUI Delegate to open the URL
    web_ui_->GetWebState()->OpenURL({
        url,
        web::Referrer(),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui::PAGE_TRANSITION_LINK,
        false,
    });
  }

  void OnMemoriesChanged() {
    if (!untrusted_ui_.is_bound()) {
      return;
    }

    // Get updated memories from prefs
    std::vector<std::string> memories = ai_chat::prefs::GetMemoriesFromPrefs(
        *ProfileIOS::FromWebUIIOS(web_ui_)->GetPrefs());

    // Notify the UI
    untrusted_ui_->OnMemoriesChanged(memories);
  }

  raw_ptr<web::WebUIIOS> web_ui_ = nullptr;
  mojo::Receiver<ai_chat::mojom::UntrustedUIHandler> receiver_;
  mojo::Remote<ai_chat::mojom::UntrustedUI> untrusted_ui_;
  PrefChangeRegistrar pref_change_registrar_;
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

  source->AddBoolean("isMobile", true);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' blob: chrome-untrusted://resources "
      "chrome-untrusted://image chrome-untrusted://favicon2;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      absl::StrFormat("frame-ancestors %s;", kAIChatUIURL));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types default;");

  ProfileIOS* profile = ProfileIOS::FromWebUIIOS(web_ui);

  web::URLDataSourceIOS::Add(
      profile, new FaviconSource(profile, chrome::FaviconUrlFormat::kFavicon2,
                                 /*serve_untrusted=*/true));
  web::URLDataSourceIOS::Add(profile,
                             new UntrustedSanitizedImageSource(profile));

  // Bind Mojom Interface
  web_ui->GetWebState()
      ->GetInterfaceBinderForMainFrame()
      ->AddUntrustedInterface(
          url,
          base::BindRepeating(
              &AIChatUntrustedConversationUI::BindInterfaceUntrustedUIHandler,
              base::Unretained(this)));
}

AIChatUntrustedConversationUI::~AIChatUntrustedConversationUI() {
  const auto url = GURL(base::StrCat(
      {kChromeUIUntrustedScheme, url::kStandardSchemeSeparator, GetHost()}));
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
