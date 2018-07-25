/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef H_BRAVE_COMPONENTS_BRAVE_SYNC_CONTROLLER_FACTORY_H
#define H_BRAVE_COMPONENTS_BRAVE_SYNC_CONTROLLER_FACTORY_H

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace brave_sync {

class Controller;

class ControllerFactory : public BrowserContextKeyedServiceFactory {
public:
  static Controller* GetForBrowserContext(
      content::BrowserContext* browser_context);

  static Controller* GetForBrowserContextIfExists(
      content::BrowserContext* browser_context);

  static ControllerFactory* GetInstance();

private:
  friend struct base::DefaultSingletonTraits<ControllerFactory>;

  ControllerFactory();
  ~ControllerFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(ControllerFactory);
};


} // namespace brave_sync


#endif // H_BRAVE_COMPONENTS_BRAVE_SYNC_CONTROLLER_FACTORY_H
