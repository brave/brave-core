// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_service_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/skus/skus_service_factory.h"
#include "brave/components/ai_chat/content/browser/model_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "components/version_info/channel.h"
#include "content/public/browser/storage_partition.h"

namespace ai_chat {

// static
AIChatServiceFactory* AIChatServiceFactory::GetInstance() {
  static base::NoDestructor<AIChatServiceFactory> instance;
  return instance.get();
}

// static
AIChatService* AIChatServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  CHECK(context);
  if (!IsAllowedForContext(context) ||
      !ModelServiceFactory::GetForBrowserContext(context) ||
      !skus::SkusServiceFactory::GetForContext(context)) {
    return nullptr;
  }
  return static_cast<AIChatService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

AIChatServiceFactory::AIChatServiceFactory()
    : ProfileKeyedServiceFactory(
          "AIChatService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {
  DependsOn(skus::SkusServiceFactory::GetInstance());
  DependsOn(ModelServiceFactory::GetInstance());
}

AIChatServiceFactory::~AIChatServiceFactory() = default;

std::unique_ptr<KeyedService>
AIChatServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto skus_service_getter = base::BindRepeating(
      [](content::BrowserContext* context) {
        return skus::SkusServiceFactory::GetForContext(context);
      },
      context);
  std::unique_ptr<AIChatCredentialManager> credential_manager =
      std::make_unique<AIChatCredentialManager>(
          std::move(skus_service_getter), g_browser_process->local_state());

  return std::make_unique<AIChatService>(
      ModelServiceFactory::GetForBrowserContext(context),

      std::move(credential_manager), user_prefs::UserPrefs::Get(context),
      (g_brave_browser_process->process_misc_metrics())
          ? g_brave_browser_process->process_misc_metrics()->ai_chat_metrics()
          : nullptr,
      g_browser_process->os_crypt_async(),
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess(),
      version_info::GetChannelString(chrome::GetChannel()), context->GetPath());
}

}  // namespace ai_chat
