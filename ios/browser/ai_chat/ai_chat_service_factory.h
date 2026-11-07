// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace base {

template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {

class AIChatService;

class AIChatServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  static AIChatService* GetForProfile(ProfileIOS* profile);
  static AIChatServiceFactory* GetInstance();

  AIChatServiceFactory(const AIChatServiceFactory&) = delete;
  AIChatServiceFactory& operator=(const AIChatServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<AIChatServiceFactory>;

  AIChatServiceFactory();
  ~AIChatServiceFactory() override;

  // ProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};
}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_
