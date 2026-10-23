// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_TAB_TRACKER_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_TAB_TRACKER_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace ai_chat {

class TabTrackerServiceFactory
    : public TypedProfileKeyedServiceFactoryIOS<TabTrackerServiceFactory,
                                                TabTrackerService> {
 public:
  explicit TabTrackerServiceFactory(PassKey key);

  TabTrackerServiceFactory(const TabTrackerServiceFactory&) = delete;
  TabTrackerServiceFactory& operator=(const TabTrackerServiceFactory&) = delete;

 private:
  // TypedProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_TAB_TRACKER_SERVICE_FACTORY_H_
