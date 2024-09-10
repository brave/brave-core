// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/ai_chat/model_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace ai_chat {
ModelService* ModelServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<ModelService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

ModelService* ModelServiceFactory::GetForBrowserStateIfExists(
    ChromeBrowserState* browser_state) {
  return static_cast<ModelService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

ModelServiceFactory* ModelServiceFactory::GetInstance() {
  static base::NoDestructor<ModelServiceFactory> instance;
  return instance.get();
}

ModelServiceFactory::ModelServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "ModelService",
          BrowserStateDependencyManager::GetInstance()) {}

ModelServiceFactory::~ModelServiceFactory() {}

std::unique_ptr<KeyedService> ModelServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return std::make_unique<ModelService>(user_prefs::UserPrefs::Get(context));
}

web::BrowserState* ModelServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

bool ModelServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
}  // namespace ai_chat
