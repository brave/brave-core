// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ai_chat/model_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace ai_chat {
ModelService* ModelServiceFactory::GetForProfile(ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<ModelService>(profile, true);
}

ModelService* ModelServiceFactory::GetForProfileIfExists(ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<ModelService>(profile, false);
}

ModelServiceFactory* ModelServiceFactory::GetInstance() {
  static base::NoDestructor<ModelServiceFactory> instance;
  return instance.get();
}

ModelServiceFactory::ModelServiceFactory()
    : ProfileKeyedServiceFactoryIOS("ModelService",
                                    ProfileSelection::kNoInstanceInIncognito,
                                    ServiceCreation::kCreateLazily,
                                    TestingCreation::kCreateService) {}

ModelServiceFactory::~ModelServiceFactory() {}

std::unique_ptr<KeyedService> ModelServiceFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return std::make_unique<ModelService>(user_prefs::UserPrefs::Get(profile));
}
}  // namespace ai_chat
