// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_OLLAMA_OLLAMA_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_OLLAMA_OLLAMA_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace base {

template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {

class OllamaService;

class OllamaServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static OllamaServiceFactory* GetInstance();
  static OllamaService* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<OllamaServiceFactory>;

  static ProfileSelections CreateProfileSelections();

  OllamaServiceFactory();
  ~OllamaServiceFactory() override;

  // ProfileKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};
}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_OLLAMA_OLLAMA_SERVICE_FACTORY_H_
