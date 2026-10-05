// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_embeddings_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/history_embeddings/brave_history_embeddings_status.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"

namespace ai_chat {

// static
AIChatEmbeddingsServiceFactory* AIChatEmbeddingsServiceFactory::GetInstance() {
  static base::NoDestructor<AIChatEmbeddingsServiceFactory> instance;
  return instance.get();
}

// static
AIChatEmbeddingsService* AIChatEmbeddingsServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AIChatEmbeddingsService*>(
      GetInstance()->GetServiceForBrowserContext(context, /*create=*/true));
}

AIChatEmbeddingsServiceFactory::AIChatEmbeddingsServiceFactory()
    : ProfileKeyedServiceFactory(
          "AIChatEmbeddingsService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {
  DependsOn(AIChatServiceFactory::GetInstance());
}

AIChatEmbeddingsServiceFactory::~AIChatEmbeddingsServiceFactory() = default;

std::unique_ptr<KeyedService>
AIChatEmbeddingsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!history_embeddings::BraveHistoryEmbeddingsStatus::GetForProfile(profile)
           ->IsEnabled()) {
    // An index left from before semantic search was turned off isn't kept.
    AIChatEmbeddingsService::DeleteIndex(profile->GetPath());
    return nullptr;
  }
  AIChatService* ai_chat_service =
      AIChatServiceFactory::GetForBrowserContext(context);
  if (!ai_chat_service) {
    return nullptr;
  }
  passage_embeddings::PassageEmbeddingsServiceController* controller =
      passage_embeddings::GetChromePassageEmbeddingsServiceController();
  return std::make_unique<AIChatEmbeddingsService>(
      ai_chat_service, profile->GetPrefs(), g_browser_process->os_crypt_async(),
      controller->GetEmbedder(), controller, profile->GetPath());
}

// Started with the profile, so stored conversations are indexed before Leo is
// first used.
bool AIChatEmbeddingsServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool AIChatEmbeddingsServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace ai_chat
