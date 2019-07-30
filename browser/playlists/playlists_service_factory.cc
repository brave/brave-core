/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlists/playlists_service_factory.h"

#include "brave/components/playlists/browser/playlists_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
PlaylistsServiceFactory* PlaylistsServiceFactory::GetInstance() {
  return base::Singleton<PlaylistsServiceFactory>::get();
}

// static
PlaylistsService* PlaylistsServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<PlaylistsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

PlaylistsServiceFactory::PlaylistsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PlaylistsService",
          BrowserContextDependencyManager::GetInstance()) {
}

PlaylistsServiceFactory::~PlaylistsServiceFactory() {
}

KeyedService* PlaylistsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PlaylistsService(Profile::FromBrowserContext(context));
}

content::BrowserContext* PlaylistsServiceFactory::GetBrowserContextToUse(
      content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
