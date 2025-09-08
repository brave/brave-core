// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/untrusted_frame.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/webui/webui_resources.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/api/ai_chat/tab_data_web_state_observer.h"
#include "brave/ios/browser/api/ai_chat/tab_tracker_service_factory.h"
#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "brave/ios/web/webui/favicon_source.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser/browser.h"
#include "ios/chrome/browser/shared/model/browser/browser_list.h"
#include "ios/chrome/browser/shared/model/browser/browser_list_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web/public/webui/web_ui_ios_message_handler.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/resource_path.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

web::WebState* GetActiveWebState(web::WebUIIOS* web_ui) {
  web::WebState* active_web_state =
      ai_chat::TabDataWebStateObserver::GetActiveTab();
  if (active_web_state) {
    DCHECK_EQ(active_web_state->GetBrowserState(),
              web_ui->GetWebState()->GetBrowserState());
    return active_web_state;
  }
  return nullptr;
}

}  // namespace

AIChatUI::AIChatUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()),
      profile_(ProfileIOS::FromWebUIIOS(web_ui)),
      active_web_state_(GetActiveWebState(web_ui)) {
  DCHECK(profile_);
  DCHECK(!profile_->IsOffTheRecord());

  // Create a URLDataSource and add resources.
  BraveWebUIIOSDataSource* source = brave::CreateAndAddWebUIDataSource(
      web_ui, url.host(), kAiChatUiGenerated, IDR_AI_CHAT_UI_HTML);

  source->AddResourcePath("styles.css", IDR_AI_CHAT_UI_CSS);
  source->AddResourcePath("manifest.webmanifest", IDR_AI_CHAT_UI_MANIFEST);
  source->AddResourcePath("pwa_icon.svg", IDR_AI_CHAT_UI_PWA_ICON);

  source->AddLocalizedStrings(webui::kAiChatStrings);

  constexpr bool kIsMobile = BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS);
  source->AddBoolean("isMobile", kIsMobile);
  source->AddBoolean("isHistoryEnabled",
                     ai_chat::features::IsAIChatHistoryEnabled());

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
      base::StringPrintf("child-src %s;", kAIChatUntrustedConversationUIURL));

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
      base::BindRepeating(&AIChatUI::BindInterfaceTabTracker,
                          base::Unretained(this)));
}

AIChatUI::~AIChatUI() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::TabTrackerService::Name_);
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::ParentUIFrame::Name_);
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::Service::Name_);
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      ai_chat::mojom::AIChatUIHandler::Name_);
}

void AIChatUI::BindInterfaceUIHandler(
    mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver) {
  // Don't associate with the WebUI's web_state
  if (active_web_state_ == web_ui()->GetWebState()) {
    active_web_state_ = nullptr;
  }

  page_handler_ = std::make_unique<ai_chat::AIChatUIPageHandler>(
      web_ui()->GetWebState(), active_web_state_, profile_,
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

void AIChatUI::BindInterfaceTabTracker(
    mojo::PendingReceiver<ai_chat::mojom::TabTrackerService> pending_receiver) {
  auto* service = ai_chat::TabTrackerServiceFactory::GetForProfile(profile_);
  CHECK(service);

  service->Bind(std::move(pending_receiver));
}
