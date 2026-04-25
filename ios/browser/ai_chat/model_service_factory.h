// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_MODEL_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_MODEL_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace base {

template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {

class ModelService;

class ModelServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  static ModelService* GetForProfile(ProfileIOS* profile);
  static ModelService* GetForProfileIfExists(ProfileIOS* profile);
  static ModelServiceFactory* GetInstance();

  ModelServiceFactory(const ModelServiceFactory&) = delete;
  ModelServiceFactory& operator=(const ModelServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<ModelServiceFactory>;

  ModelServiceFactory();
  ~ModelServiceFactory() override;

  // ProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};
}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_MODEL_SERVICE_FACTORY_H_
