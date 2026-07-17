// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_WORKSPACE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_WORKSPACE_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {
class WorkspaceService;

class WorkspaceServiceFactory : public ProfileKeyedServiceFactory {
 public:
  WorkspaceServiceFactory(const WorkspaceServiceFactory&) = delete;
  WorkspaceServiceFactory& operator=(const WorkspaceServiceFactory&) = delete;

  static WorkspaceServiceFactory* GetInstance();
  static WorkspaceService* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend base::NoDestructor<WorkspaceServiceFactory>;

  WorkspaceServiceFactory();
  ~WorkspaceServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_WORKSPACE_SERVICE_FACTORY_H_
