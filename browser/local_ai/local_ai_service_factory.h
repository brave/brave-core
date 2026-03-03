// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_LOCAL_AI_LOCAL_AI_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_LOCAL_AI_LOCAL_AI_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class KeyedService;
class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace local_ai {

class LocalAIService;  // core/local_ai_service.h

class LocalAIServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static LocalAIServiceFactory* GetInstance();
  static mojo::PendingRemote<mojom::LocalAIService> GetForProfile(
      Profile* profile);
  static void BindForProfile(
      Profile* profile,
      mojo::PendingReceiver<mojom::LocalAIService> receiver);

 private:
  friend base::NoDestructor<LocalAIServiceFactory>;
  LocalAIServiceFactory();
  ~LocalAIServiceFactory() override;

  static LocalAIService* GetServiceForProfile(Profile* profile);

  LocalAIServiceFactory(const LocalAIServiceFactory&) = delete;
  LocalAIServiceFactory& operator=(const LocalAIServiceFactory&) = delete;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_LOCAL_AI_SERVICE_FACTORY_H_
