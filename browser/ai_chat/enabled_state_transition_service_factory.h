// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_ENABLED_STATE_TRANSITION_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_ENABLED_STATE_TRANSITION_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {

class EnabledStateTransitionService;

class EnabledStateTransitionServiceFactory : public ProfileKeyedServiceFactory {
 public:
  EnabledStateTransitionServiceFactory(
      const EnabledStateTransitionServiceFactory&) = delete;
  EnabledStateTransitionServiceFactory& operator=(
      const EnabledStateTransitionServiceFactory&) = delete;

  static EnabledStateTransitionServiceFactory* GetInstance();
  static EnabledStateTransitionService* GetForBrowserContext(
      content::BrowserContext* context);

  bool ServiceIsCreatedWithBrowserContext() const override;

 private:
  friend base::NoDestructor<EnabledStateTransitionServiceFactory>;

  EnabledStateTransitionServiceFactory();
  ~EnabledStateTransitionServiceFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_ENABLED_STATE_TRANSITION_SERVICE_FACTORY_H_
