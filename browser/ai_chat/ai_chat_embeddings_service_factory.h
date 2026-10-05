// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_EMBEDDINGS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_EMBEDDINGS_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {

class AIChatEmbeddingsService;

// Builds the on-device index of a profile's Leo conversations and memories,
// which the Semantic history search setting turns on along with the index of
// its browsing history.
class AIChatEmbeddingsServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static AIChatEmbeddingsServiceFactory* GetInstance();
  // Null while semantic search is off for the profile.
  static AIChatEmbeddingsService* GetForBrowserContext(
      content::BrowserContext* context);

  AIChatEmbeddingsServiceFactory(const AIChatEmbeddingsServiceFactory&) =
      delete;
  AIChatEmbeddingsServiceFactory& operator=(
      const AIChatEmbeddingsServiceFactory&) = delete;

 private:
  friend base::NoDestructor<AIChatEmbeddingsServiceFactory>;

  AIChatEmbeddingsServiceFactory();
  ~AIChatEmbeddingsServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_EMBEDDINGS_SERVICE_FACTORY_H_
