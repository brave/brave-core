/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/ollama/ollama_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace ai_chat {

// static
OllamaServiceFactory* OllamaServiceFactory::GetInstance() {
  static base::NoDestructor<OllamaServiceFactory> instance;
  return instance.get();
}

// static
OllamaService* OllamaServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<OllamaService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
ProfileSelections OllamaServiceFactory::CreateProfileSelections() {
  if (!features::IsAIChatEnabled()) {
    return ProfileSelections::BuildNoProfilesSelected();
  }
  return ProfileSelections::Builder()
      .WithRegular(ProfileSelection::kOriginalOnly)
      .Build();
}

OllamaServiceFactory::OllamaServiceFactory()
    : ProfileKeyedServiceFactory("OllamaServiceFactory",
                                 CreateProfileSelections()) {}

OllamaServiceFactory::~OllamaServiceFactory() = default;

std::unique_ptr<KeyedService>
OllamaServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto url_loader_factory = context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess();
  return std::make_unique<OllamaService>(url_loader_factory);
}

}  // namespace ai_chat
