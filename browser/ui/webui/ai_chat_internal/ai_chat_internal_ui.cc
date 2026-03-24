// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat_internal/ai_chat_internal_ui.h"

#include <utility>

#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/resources/ai_chat_internal/grit/ai_chat_internal_generated_map.h"
#include "brave/browser/resources/ai_chat_internal/grit/ai_chat_internal_static_resources.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

AIChatInternalUI::AIChatInternalUI(content::WebUI* web_ui,
                                   std::string_view host)
    : ui::MojoWebUIController(web_ui) {
  profile_ = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile_, std::string(host));
  webui::SetupWebUIDataSource(source, kAiChatInternalGenerated,
                              IDR_AI_CHAT_INTERNAL_STATIC_INDEX_HTML);
}

AIChatInternalUI::~AIChatInternalUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(AIChatInternalUI)

void AIChatInternalUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::Service> receiver) {
  ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_)->Bind(
      std::move(receiver));
}
