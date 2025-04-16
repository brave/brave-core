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
#include "components/user_prefs/user_prefs.h"
#include "components/version_info/channel.h"
#include "components/version_info/version_info.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/common/channel_info.h"

namespace ai_chat {

// static
AIChatService* AIChatServiceFactory::GetForProfile(ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<AIChatService>(profile, true);
}

// static
AIChatServiceFactory* AIChatServiceFactory::GetInstance() {
  static base::NoDestructor<AIChatServiceFactory> instance;
  return instance.get();
}

AIChatServiceFactory::AIChatServiceFactory()
    : ProfileKeyedServiceFactoryIOS("AIChatService",
                                    ProfileSelection::kRedirectedInIncognito,
                                    ServiceCreation::kCreateLazily,
                                    TestingCreation::kNoServiceForTests),
      // NOTE: Currently, there are no iOS AIChat metrics that depend on profile
      // prefs, so passing nullptr is acceptable here.
      // If this constraint changes, the following issue
      // must be addressed first:
      // https://github.com/brave/brave-browser/issues/45459
      ai_chat_metrics_(std::make_unique<AIChatMetrics>(
          GetApplicationContext()->GetLocalState(),
          nullptr)) {}

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
        return skus::SkusServiceFactory::GetForProfile(profile);
      },
      base::Unretained(profile));
  auto credential_manager = std::make_unique<AIChatCredentialManager>(
      std::move(skus_service_getter), GetApplicationContext()->GetLocalState());
  ModelService* model_service = ModelServiceFactory::GetForProfile(profile);
  return std::make_unique<AIChatService>(
      model_service, nullptr /* tab_tracker_service */,
      std::move(credential_manager), user_prefs::UserPrefs::Get(context),
      ai_chat_metrics_.get(), GetApplicationContext()->GetOSCryptAsync(),
      context->GetSharedURLLoaderFactory(),
      version_info::GetChannelString(::GetChannel()), profile->GetStatePath());
}

}  // namespace ai_chat
