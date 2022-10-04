/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_page_handler_factory.h"

#include <utility>

#include "brave/browser/brave_browser_process.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace playlist {

// static
PlaylistPageHandlerFactory* PlaylistPageHandlerFactory::GetInstance() {
  return base::Singleton<PlaylistPageHandlerFactory>::get();
}

// static
mojo::PendingRemote<mojom::PageHandler>
PlaylistPageHandlerFactory::GetForContext(content::BrowserContext* context) {
  return static_cast<PlaylistPageHandler*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
PlaylistPageHandler* PlaylistPageHandlerFactory::GetServiceForContext(
    content::BrowserContext* context) {
  return static_cast<PlaylistPageHandler*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
void PlaylistPageHandlerFactory::BindForContext(
    content::BrowserContext* context,
    mojo::PendingReceiver<mojom::PageHandler> receiver) {
  auto* playlist_page_handler =
      PlaylistPageHandlerFactory::GetServiceForContext(context);
  if (playlist_page_handler) {
    playlist_page_handler->Bind(std::move(receiver));
  }
}

PlaylistPageHandlerFactory::PlaylistPageHandlerFactory()
    : BrowserContextKeyedServiceFactory(
          "PlaylistPageHandler",
          BrowserContextDependencyManager::GetInstance()) {}

PlaylistPageHandlerFactory::~PlaylistPageHandlerFactory() = default;

KeyedService* PlaylistPageHandlerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  return new PlaylistPageHandler(profile);
}

content::BrowserContext* PlaylistPageHandlerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace playlist
