// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_ON_DEVICE_AI_ON_DEVICE_AI_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_ON_DEVICE_AI_ON_DEVICE_AI_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/on_device_ai/core/on_device_ai.mojom.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class KeyedService;
class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace on_device_ai {

class OnDeviceAIService;  // core/on_device_ai_service.h

class OnDeviceAIServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static OnDeviceAIServiceFactory* GetInstance();
  static mojo::PendingRemote<mojom::OnDeviceAIService> GetForProfile(
      Profile* profile);
  static void BindForProfile(
      Profile* profile,
      mojo::PendingReceiver<mojom::OnDeviceAIService> receiver);

 private:
  friend base::NoDestructor<OnDeviceAIServiceFactory>;
  OnDeviceAIServiceFactory();
  ~OnDeviceAIServiceFactory() override;

  static OnDeviceAIService* GetServiceForProfile(Profile* profile);

  OnDeviceAIServiceFactory(const OnDeviceAIServiceFactory&) = delete;
  OnDeviceAIServiceFactory& operator=(const OnDeviceAIServiceFactory&) = delete;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace on_device_ai

#endif  // BRAVE_BROWSER_ON_DEVICE_AI_ON_DEVICE_AI_SERVICE_FACTORY_H_
