// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TOOL_PROVIDER_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TOOL_PROVIDER_FACTORY_H_

#include <memory>

#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider_factory.h"

class Profile;

namespace actor {
class ActorKeyedService;
}  // namespace actor

namespace ai_chat {

// Factory for creating ToolProvider instances in the browser layer for
// each conversation. Having a factory allows us to create a set of tools
// isolated from each conversation whilst having the AIChatService managing
// when to create them.
class ContentAgentToolProviderFactory : public ToolProviderFactory {
 public:
  explicit ContentAgentToolProviderFactory(
      Profile* profile,
      actor::ActorKeyedService* actor_service);
  ~ContentAgentToolProviderFactory() override;

  ContentAgentToolProviderFactory(const ContentAgentToolProviderFactory&) =
      delete;
  ContentAgentToolProviderFactory& operator=(
      const ContentAgentToolProviderFactory&) = delete;

  // ToolProviderFactory implementation
  std::unique_ptr<ToolProvider> CreateToolProvider() override;

 private:
  // Each instance needs an actor service to perform the actions. Not provided
  // if not enabled for the current profile.
  raw_ptr<actor::ActorKeyedService> actor_service_ = nullptr;

  // Ensures tabs are created and managed only for a specific profile
  raw_ptr<Profile> profile_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TOOL_PROVIDER_FACTORY_H_
