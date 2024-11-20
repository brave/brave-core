// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/ai_chat/ai_chat_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_feedback_api.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/ios/browser/api/ai_chat/model_service_factory.h"
#include "brave/ios/browser/skus/skus_service_factory.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "components/version_info/channel.h"
#include "components/version_info/version_info.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/common/channel_info.h"

namespace ai_chat {

// static
AIChatService* AIChatServiceFactory::GetForBrowserState(ProfileIOS* profile) {
  return static_cast<AIChatService*>(
      GetInstance()->GetServiceForBrowserState(profile, true));
}

// static
AIChatServiceFactory* AIChatServiceFactory::GetInstance() {
  static base::NoDestructor<AIChatServiceFactory> instance;
  return instance.get();
}

AIChatServiceFactory::AIChatServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "AIChatService",
          BrowserStateDependencyManager::GetInstance()),
      ai_chat_metrics_(std::make_unique<AIChatMetrics>(
          GetApplicationContext()->GetLocalState())) {}

AIChatServiceFactory::~AIChatServiceFactory() {}

std::unique_ptr<KeyedService> AIChatServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  if (!features::IsAIChatEnabled()) {
    return nullptr;
  }
  auto* profile = ProfileIOS::FromBrowserState(context);
  if (profile->IsOffTheRecord()) {
    return nullptr;
  }

  auto skus_service_getter = base::BindRepeating(
      [](ProfileIOS* profile) {
        return skus::SkusServiceFactory::GetForBrowserState(profile);
      },
      base::Unretained(profile));
  auto credential_manager = std::make_unique<AIChatCredentialManager>(
      std::move(skus_service_getter), GetApplicationContext()->GetLocalState());
  ModelService* model_service =
      ModelServiceFactory::GetForBrowserState(profile);
  return std::make_unique<AIChatService>(
      model_service, std::move(credential_manager),
      user_prefs::UserPrefs::Get(context), ai_chat_metrics_.get(),
      GetApplicationContext()->GetOSCryptAsync(),
      context->GetSharedURLLoaderFactory(),
      version_info::GetChannelString(::GetChannel()),
      ProfileIOS::FromBrowserState(context)->GetStatePath());
}

web::BrowserState* AIChatServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

bool AIChatServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
}  // namespace ai_chat
