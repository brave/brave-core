/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/ollama/ollama_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/content/browser/model_service_factory.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_model_fetcher.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/user_prefs/user_prefs.h"
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
                                 CreateProfileSelections()) {
  DependsOn(ModelServiceFactory::GetInstance());
}

OllamaServiceFactory::~OllamaServiceFactory() = default;

std::unique_ptr<KeyedService>
OllamaServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto url_loader_factory = context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess();
  auto ollama_service = std::make_unique<OllamaService>(url_loader_factory);

  // Create OllamaModelFetcher and wire it to the OllamaService
  auto* model_service = ModelServiceFactory::GetForBrowserContext(context);
  auto* prefs = user_prefs::UserPrefs::Get(context);
  if (model_service && prefs) {
    auto model_fetcher = std::make_unique<OllamaModelFetcher>(
        *model_service, prefs, ollama_service.get());
    ollama_service->SetModelFetcher(std::move(model_fetcher));
  }

  return ollama_service;
}

}  // namespace ai_chat
