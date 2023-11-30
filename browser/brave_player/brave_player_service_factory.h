/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_SERVICE_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_player {

class BravePlayerService;

class BravePlayerServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static BravePlayerServiceFactory& GetInstance();
  static BravePlayerService& GetForBrowserContext(
      content::BrowserContext* context);

  BravePlayerServiceFactory(const BravePlayerServiceFactory&) = delete;
  BravePlayerServiceFactory& operator=(const BravePlayerServiceFactory&) =
      delete;

  ~BravePlayerServiceFactory() override;

 private:
  friend base::NoDestructor<BravePlayerServiceFactory>;
  BravePlayerServiceFactory();

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace brave_player

#endif  // BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_SERVICE_FACTORY_H_
