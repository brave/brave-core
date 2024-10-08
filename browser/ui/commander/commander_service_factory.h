// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDER_COMMANDER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_UI_COMMANDER_COMMANDER_SERVICE_FACTORY_H_

#include <memory>

#include "brave/browser/ui/commander/commander_service.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace commander {

class CommanderServiceFactory : public ProfileKeyedServiceFactory {
 public:
  CommanderServiceFactory(const CommanderServiceFactory&) = delete;
  CommanderServiceFactory& operator=(const CommanderServiceFactory&) = delete;

  static CommanderServiceFactory* GetInstance();
  static CommanderService* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  CommanderServiceFactory();
  ~CommanderServiceFactory() override;
  friend base::NoDestructor<CommanderServiceFactory>;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

}  // namespace commander

#endif  // BRAVE_BROWSER_UI_COMMANDER_COMMANDER_SERVICE_FACTORY_H_
