// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_FACTORY_H_

#include <memory>

#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace content {
class BrowserContext;
}

namespace brave_news {

class BraveNewsController;

class BraveNewsControllerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static BraveNewsController* GetForBrowserContext(
      content::BrowserContext* context);
  static mojo::PendingRemote<mojom::BraveNewsController> GetRemoteService(
      content::BrowserContext* context);
  static BraveNewsControllerFactory* GetInstance();

  BraveNewsControllerFactory(const BraveNewsControllerFactory&) = delete;
  BraveNewsControllerFactory& operator=(const BraveNewsControllerFactory&) =
      delete;

  bool ServiceIsCreatedWithBrowserContext() const override;

 private:
  friend base::NoDestructor<BraveNewsControllerFactory>;

  BraveNewsControllerFactory();
  ~BraveNewsControllerFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace brave_news

#endif  // BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_FACTORY_H_
