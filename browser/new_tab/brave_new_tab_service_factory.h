/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_BRAVE_NEW_TAB_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_NEW_TAB_BRAVE_NEW_TAB_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

class BraveNewTabService;

class BraveNewTabServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  BraveNewTabServiceFactory(const BraveNewTabServiceFactory&) = delete;
  BraveNewTabServiceFactory& operator=(const BraveNewTabServiceFactory&) =
      delete;

  static BraveNewTabService* GetServiceForContext(
      content::BrowserContext* context);
  static BraveNewTabServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BraveNewTabServiceFactory>;

  BraveNewTabServiceFactory();
  ~BraveNewTabServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_NEW_TAB_BRAVE_NEW_TAB_SERVICE_FACTORY_H_
