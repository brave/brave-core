// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/bookmarks_page_handler.h"
#include "brave/components/ai_chat/core/browser/history_ui_handler.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/bookmarks.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/history.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/untrusted_frame.mojom.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge_holder.h"
#include "brave/ios/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/ios/browser/api/web_view/brave_web_view.h"
#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/ios/browser/ui/webui/favicon_source.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web_view/internal/cwv_web_view_internal.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

AIChatUI::AIChatUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.GetHost()),
      profile_(ProfileIOS::FromWebUIIOS(web_ui)) {
  DCHECK(profile_);
  DCHECK(!profile_->IsOffTheRecord());

  // Create a URLDataSource and add resources.
  BraveWebUIIOSDataSource* source = brave::CreateAndAddWebUIDataSource(
      web_ui, url.host(), kAiChatUiGenerated, IDR_AI_CHAT_UI_HTML);

  source->AddResourcePath("styles.css", IDR_AI_CHAT_UI_CSS);
  source->AddResourcePath("manifest.webmanifest", IDR_AI_CHAT_UI_MANIFEST);
  source->AddResourcePath("pwa_icon.svg", IDR_AI_CHAT_UI_PWA_ICON);

  source->AddLocalizedStrings(webui::kAiChatStrings);

  source->AddBoolean("isMobile", true);
  source->AddBoolean("isHistoryEnabled",
                     ai_chat::features::IsAIChatHistoryEnabled());
  source->AddBoolean("isAIChatAgentProfileFeatureEnabled",
                     ai_chat::features::IsAIChatAgentProfileEnabled());
  source->AddBoolean("isAIChatAgentProfile", false);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' blob: chrome://resources chrome://favicon2;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src 'self' chrome://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ChildSrc,
      absl::StrFormat("child-src %s;", kAIChatUntrustedConversationUIURL));

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types default;");

  web::URLDataSourceIOS::Add(
      profile_,
      new FaviconSource(profile_, chrome::FaviconUrlFormat::kFavicon2));

  // Bind Mojom Interface
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&AIChatUI::BindInterfaceUIHandler,
                          base::Unretained(this)));

  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&AIChatUI::BindInterfaceChatService,
                          base::Unretained(this)));

  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&AIChatUI::BindInterfaceParentUIFrame,
                          base::Unretained(this)));

  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&AIChatUI::BindInterfaceHistoryUIHandler,
                          base::Unretained(this)));

  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&AIChatUI::BindInterfaceBookmarksPageHandler,
                          base::Unretained(this)));

  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&AIChatUI::BindInterfaceTabTrackerService,
                          base::Unretained(this)));
}

AIChatUI::~AIChatUI() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::TabTrackerService::Name_);
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::BookmarksPageHandler::Name_);
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::HistoryUIHandler::Name_);
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::ParentUIFrame::Name_);
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::Service::Name_);
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::AIChatUIHandler::Name_);
}

void AIChatUI::BindInterfaceUIHandler(
    mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver) {
  id<AIChatUIHandlerBridge> bridge =
      ai_chat::UIHandlerBridgeHolder::GetOrCreateForWebState(
          web_ui()->GetWebState())
          ->bridge();
  web::WebState* chat_context_web_state = nullptr;
  if (BraveWebView* webView = [bridge webViewForAssociatedContent]) {
    chat_context_web_state = webView.webState;
  }
  page_handler_ = std::make_unique<ai_chat::AIChatUIPageHandler>(
      web_ui()->GetWebState(), chat_context_web_state, profile_,
      std::move(receiver));
}

void AIChatUI::BindInterfaceChatService(
    mojo::PendingReceiver<ai_chat::mojom::Service> receiver) {
  ai_chat::AIChatServiceFactory::GetForProfile(profile_)->Bind(
      std::move(receiver));
}

void AIChatUI::BindInterfaceParentUIFrame(
    mojo::PendingReceiver<ai_chat::mojom::ParentUIFrame>
        parent_ui_frame_receiver) {
  CHECK(page_handler_);
  page_handler_->BindParentUIFrameFromChildFrame(
      std::move(parent_ui_frame_receiver));
}

void AIChatUI::BindInterfaceHistoryUIHandler(
    mojo::PendingReceiver<ai_chat::mojom::HistoryUIHandler> receiver) {
  // TODO: https://github.com/brave/brave-browser/issues/51184 Add support
  // for associating bookmarks/history
  NOTIMPLEMENTED();
}

void AIChatUI::BindInterfaceBookmarksPageHandler(
    mojo::PendingReceiver<ai_chat::mojom::BookmarksPageHandler> receiver) {
  // TODO: https://github.com/brave/brave-browser/issues/51184 Add support
  // for associating bookmarks/history
  NOTIMPLEMENTED();
}

void AIChatUI::BindInterfaceTabTrackerService(
    mojo::PendingReceiver<ai_chat::mojom::TabTrackerService> receiver) {
  auto* service = ai_chat::TabTrackerServiceFactory::GetForProfile(profile_);
  CHECK(service);
  service->Bind(std::move(receiver));
}
