// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat_internal/ai_chat_internal_ui.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/grit/ai_chat_internal_resources.h"
#include "brave/grit/ai_chat_internal_resources_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"

AIChatInternalUI::AIChatInternalUI(content::WebUI* web_ui,
                                   std::string_view host)
    : ui::MojoWebUIController(web_ui, /*enable_chrome_send=*/true) {
  profile_ = Profile::FromWebUI(web_ui);
  auto* source = CreateAndAddWebUIDataSource(
      web_ui, host, kAiChatInternalResources, IDR_AI_CHAT_INTERNAL_INDEX_HTML);
  DCHECK(source);
}

AIChatInternalUI::~AIChatInternalUI() = default;
WEB_UI_CONTROLLER_TYPE_IMPL(AIChatInternalUI)

void AIChatInternalUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::AIChatInternalPageHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void AIChatInternalUI::GetRawConversationData(
    const std::string& uuid,
    GetRawConversationDataCallback callback) {
  auto* service = ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_);
  if (!service) {
    std::move(callback).Run(ai_chat::mojom::Conversation::New(), {});
    return;
  }

  auto* handler = service->GetConversation(uuid);
  std::vector<ai_chat::mojom::ConversationTurnPtr> turns;
  ai_chat::mojom::ConversationPtr target_conv =
      ai_chat::mojom::Conversation::New();

  if (handler) {
    target_conv = handler->GetMetadataForTesting().Clone();
    for (const auto& turn : handler->GetConversationHistory()) {
      turns.push_back(turn.Clone());
    }
  }

  std::move(callback).Run(std::move(target_conv), std::move(turns));
}

void AIChatInternalUI::GetConversations(GetConversationsCallback callback) {
  auto* service = ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_);
  if (!service) {
    std::move(callback).Run({});
    return;
  }
  service->GetConversations(std::move(callback));
}
