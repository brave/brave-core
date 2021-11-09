// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}

namespace brave_news {

class BraveNewsController;

class BraveNewsControllerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static BraveNewsController* GetForContext(content::BrowserContext* context);
  static BraveNewsControllerFactory* GetInstance();

  BraveNewsControllerFactory(const BraveNewsControllerFactory&) = delete;
  BraveNewsControllerFactory& operator=(const BraveNewsControllerFactory&) =
      delete;

 private:
  friend struct base::DefaultSingletonTraits<BraveNewsControllerFactory>;

  BraveNewsControllerFactory();
  ~BraveNewsControllerFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_news

#endif  // BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_FACTORY_H_
