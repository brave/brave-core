// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace commands {

class AcceleratorServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static AcceleratorService* GetForContext(content::BrowserContext* context);
  static AcceleratorServiceFactory* GetInstance();

  AcceleratorServiceFactory(const AcceleratorServiceFactory&) = delete;
  AcceleratorServiceFactory& operator=(const AcceleratorServiceFactory&) =
      delete;

 private:
  friend base::NoDestructor<AcceleratorServiceFactory>;

  AcceleratorServiceFactory();
  ~AcceleratorServiceFactory() override;

  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_SERVICE_FACTORY_H_
