// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_MODEL_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_MODEL_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/ai_chat/core/browser/model_service.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace ai_chat {

class ModelServiceFactory
    : public TypedProfileKeyedServiceFactoryIOS<ModelServiceFactory,
                                                ModelService> {
 public:
  explicit ModelServiceFactory(PassKey key);

  ModelServiceFactory(const ModelServiceFactory&) = delete;
  ModelServiceFactory& operator=(const ModelServiceFactory&) = delete;

 private:
  // TypedProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};
}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_MODEL_SERVICE_FACTORY_H_
