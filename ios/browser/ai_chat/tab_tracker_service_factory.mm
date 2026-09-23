// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ai_chat/tab_tracker_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace ai_chat {

// static
TabTrackerService* TabTrackerServiceFactory::GetForProfile(
    ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<TabTrackerService>(profile,
                                                                  true);
}

// static
TabTrackerServiceFactory* TabTrackerServiceFactory::GetInstance() {
  static base::NoDestructor<TabTrackerServiceFactory> instance;
  return instance.get();
}

TabTrackerServiceFactory::TabTrackerServiceFactory()
    : ProfileKeyedServiceFactoryIOS("TabTrackerService",
                                    ProfileSelection::kNoInstanceInIncognito,
                                    ServiceCreation::kCreateWithProfile,
                                    TestingCreation::kNoServiceForTests) {}

TabTrackerServiceFactory::~TabTrackerServiceFactory() = default;

std::unique_ptr<KeyedService> TabTrackerServiceFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  if (!features::IsAIChatEnabled()) {
    return nullptr;
  }
  return std::make_unique<TabTrackerService>();
}

}  // namespace ai_chat
