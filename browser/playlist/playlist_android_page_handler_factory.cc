/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_android_page_handler_factory.h"

#include <utility>

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace playlist {

// static
PlaylistAndroidPageHandlerFactory*
PlaylistAndroidPageHandlerFactory::GetInstance() {
  return base::Singleton<PlaylistAndroidPageHandlerFactory>::get();
}

// static
mojo::PendingRemote<mojom::PageHandler>
PlaylistAndroidPageHandlerFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<PlaylistAndroidPageHandler*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
PlaylistAndroidPageHandler*
PlaylistAndroidPageHandlerFactory::GetServiceForContext(
    content::BrowserContext* context) {
  return static_cast<PlaylistAndroidPageHandler*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void PlaylistAndroidPageHandlerFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::PageHandler> receiver) {
  auto* playlist_page_handler =
      PlaylistAndroidPageHandlerFactory::GetServiceForContext(context);
  if (playlist_page_handler) {
    playlist_page_handler->Bind(std::move(receiver));
  }
}

PlaylistAndroidPageHandlerFactory::PlaylistAndroidPageHandlerFactory()
    : BrowserContextKeyedServiceFactory(
          "PlaylistAndroidPageHandler",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(playlist::PlaylistServiceFactory::GetInstance());
}

PlaylistAndroidPageHandlerFactory::~PlaylistAndroidPageHandlerFactory() =
    default;

KeyedService* PlaylistAndroidPageHandlerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  playlist::PlaylistServiceFactory::GetInstance()->GetForBrowserContext(
      context);
  return new PlaylistAndroidPageHandler(profile);
}

content::BrowserContext*
PlaylistAndroidPageHandlerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace playlist