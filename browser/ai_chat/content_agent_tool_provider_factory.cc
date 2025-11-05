// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/content_agent_tool_provider_factory.h"

#include <memory>

#include "brave/browser/ai_chat/content_agent_tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "chrome/browser/profiles/profile.h"

namespace ai_chat {

ContentAgentToolProviderFactory::ContentAgentToolProviderFactory(
    Profile* profile,
    actor::ActorKeyedService* actor_service)
    : actor_service_(actor_service), profile_(profile) {}

ContentAgentToolProviderFactory::~ContentAgentToolProviderFactory() = default;

std::unique_ptr<ToolProvider>
ContentAgentToolProviderFactory::CreateToolProvider() {
  return std::make_unique<ContentAgentToolProvider>(profile_,
                                                    actor_service_.get());
}

}  // namespace ai_chat
