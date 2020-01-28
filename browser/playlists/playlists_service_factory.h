/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLISTS_PLAYLISTS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_PLAYLISTS_PLAYLISTS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace brave_playlists {
class PlaylistsService;

class PlaylistsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static PlaylistsService* GetForProfile(Profile* profile);
  static PlaylistsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PlaylistsServiceFactory>;

  PlaylistsServiceFactory();
  ~PlaylistsServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(PlaylistsServiceFactory);
};

}  // namespace brave_playlists

#endif  // BRAVE_BROWSER_PLAYLISTS_PLAYLISTS_SERVICE_FACTORY_H_
