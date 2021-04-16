/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_EVENT_ROUTER_FACTORY_H_
#define BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_EVENT_ROUTER_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace playlist {

// PlaylistEventRouter relays playlist service's event to extensions event.
// With this, internal webui/extensions can get playlist status change.
class PlaylistEventRouterFactory : public BrowserContextKeyedServiceFactory {
 public:
  class PlaylistEventRouter;

  static PlaylistEventRouterFactory* GetInstance();
  PlaylistEventRouter* GetForBrowserContext(content::BrowserContext* context);

  PlaylistEventRouterFactory(const PlaylistEventRouterFactory&) = delete;
  PlaylistEventRouterFactory& operator=(const PlaylistEventRouterFactory&) =
      delete;

 private:
  friend struct base::DefaultSingletonTraits<PlaylistEventRouterFactory>;

  PlaylistEventRouterFactory();
  ~PlaylistEventRouterFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_EXTENSIONS_API_PLAYLIST_PLAYLIST_EVENT_ROUTER_FACTORY_H_
