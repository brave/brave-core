/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_PLAYLIST_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_PLAYLIST_PLAYLIST_SERVICE_FACTORY_H_

#include <memory>

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace playlist {
class PlaylistService;
class PlaylistYoutubeDownComponentManager;

class PlaylistServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static bool IsPlaylistEnabled(content::BrowserContext* context);

  static PlaylistService* GetForBrowserContext(
      content::BrowserContext* context);
  static PlaylistServiceFactory* GetInstance();

  PlaylistServiceFactory(const PlaylistServiceFactory&) = delete;
  PlaylistServiceFactory& operator=(const PlaylistServiceFactory&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<PlaylistServiceFactory>;

  PlaylistServiceFactory();
  ~PlaylistServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  // youtubedown.js component is global extension and it's used all service
  // instances.
  std::unique_ptr<PlaylistYoutubeDownComponentManager>
      playlist_youtubedown_component_manager_;
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_PLAYLIST_PLAYLIST_SERVICE_FACTORY_H_
