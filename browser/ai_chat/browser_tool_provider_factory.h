// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_FACTORY_H_

#include <memory>

#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider_factory.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"

class Profile;

namespace actor {
class ActorKeyedService;
}  // namespace actor

namespace ai_chat {

// Provides tools the ability to create, use, and group browser action tasks
// in order to act on tabs and get tab data for results.
class BrowserToolTaskProvider {
 public:
  virtual ~BrowserToolTaskProvider() = default;

  // Get the task ID for this instance
  virtual actor::TaskId GetTaskId() = 0;

  // Get the current tab for the task.
  // TODO(cr140): multiple
  // tabs will be able to be added to the task, observed and
  // acted on and we'll need to decide which one to act on. That
  // decision can be made by the AI, or a default one can be used.
  virtual void GetOrCreateTabHandleForTask(
      base::OnceCallback<void(tabs::TabHandle)> callback) = 0;

  // TODO(petemill): Now that we can send ToolRequest directly to the
  // ActorKeyedService, this method can accept those instead of the Actions
  // proto. It will be nicer for the Tools to build the tool requests directly
  // instead of dealing with the proto intermediaries.
  virtual void ExecuteActions(optimization_guide::proto::Actions actions,
                              Tool::UseToolCallback callback) = 0;
};

// Factory for creating ToolProvider instances in the browser layer
class BrowserToolProviderFactory : public ToolProviderFactory {
 public:
  explicit BrowserToolProviderFactory(Profile* profile,
                                      actor::ActorKeyedService* actor_service);
  ~BrowserToolProviderFactory() override;

  BrowserToolProviderFactory(const BrowserToolProviderFactory&) = delete;
  BrowserToolProviderFactory& operator=(const BrowserToolProviderFactory&) =
      delete;

  // ToolProviderFactory implementation
  std::unique_ptr<ToolProvider> CreateToolProvider() override;
  raw_ptr<actor::ActorKeyedService> actor_service_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_FACTORY_H_
